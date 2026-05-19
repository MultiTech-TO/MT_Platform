#ifndef _LORA_APP_
#define _LORA_APP_

#include <stdbool.h>
#include <stdint.h>

int lora_app_init(void);
void lora_app_run(void);
bool lora_app_is_joined(void);
void lora_app_add_uplink(uint8_t *data, uint8_t len);

#endif