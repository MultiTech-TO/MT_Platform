#ifndef _LORA_APP_
#define _LORA_APP_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/sensing/sensing_datatypes.h>
#include "../../sensors/light_sensor.h"

int lora_app_init(void);
void lora_app_run(void);
void lora_app_start_join(void);
bool lora_app_is_joined(void);
void lora_app_add_uplink(struct sensor_value *lux, struct sensor_value *white);

#endif