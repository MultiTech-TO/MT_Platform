#include "lora_app.h"

#include <zephyr/device.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/kernel.h>
#include <stdbool.h>

#include "../payload.h"

/* Customize based on network configuration */
#define LORAWAN_DEV_EUI  {0x00, 0xAA, 0x00, 0xAA, 0x00, 0xAB, 0x00, 0x00}
#define LORAWAN_JOIN_EUI {0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB}
#define LORAWAN_APP_KEY                                                                            \
	{0xAA, 0xBB, 0xAA, 0xBB, 0xAA, 0xBB, 0xAA, 0xBB, 0xAA, 0xBB, 0xAA, 0xBB, 0xAA, 0xBB, 0xAA, 0xBB}

#define DELAY K_MSEC(10000)

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lorawan_class_a, CONFIG_LOG_DEFAULT_LEVEL);

static bool is_joined;
static uint32_t curr_time;

struct message_item{
	uint16_t co2;
	uint16_t temperature_i;
	uint16_t temperature_d;
	uint16_t humidity_i;
	uint16_t humidity_d;
};

K_MSGQ_DEFINE(payload_queue, sizeof(struct message_item), 10, 1);


char data[] = {'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r', 'l', 'd'};

uint8_t dev_eui[] = LORAWAN_DEV_EUI;
uint8_t join_eui[] = LORAWAN_JOIN_EUI;
uint8_t app_key[] = LORAWAN_APP_KEY;

static enum {
	NOT_JOINED,
	JOIN_BUSY,
	JOIN_BACKOFF,
	JOINED,
	IDLE,
	BUSY
} LRWAN_APP_STATE_e;

struct lora_queue_item_t{
	void *fifo_reserved;
	uint8_t bytes[PAYLOAD_MAX_SIZE];
	uint8_t payload_size;
};

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

static void dl_callback(
	uint8_t port, uint8_t flags, int16_t rssi, int8_t snr, uint8_t len, const uint8_t *hex_data)
{
	LOG_INF("Port %d, Pending %d, RSSI %ddB, SNR %ddBm, Time %d", port,
		flags & LORAWAN_DATA_PENDING, rssi, snr, !!(flags & LORAWAN_TIME_UPDATED));
	if (hex_data) {
		LOG_HEXDUMP_INF(hex_data, len, "Payload: ");
	}
}

static void lorwan_datarate_changed(enum lorawan_datarate dr)
{
	uint8_t unused, max_size;

	lorawan_get_payload_sizes(&unused, &max_size);
	LOG_INF("New Datarate: DR_%d, Max Payload %d", dr, max_size);
}

static void lorawan_set_sub_band(uint8_t sub_band){
	sub_band--;
	uint8_t mask = 0xFF;
	uint16_t masks[6] = {0};
	masks[sub_band/2] = 0xFF << sub_band % 2;
	lorawan_set_channels_mask(masks, sizeof(masks) / 2);
}

// This must be defined globally or the callback address will become corrupted when it goes out of scope
struct lorawan_downlink_cb downlink_cb = {.port = LW_RECV_PORT_ANY, .cb = dl_callback};

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

int64_t lora_app_tick = 0;

void lora_app_run(void)
{
	if (LRWAN_APP_STATE_e == NOT_JOINED) {
		int sc = lora_app_join();
		if(sc == 0){
			LRWAN_APP_STATE_e = JOINED;
		}
		else{
			LRWAN_APP_STATE_e = JOIN_BACKOFF;
			lora_app_tick = k_uptime_get();
		}
		return;
	}
	else if (LRWAN_APP_STATE_e == JOIN_BACKOFF) {
		if(k_uptime_get() - lora_app_tick >= 60000){
			LRWAN_APP_STATE_e = NOT_JOINED;
			
		}
	} else if (LRWAN_APP_STATE_e == JOINED) {
		// lorawan_request_device_time(false);
		uint8_t data[2] = {0x1, 0x2};
		lorawan_send(1, data, 2, LORAWAN_MSG_CONFIRMED);
		LRWAN_APP_STATE_e = IDLE;

	} else if (LRWAN_APP_STATE_e == IDLE) {
		if(k_msgq_num_used_get(&payload_queue) != 0){
			return;
		}
		struct message_item msg = {0};
		k_msgq_get(&payload_queue, &msg, K_FOREVER);
		lorawan_send(1, (uint8_t *)&msg, sizeof(msg), LORAWAN_MSG_CONFIRMED);
		LOG_HEXDUMP_INF((uint8_t *)&msg, sizeof(msg), "PAYLOAD: ");

	} else if (LRWAN_APP_STATE_e == BUSY) {
	}
}

bool lora_app_is_joined(void){
	return LRWAN_APP_STATE_e > JOINED;
}

void lora_app_add_uplink(co2_data_t *co2_data){
	if(LRWAN_APP_STATE_e < JOINED){
		return;
	}

	struct message_item msg = {.co2 = co2_data->co2.val1,
								.temperature_i = co2_data->temperature.val1,
								.temperature_d = co2_data->temperature.val2,
								.humidity_i = co2_data->humidity.val1,
								.humidity_d = co2_data->humidity.val2
							};

	k_msgq_put(&payload_queue, &msg, K_NO_WAIT);
}
