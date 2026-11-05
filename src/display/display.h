

#ifndef _DISPLAY_DRIVER_H_
#define _DISPLAY_DRIVER_H_

#include "../sensors/light_sensor.h"

int display_init(void);
int display_run(void);

int display_update(struct sensor_value *val);

#endif