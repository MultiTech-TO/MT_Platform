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
#include "sensors/co2_sensor.h"
#include "wireless/BLE/ble_app.h"
#include "wireless/BLE/co2_service/co2_service.h"
#include "wireless/LoRa/lora_app.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/led.h>
#include <zephyr/shell/shell.h>
#include "display/display.h"
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

void ble_thread_loop(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	ble_app_init();
	while (true) {
		k_sleep(DELAY);
		ble_app_process_action();
	}
}

K_THREAD_DEFINE(lora_thread, STACKSIZE, lora_thread_loop, NULL, NULL, NULL, PRIORITY, 0, 6500);
K_THREAD_DEFINE(ble_thread, STACKSIZE, ble_thread_loop, NULL, NULL, NULL, PRIORITY, 0, 6000);

static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));

#define LED_NODE DT_ALIAS(led1)
static const struct device *led_dev = DEVICE_DT_GET(DT_PARENT(LED_NODE));
static const uint32_t led_idx = DT_NODE_CHILD_IDX(LED_NODE);

#define MIN_PERIOD PWM_SEC(1U) / 128U
#define MAX_PERIOD PWM_SEC(1U)

int led_pwm_init()
{
	int ret;

	if (!pwm_is_ready_dt(&pwm_led0)) {
		printk("Error: PWM device %s is not ready\n", pwm_led0.dev->name);
		return 0;
	}

	pwm_set_dt(&pwm_led0, pwm_led0.period, pwm_led0.period / 100 * 75);
}
#define UART_DEVICE_NODE DT_ALIAS(sloan_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);
static bool hand_dect = false;
static char rx_buf[8];
static int rx_buf_pos;

void serial_cb(const struct device *dev, void *user_data)
{
	uint8_t c;

	uart_irq_update(uart_dev);

	if (uart_irq_rx_ready(uart_dev) <= 0) {
		return;
	}

	/* read until FIFO empty */
	while (uart_fifo_read(uart_dev, &c, 1) == 1) {
		if ((c == '\n' || c == '\r') && rx_buf_pos > 0) {
			/* terminate string */
			rx_buf[rx_buf_pos] = '\0';
			LOG_INF("%s", rx_buf);
			if(rx_buf[rx_buf_pos - 2] == '1'){
				led_on(led_dev, led_idx);

			}
			else if(rx_buf[rx_buf_pos - 2] == '0'){
				led_off(led_dev, led_idx);
			}
			/* reset the buffer (it was copied to the msgq) */
			rx_buf_pos = 0;
		} else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
			rx_buf[rx_buf_pos++] = c;
		}
		/* else: characters beyond buffer size are dropped */
	}
}

void sloan_init(void){

	if (!device_is_ready(uart_dev)) {
		printk("UART device not found!");
		return;
	}

	LOG_INF("UART INITIALIZED");
	int ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);
	uart_irq_rx_enable(uart_dev);
}



void sloan_task(void){
	// static uint8_t sloan_buf[8];
	// static uint8_t buf_idx;
	// uint8_t recv = 0;
	// if(uart_poll_in(uart_bb_dev, &recv) == 0){
	// 	if(buf_idx < (sizeof(sloan_buf) - 1)){
	// 		sloan_buf[buf_idx++] = recv;
	// 	}
	// }
	// else{
	// 	return;
	// }

	// if(recv == '\r'){
	// 	LOG_INF("%s", sloan_buf);
	// 		buf_idx = 0;

	// 	if(sloan_buf[buf_idx - 1] == 1){
	// 		hand_dect = 1;
	// 		led_on(led_dev, led_idx);
	// 	}
	// 	else{
	// 		hand_dect = 0;
	// 		led_off(led_dev, led_idx);

	// 	}
	// }
}

int main(void)
{
	led_pwm_init();
	sloan_init();
	
	display_init();
	co2_sensor_init();

	k_sleep(K_SECONDS(5));

	co2_data_t co2_data = {0};
	co2_sensor_get_data(&co2_data);
	co2_service_update(&co2_data);
	display_update(&co2_data);
	int64_t uptime = k_uptime_seconds();

	while (1) {
		// Update every 60 seconds or if the button is pressed. Limit updates
		// from button presses to every 5 seconds
		if(k_uptime_seconds() - uptime > 120 || (btn_pressed && k_uptime_seconds() - uptime > 5)){
			co2_sensor_get_data(&co2_data);
			display_update(&co2_data);
			co2_service_update(&co2_data);
			uptime = k_uptime_seconds();
			btn_pressed = false;

			if(lora_app_is_joined()){
				lora_app_add_uplink(&co2_data);
			}
		}
		sloan_task();
		display_run();
		k_sleep(K_MSEC(10));
	}
}
