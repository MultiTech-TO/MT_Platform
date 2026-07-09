#ifndef _CO2_SENSOR_
#define _CO2_SENSOR_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/drivers/sensor.h>

typedef struct{
    struct sensor_value co2;
    struct sensor_value temperature;
    struct sensor_value humidity;
} co2_data_t;

int co2_sensor_init(void);
bool co2_sensor_data_ready(void);
int co2_sensor_get_data(co2_data_t *data);
int co2_sensor_set_p(struct sensor_value *pressure);

#endif