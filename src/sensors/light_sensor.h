#ifndef _LIGHT_SENSOR_
#define _LIGHT_SENSOR_

#include <zephyr/drivers/sensor.h>

int light_sensor_init(void);
int light_sensor_get_data(struct sensor_value *lux, struct sensor_value *white);

#endif