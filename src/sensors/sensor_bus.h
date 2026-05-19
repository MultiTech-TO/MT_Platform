#ifndef _SENSOR_BUS_
#define _SENSOR_BUS_

#include <stddef.h>
#include <stdint.h>

typedef enum{
    INVALID = -1,
    CO2_SENSOR = 0,
    TEMPERATURE_SENSOR,
    PRESSURE_SENSOR,
    HUMIDITY_SENSOR,
    SENSOR_TYPE_COUNT
}sensor_type_e;

typedef struct{
    void *data;
    size_t data_len;
    sensor_type_e sensor_type;
}sensor_data_t;

int sensor_bus_init(void);

#endif