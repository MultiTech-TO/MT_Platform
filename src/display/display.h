

#ifndef _DISPLAY_DRIVER_H_
#define _DISPLAY_DRIVER_H_

#include "../sensors/co2_sensor.h"

int display_init(void);
int display_run(void);

int display_update(co2_data_t *co2_data);

#endif