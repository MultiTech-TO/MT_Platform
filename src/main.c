/*
 * Class A LoRaWAN sample application
 *
 * Copyright (c) 2020 Manivannan Sadhasivam <mani@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include "ble_app.h"
#include "lora_app.h"

#define DELAY K_MSEC(10000)

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);



int main(void)
{
	ble_app_init();
	lora_app_init();

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
		ble_app_process_action();
		lora_app_run();
		k_sleep(DELAY);
	}
}
