#include "co2_sensor.h"

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/kernel.h>

#include <zephyr/sys/__assert.h>

#include <zephyr/drivers/sensor/scd4x.h>


#define SCD4X_CALIB_PER_DEFAULT      44
#define SCD4X_INIT_CALIB_PER_DEFAULT 156

#define CO2_SAMPLE_PERIOD 15

// For SCD4X in single shot, Sensirion recommends calibration every 7 days
#define SCD4X_CALIB_PER(m)      (((7 * 24 * 60) / m) / 12)
// For SCD4X, Sensirion recommends an initial calibration after 2 days
#define SCD4X_INIT_CALIB_PER(m) (((2 * 24 * 60) / m) / 12)

#define CO2_SENSOR SCD43
#define CO2_NODE DT_NODELABEL(co2_sensor)


// Example of supporting multiple sensors with the same (similar) API. This 
// will load in the relevant code based on which CO2 sensor (SCD41, STCC4)
// is defined in the devicetree.
// If in the device tree co2_sensor does not exist you will get a warning
// and the values will be fixed

#if DT_NODE_EXISTS(CO2_NODE)
#if DT_NODE_HAS_COMPAT_STATUS(CO2_NODE, sensirion_scd41, okay)
static const struct device *const dev = DEVICE_DT_GET_ANY(sensirion_scd41);

SENSOR_DT_READ_IODEV(co2_iodev, DT_COMPAT_GET_ANY_STATUS_OKAY(sensirion_scd41),
		     {SENSOR_CHAN_AMBIENT_TEMP, 0}, {SENSOR_CHAN_HUMIDITY, 0},
		     {SENSOR_CHAN_CO2, 0});
#elif DT_NODE_HAS_COMPAT_STATUS(CO2_NODE, sensirion_stcc4, okay)
static const struct device *const dev = DEVICE_DT_GET_ANY(sensirion_stcc4);

SENSOR_DT_READ_IODEV(co2_iodev, DT_COMPAT_GET_ANY_STATUS_OKAY(sensirion_stcc4),
		     {SENSOR_CHAN_CO2, 0}, {SENSOR_CHAN_AMBIENT_TEMP, 0},
		     {SENSOR_CHAN_HUMIDITY, 0});
#endif
#else
#warning "No CO2 sensor selected. Emulating sensor"

#define CO2_EMULATION		1
#endif



#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(co2_sensor);

RTIO_DEFINE(co2_ctx, 1, 1);
#ifndef CO2_EMULATION
static const bool check_co2_device(void)
{
	__ASSERT(dev != NULL, "No CO2 Sensor Found.");

	if (!device_is_ready(dev)) {
		printk("\nError: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.\n",
		       dev->name);
		return EBUSY;
	}

	printk("Found device \"%s\", getting sensor data\n", dev->name);
	return 0;
}

int co2_sensor_init(void)
{
	int sc = check_co2_device();
	if (sc != 0) {
		return sc;
	}

	struct sensor_value pressure = {.val1 = 0x03db, .val2 = 0};
	co2_sensor_set_p(&pressure);
	return 0;
}

#define SINGLE_SHOT_DURATION 10000
static int64_t start_tick = 0;

bool co2_sensor_data_ready(void)
{
	if (k_uptime_get() - start_tick > SINGLE_SHOT_DURATION) {
		return true;

	} else {
		return false;
	}
}

int co2_sensor_get_data(co2_data_t *co2_data)
{
	struct sensor_value val;

	int sc = sensor_sample_fetch(dev);
	if (sc != 0) {
		LOG_ERR("FAILED TO FETCH SAMPLES");
		return sc;
	}

	sc = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &co2_data->temperature);
	if (sc != 0) {
		LOG_ERR("FAILED TO READ TEMPERATURE");
		return sc;
	}
	// Hacky way to truncate the decimal part of the sensor reading
	// There's probably a better way to do this
	while (co2_data->temperature.val2 > 100) {
		co2_data->temperature.val2 /= 10;
	}

	sc = sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &co2_data->humidity);
	if (sc != 0) {
		LOG_ERR("FAILED TO READ HUMIDITY");
		return sc;
	}
	while (co2_data->humidity.val2 > 100) {
		co2_data->humidity.val2 /= 10;
	}

	sc = sensor_channel_get(dev, SENSOR_CHAN_CO2, &co2_data->co2);
	if (sc != 0) {
		LOG_ERR("FAILED TO READ CO2");
		return sc;
	}

	LOG_INF("CO2: %d, TEMPERATURE: %d.%d, HUMIDITY: %d.%d", co2_data->co2.val1,
		co2_data->temperature.val1, co2_data->temperature.val2, co2_data->humidity.val1,
		co2_data->humidity.val2);

	return 0;
}

int co2_sensor_set_p(struct sensor_value *pressure)
{
	return sensor_attr_set(dev, SENSOR_CHAN_CO2, SENSOR_ATTR_SCD4X_AMBIENT_PRESSURE, pressure);
	return 0;
}
#else
int co2_sensor_init(void)
{
return 0;
}

bool co2_sensor_data_ready(void)
{
return true;
}

int co2_sensor_get_data(co2_data_t *co2_data)
{
	struct sensor_value val;
	co2_data->temperature.val1 = 24;
	co2_data->temperature.val2 = 0;
	co2_data->humidity.val1 = 50;
	co2_data->humidity.val2 = 0;
	co2_data->co2.val1 = 400;
	co2_data->co2.val2 = 0;

	LOG_INF("CO2: %d, TEMPERATURE: %d.%d, HUMIDITY: %d.%d", co2_data->co2.val1,
		co2_data->temperature.val1, co2_data->temperature.val2, co2_data->humidity.val1,
		co2_data->humidity.val2);

	return 0;
}

int co2_sensor_set_p(struct sensor_value *pressure)
{
	return 0;
}
#endif