#include "sensor_bus.h"

#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(sensor_bus, LOG_LEVEL_DBG);

ZBUS_CHAN_DEFINE(sensor_data,
		 sensor_data_t,
		 NULL,
		 NULL,
		 ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(.data = NULL, .data_len = 0, .sensor_type = INVALID));

int sensor_bus_init(void)
{
}
