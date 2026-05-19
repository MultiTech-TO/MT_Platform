#include "lora_app.h"

#include <zephyr/device.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/kernel.h>

/* Customize based on network configuration */
#define LORAWAN_DEV_EUI  {0xDD, 0xEE, 0xAA, 0xDD, 0xBB, 0xEE, 0xEE, 0xFF}
#define LORAWAN_JOIN_EUI {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define LORAWAN_APP_KEY                                                                            \
	{0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,                                           \
	 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C}

#define DELAY K_MSEC(10000)

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lorawan_class_a, CONFIG_LOG_DEFAULT_LEVEL);

// The max payload size used by the application. Used to allocate space for the
// payload queue.
#define PAYLOAD_MAX_SIZE 			32
#if PAYLOAD_MAX_SIZE > 255
#warning "Payload size may not exceed 255!"
#endif

K_FIFO_DEFINE(payload_queue);

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

int lora_app_init(void)
{
	const struct device *lora_dev;

	int ret;

	struct lorawan_downlink_cb downlink_cb = {.port = LW_RECV_PORT_ANY, .cb = dl_callback};

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

	lorawan_register_downlink_callback(&downlink_cb);
	lorawan_register_dr_changed_callback(lorwan_datarate_changed);
}

int64_t lora_app_tick = 0;

void lora_app_run(void)
{
	if (LRWAN_APP_STATE_e == NOT_JOINED) {
		int sc = lora_app_join();
		if(sc == 0){
			LRWAN_APP_STATE_e = JOIN_BUSY;
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

	} else if (LRWAN_APP_STATE_e == IDLE) {
		if(k_fifo_is_empty(&payload_queue)){
			return;
		}
		struct lora_queue_item_t queue_item = *(struct lora_queue_item_t *)k_fifo_get(&payload_queue, K_NO_WAIT);
		lorawan_send(1, queue_item.bytes, queue_item.payload_size, LORAWAN_MSG_CONFIRMED);

	} else if (LRWAN_APP_STATE_e == BUSY) {
	}
}

bool lora_app_is_joined(void);

void lora_app_add_uplink(uint8_t *data, uint8_t len){
	struct lora_queue_item_t queue_item = {0};
	for(int i = 0; i < len; i++){
		queue_item.bytes[i] = data[i];
	}
	queue_item.payload_size = len;
	k_fifo_put(&payload_queue, &queue_item);
}
