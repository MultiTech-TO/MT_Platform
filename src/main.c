/*
 * Class A LoRaWAN sample application
 *
 * Copyright (c) 2020 Manivannan Sadhasivam <mani@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include "sensors/sensor_manager.h"

#include "wireless/BLE/ble_app.h"
#include "wireless/BLE/co2_service/co2_service.h"
#include "wireless/LoRa/lora_app.h"

#include <zephyr/shell/shell.h>

#define DELAY K_MSEC(1000)

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

uint32_t sensor_tick = 0;

void run_sensor_thread(void){
	while(1){
		sensor_tick = k_uptime_get();
		while(k_uptime_get() - sensor_tick < 30000){
			k_sleep(DELAY);
		}
	}
	
}

int main(void)
{
	// ble_app_init();
	lora_app_init();
	sensors_init();
	
	while (1) {
		// ret = lorawan_send(2, data, sizeof(data),
		// 		   LORAWAN_MSG_CONFIRMED);

		// /*
		//  * Note: The stack may return -EAGAIN if the provided data
		//  * length exceeds the maximum possible one for the region and
		//  * datarate. But since we are just sending the same data here,
		//  * we'll just continue.
		//  */
		// if (ret == -EAGAIN) {
		// 	LOG_ERR("lorawan_send failed: %d. Continuing...", ret);
		// 	k_sleep(DELAY);
		// 	continue;
		// }

		// if (ret < 0) {
		// 	LOG_ERR("lorawan_send failed: %d", ret);
		// 	return 0;
		// }

		// LOG_INF("Data sent!");
		// ble_app_process_action();
		// lora_app_run();
		// sensor_run();
		
		k_sleep(DELAY);
	}
}
K_THREAD_DEFINE(blink0_id, STACKSIZE, run_sensor_thread, NULL, NULL, NULL,
		PRIORITY, 0, 0);
