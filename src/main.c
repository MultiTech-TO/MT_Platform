/*
 * Class A LoRaWAN sample application
 *
 * Copyright (c) 2020 Manivannan Sadhasivam <mani@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/input/input.h>
#include "wireless/LoRa/lora_app.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/shell/shell.h>

#include "display/display.h"

#include "sensors/light_sensor.h"

#define DELAY K_MSEC(1)
#define LOG_LEVEL LOG_LEVEL_INFO
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);


/* size of stack area used by each thread */
#define STACKSIZE 3072

/* scheduling priority used by each thread */
#define PRIORITY 2

K_EVENT_DEFINE(sensor_event);

bool btn_pressed = false;

static void button_input_cb(struct input_event *evt, void *user_data)
{
	if (evt->sync == 0) {
		return;
	}
	if(evt->value && !btn_pressed){
		btn_pressed = true;
	}
}

INPUT_CALLBACK_DEFINE(NULL, button_input_cb, NULL);

void lora_thread_loop(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	lora_app_init();

	while (true) {
		k_sleep(DELAY);
		lora_app_run();
	}
}

K_THREAD_DEFINE(lora_thread, STACKSIZE, lora_thread_loop, NULL, NULL, NULL, PRIORITY, 0, 6500);

int main(void)
{
	light_sensor_init();
	display_init();
	struct sensor_value lux, white;

	light_sensor_get_data(&lux, &white);
	display_update(&lux);

	k_sleep(K_SECONDS(5));
	lora_app_start_join();
	int64_t uptime = k_uptime_seconds();

	while (1) {
		// Update every 60 seconds or if the button is pressed. Limit updates
		// from button presses to every 5 seconds
		if(k_uptime_seconds() - uptime > 120 || (btn_pressed && k_uptime_seconds() - uptime > 5)){

			light_sensor_get_data(&lux, &white);
			display_update(&lux);

			uptime = k_uptime_seconds();
			// btn_pressed = false;

			if(lora_app_is_joined()){
				lora_app_add_uplink(&lux, &white);
			}
		}
		display_run();
		k_sleep(K_MSEC(10));
	}
}
