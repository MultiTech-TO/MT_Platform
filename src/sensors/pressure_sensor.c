#include "pressure_sensor.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_data_types.h>

/*
 * Get a device structure from a devicetree node with compatible
 * "bosch,bme280". (If there are multiple, just pick one.)
 */
static const struct device *const dev = DEVICE_DT_GET_ANY(bosch_bme280);

SENSOR_DT_READ_IODEV(pressure_dev, DT_COMPAT_GET_ANY_STATUS_OKAY(bosch_bme280),
		{SENSOR_CHAN_AMBIENT_TEMP, 0},
		{SENSOR_CHAN_HUMIDITY, 0},
		{SENSOR_CHAN_PRESS, 0});

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pressure_sensor);
RTIO_DEFINE(pressure_ctx, 1, 1);

static const bool check_bme280_device(void)
{
	__ASSERT(dev != NULL, "No Pressure Sensor Found.");

	if (!device_is_ready(dev)) {
		printk("\nError: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.\n",
		       dev->name);
		return EBUSY;
	}

	printk("Found device \"%s\", getting sensor data\n", dev->name);
	return 0;
}

int pressure_sensor_init(void){
    check_bme280_device();
}

int pressure_sensor_read(struct sensor_value *pressure){
	// The sensor is configured to FORCED mode which makes this call blocking
	int sc = sensor_sample_fetch(dev);
	if(sc != 0){
		LOG_ERR("FETCH: %u", sc);
		return sc;
	}

	sc = sensor_channel_get(dev, SENSOR_CHAN_PRESS, pressure);
	if(sc != 0){
		LOG_ERR("GET: %u", sc);
		return sc;
	}

}