#ifndef _PRESSURE_SENSOR_

#define _PRESSURE_SENSOR_

#include <stdint.h>

#include <zephyr/drivers/sensor.h>

int pressure_sensor_init(void);
int pressure_sensor_read(struct sensor_value *pressure);

#endif