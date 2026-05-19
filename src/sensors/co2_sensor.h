#ifndef _CO2_SENSOR_
#define _CO2_SENSOR_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/drivers/sensor.h>

int co2_sensor_init(void);
int co2_sensor_read(struct sensor_value *data);
bool co2_sensor_data_ready(void);
int co2_sensor_get_data(uint32_t *data);
int co2_sensor_set_p(struct sensor_value *pressure);

#endif