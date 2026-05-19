#ifndef _CO2_SERVICE_
#define _CO2_SERVICE_
#include <inttypes.h>
#include <zephyr/bluetooth/bluetooth.h>

typedef struct{
    uint16_t co2_ppm;
    uint16_t temperature;
    uint16_t humidity;
    uint16_t pressure;
} CO2_SENSOR_t;

int co2_service_init(void);
int co2_service_run(void);
void co2_service_set_period(uint16_t minutes);
void co2_service_start(void);
void co2_service_stop(void);
void co2_service_indicate_co2(struct bt_conn *connection, uint16_t co2);
void co2_service_indicate_tp(struct bt_conn *connection, uint16_t tp);

#endif