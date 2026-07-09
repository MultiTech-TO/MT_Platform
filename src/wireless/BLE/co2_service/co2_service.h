#ifndef _CO2_SERVICE_
#define _CO2_SERVICE_
#include <inttypes.h>
#include <zephyr/bluetooth/bluetooth.h>
#include "../../../sensors/co2_sensor.h"

int co2_service_init(void);
int co2_service_run(void);
void co2_service_update(co2_data_t *data);
#endif