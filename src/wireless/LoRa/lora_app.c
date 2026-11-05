#include "lora_app.h"

#include <zephyr/device.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/kernel.h>
#include <zephyr/smf.h>
#include <stdbool.h>

#include "time_conv.h"

/* Customize based on network configuration */
#define LORAWAN_DEV_EUI  {0x00, 0xAA, 0x00, 0xAA, 0x00, 0xAB, 0x00, 0x00}
#define LORAWAN_JOIN_EUI {0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB}
#define LORAWAN_APP_KEY                                                                            \
	{0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,                                           \
	 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77}

#define DELAY K_MSEC(10000)

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lorawan_class_a, CONFIG_LOG_DEFAULT_LEVEL);

static uint16_t join_backoff_times[] = {min_2_sec(15), min_2_sec(30), hr_2_sec(1), hr_2_sec(2),
					hr_2_sec(4),   hr_2_sec(8),   hr_2_sec(12)};

static bool join_started;
static bool is_joined;
static uint32_t curr_time;

struct message_item {
	uint16_t lux;
	uint16_t white;
};

K_MSGQ_DEFINE(payload_queue, sizeof(struct message_item), 10, 1);

char data[] = {'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r', 'l', 'd'};

uint8_t dev_eui[] = LORAWAN_DEV_EUI;
uint8_t join_eui[] = LORAWAN_JOIN_EUI;
uint8_t app_key[] = LORAWAN_APP_KEY;

// static enum {
// 	NOT_JOINED,
// 	JOIN_BUSY,
// 	JOIN_BACKOFF,
// 	JOINED,
// 	IDLE,
// 	BUSY
// } LRWAN_APP_STATE_e;

static int lora_app_join(void)
{
	struct lorawan_join_config join_cfg;
	join_cfg.mode = LORAWAN_ACT_OTAA;
	join_cfg.dev_eui = dev_eui;
	join_cfg.otaa.join_eui = join_eui;
	join_cfg.otaa.app_key = app_key;
	join_cfg.otaa.nwk_key = app_key;
	join_cfg.otaa.dev_nonce = 0u;

	LOG_INF("Joining network over OTAA");
	int ret = lorawan_join(&join_cfg);
	if (ret < 0) {
		LOG_ERR("lorawan_join_network failed: %d", ret);
		return ret;
	}

	return 0;
}

static bool dl_pending;

static void dl_callback(uint8_t port, uint8_t flags, int16_t rssi, int8_t snr, uint8_t len,
			const uint8_t *hex_data)
{
	LOG_INF("Port %d, Pending %d, RSSI %ddB, SNR %ddBm, Time %d", port,
		flags & LORAWAN_DATA_PENDING, rssi, snr, !!(flags & LORAWAN_TIME_UPDATED));
	if (hex_data) {
		LOG_HEXDUMP_INF(hex_data, len, "Payload: ");
	}
	dl_pending = false;
}

static void lorwan_datarate_changed(enum lorawan_datarate dr)
{
	uint8_t unused, max_size;

	lorawan_get_payload_sizes(&unused, &max_size);
	LOG_INF("New Datarate: DR_%d, Max Payload %d", dr, max_size);
}

static void lorawan_set_sub_band(uint8_t sub_band)
{
	sub_band--;
	uint8_t mask = 0xFF;
	uint16_t masks[6] = {0}; 
	masks[sub_band / 2] = 0xFF << sub_band % 2;
	lorawan_set_channels_mask(masks, sizeof(masks) / 2);
}

// This must be defined globally or the callback address will become corrupted when it goes out of
// scope
struct lorawan_downlink_cb downlink_cb = {.port = LW_RECV_PORT_ANY, .cb = dl_callback};

void lora_app_start_join(void)
{
	join_started = true;
}

bool lora_app_is_joined(void)
{
	return true;
}

void lora_app_add_uplink(struct sensor_value *lux, struct sensor_value *white)
{
	struct message_item msg = {.lux = lux->val1, .white = white->val1};

	k_msgq_put(&payload_queue, &msg, K_NO_WAIT);
}

enum LORA_STATE {
	DEEP_SLEEP,
	JOINING,
	BACKOFF,
	IDLE,
	BUSY
};

struct s_object {
	struct smf_ctx ctx;
} s_obj;

static void deep_sleep_entry(void *o);
static enum smf_state_result deep_sleep_run(void *o);
static void deep_sleep_exit(void *o);

static void joining_entry(void *o);
static enum smf_state_result joining_run(void *o);
static void joining_exit(void *o);

static void backoff_entry(void *o);
static enum smf_state_result backoff_run(void *o);
static void backoff_exit(void *o);

static void idle_entry(void *o);
static enum smf_state_result idle_run(void *o);
static void idle_exit(void *o);

static void busy_entry(void *o);
static enum smf_state_result busy_run(void *o);
static void busy_exit(void *o);

const struct smf_state lora_states[] = {
	[DEEP_SLEEP] =
		SMF_CREATE_STATE(deep_sleep_entry, deep_sleep_run, deep_sleep_exit, NULL, NULL),
	[JOINING] = SMF_CREATE_STATE(joining_entry, joining_run, joining_exit, NULL, NULL),
	[BACKOFF] = SMF_CREATE_STATE(backoff_entry, backoff_run, backoff_exit, NULL, NULL),
	[IDLE] = SMF_CREATE_STATE(idle_entry, idle_run, idle_exit, NULL, NULL),
	[BUSY] = SMF_CREATE_STATE(busy_entry, busy_run, busy_exit, NULL, NULL)};

static void deep_sleep_entry(void *o)
{
	LOG_PRINTK("DEEP_SLEEP\n");
	return;
}

static enum smf_state_result deep_sleep_run(void *o)
{
	if (join_started) {
		smf_set_state(SMF_CTX(&s_obj), &lora_states[JOINING]);
	}

	return SMF_EVENT_HANDLED;
}

static void deep_sleep_exit(void *o)
{
	join_started = false;
}

static void joining_entry(void *o)
{
	LOG_PRINTK("JOINING\n");

	return;
}

static int join_status;

static enum smf_state_result joining_run(void *o)
{
	join_status = lora_app_join();
	if (join_status > 0) {
		smf_set_state(SMF_CTX(&s_obj), &lora_states[BACKOFF]);
	}
	else {
		smf_set_state(SMF_CTX(&s_obj), &lora_states[IDLE]);
	}
	return SMF_EVENT_HANDLED;
}

static void joining_exit(void *o)
{
	join_status = 0;
	return;
}

int64_t backoff_ticks;
uint8_t backoff_idx;

static void backoff_entry(void *o)
{
	LOG_PRINTK("BACKOFF\n");
	backoff_ticks = k_uptime_get();
	return;
}
static enum smf_state_result backoff_run(void *o)
{
	if(k_uptime_get() - backoff_ticks >= join_backoff_times[backoff_idx]){
		backoff_idx++;
		smf_set_state(SMF_CTX(&s_obj), &lora_states[JOINING]);
	}
	return SMF_EVENT_HANDLED;
}

static void backoff_exit(void *o)
{
	return;
}

int64_t gpstime_tick;

static void idle_entry(void *o)
{
	LOG_PRINTK("IDLE\n");

	backoff_idx = 0;
	gpstime_tick = 0;
	return;
}

static enum smf_state_result idle_run(void *o)
{
	if(k_uptime_get() - gpstime_tick >= hr_2_sec(24) * 1000){
		gpstime_tick = k_uptime_get();
		lorawan_request_device_time(true);
	}
	if(k_msgq_num_used_get(&payload_queue) == 0){
		return SMF_EVENT_HANDLED;
	}
	struct message_item msg = {0};
	k_msgq_get(&payload_queue, &msg, K_FOREVER);
	lorawan_send(1, (uint8_t *)&msg, sizeof(msg), LORAWAN_MSG_CONFIRMED);
	LOG_HEXDUMP_INF((uint8_t *)&msg, sizeof(msg), "PAYLOAD: ");
		
	return SMF_EVENT_HANDLED;
}

static void idle_exit(void *o)
{
	return;
}

static void busy_entry(void *o){
	LOG_PRINTK("BUSY\n");

	return;
}

static enum smf_state_result busy_run(void *o){
	if(!dl_pending){
		
	}
	return SMF_EVENT_HANDLED;
}
static void busy_exit(void *o){
	return;
}

int lora_app_init(void)
{
	const struct device *lora_dev;

	int ret;

	lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s: device not ready.", lora_dev->name);
		return 0;
	}
	/* If more than one region Kconfig is selected, app should set region
	 * before calling lorawan_start()
	 */
	ret = lorawan_set_region(LORAWAN_REGION_US915);
	if (ret < 0) {
		LOG_ERR("lorawan_set_region failed: %d", ret);
		return 0;
	}

	smf_set_initial(SMF_CTX(&s_obj), &lora_states[DEEP_SLEEP]);

	ret = lorawan_start();
	if (ret < 0) {
		LOG_ERR("lorawan_start failed: %d", ret);
		return 0;
	}
	lorawan_set_datarate(LORAWAN_DR_1);
	lorawan_set_sub_band(1);
	lorawan_register_downlink_callback(&downlink_cb);
	lorawan_register_dr_changed_callback(lorwan_datarate_changed);
}

void lora_app_run(void)
{
	smf_run_state(SMF_CTX(&s_obj));
}
