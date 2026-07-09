#include "display.h"


#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display);

#define DISPLAY_NODE		DT_CHOSEN(zephyr_display)
const struct device *const display_dev = DEVICE_DT_GET(DISPLAY_NODE);

static uint32_t count;
static lv_obj_t * table;

int display_init(void){
	int ret;

	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return 0;
	}

	lv_obj_t * screen = lv_screen_active();

    table = lv_table_create(screen);
	lv_obj_set_style_text_font(table, &lv_font_montserrat_24, 0);
	lv_table_set_col_width(table, 0, 200);
	lv_table_set_col_width(table, 1, 200);
    lv_table_set_row_count(table, 3);
    lv_table_set_column_count(table, 2);
    lv_obj_set_align(table, LV_ALIGN_CENTER);
    lv_table_set_cell_value(table, 0, 0, "CO2");
    lv_table_set_cell_value(table, 0, 1, "");
    lv_table_set_cell_value(table, 1, 0, "Temperature");
    lv_table_set_cell_value(table, 1, 1, "");
    lv_table_set_cell_value(table, 2, 0, "Humidity");
    lv_table_set_cell_value(table, 2, 1, "");

	lv_timer_handler();
	ret = display_blanking_off(display_dev);

}

int display_run(void){
	lv_timer_handler();
}

int display_update(co2_data_t *co2_data){
	uint8_t buf[32] = {0};
	
	snprintk(buf, sizeof(buf), "%d ppm", co2_data->co2.val1);
	lv_table_set_cell_value(table, 0, 1, buf);

	snprintk(buf, sizeof(buf), "%d.%2d °C", co2_data->temperature.val1, co2_data->temperature.val2);
	lv_table_set_cell_value(table, 1, 1, buf);

	snprintk(buf, sizeof(buf), "%d.%2d \%", co2_data->humidity.val1, co2_data->humidity.val2);
	lv_table_set_cell_value(table, 2, 1, buf);
}



