#ifndef _LORA_APP_
#define _LORA_APP_

#include <stdbool.h>
#include <stdint.h>

#include "../../sensors/co2_sensor.h"

int lora_app_init(void);
void lora_app_run(void);
bool lora_app_is_joined(void);
void lora_app_add_uplink(co2_data_t *co2_data);

#endif