#include "co2_sensor.h"

#include <errno.h>


#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/kernel.h>

#include <zephyr/sys/__assert.h>

#include <zephyr/drivers/sensor/scd4x.h>

#define SCD4X_CALIB_PER_DEFAULT			44
#define SCD4X_INIT_CALIB_PER_DEFAULT	156

#define CO2_SAMPLE_PERIOD				15

// For SCD4X in single shot, Sensirion recommends calibration every 7 days
#define SCD4X_CALIB_PER(m) 				(((7 * 24 * 60) / m) / 12)
// For SCD4X, Sensirion recommends an initial calibration after 2 days
#define SCD4X_INIT_CALIB_PER(m) 		(((2 * 24 * 60) / m) / 12)

#define SCD43 				0
#define STCC4				1

#define CO2_SENSOR			SCD43

/*
 * Get a device structure from a devicetree node with compatible
 * "sensirion,scd4x". (If there are multiple, just pick one.)
 */
#if CO2_SENSOR == SCD43
static const struct device *const dev = DEVICE_DT_GET_ANY(sensirion_scd41);

SENSOR_DT_READ_IODEV(co2_iodev, DT_COMPAT_GET_ANY_STATUS_OKAY(sensirion_scd41),
		{SENSOR_CHAN_CO2, 0});

#elif CO2_SENSOR == STCC4
static const struct device *const dev = DEVICE_DT_GET_ANY(sensirion_stcc4);

SENSOR_DT_READ_IODEV(co2_iodev, DT_COMPAT_GET_ANY_STATUS_OKAY(sensirion_stcc4),
		{SENSOR_CHAN_CO2, 0},
		{SENSOR_CHAN_AMBIENT_TEMP, 0},
		{SENSOR_CHAN_HUMIDITY, 0});
#else
#warning "No CO2 sensor selected."
#endif

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(co2_sensor);

RTIO_DEFINE(co2_ctx, 1, 1);

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

int co2_sensor_init(void){
    int sc = check_co2_device();
	if(sc != 0){
		return sc;
	}

	const struct sensor_value calib_per = {.val1 = SCD4X_CALIB_PER(CO2_SAMPLE_PERIOD), .val2 = 0};

	// sc = sensor_attr_set(dev, SENSOR_CHAN_CO2, SENSOR_ATTR_SCD4X_SELF_CALIB_STANDARD_PERIOD, &calib_per);
	// if(sc != 0){
	// 	LOG_ERR("Failed to set ASC Period");
	// 	return sc;
	// }

	// const struct sensor_value init_calib_per = {.val1 = SCD4X_INIT_CALIB_PER(CO2_SAMPLE_PERIOD), .val2 = 0};
	// sc = sensor_attr_set(dev, SENSOR_CHAN_CO2, SENSOR_ATTR_SCD4X_SELF_CALIB_INITIAL_PERIOD, &init_calib_per);
	// if(sc != 0){
	// 	LOG_ERR("Failed to set Initial ASC Period");
	// 	return sc;
	// }

	return 0;
}

#define SINGLE_SHOT_DURATION		10000
static int64_t start_tick = 0;

int co2_sensor_read(struct sensor_value *data){
	int sc = sensor_sample_fetch(dev);
	if(sc != 0){
		return sc;
	}

	sc = sensor_channel_get(dev, SENSOR_CHAN_CO2, data);
	return sc;
}

bool co2_sensor_data_ready(void){
	if(k_uptime_get() - start_tick > SINGLE_SHOT_DURATION){
		return true;
		
	}
	else{
		return false;
	}
}

int co2_sensor_get_data(uint32_t *co2_data){
	struct sensor_value val;

	int sc = sensor_channel_get(dev, SENSOR_CHAN_CO2, &val);
	if(sc != 0){
		return sc;
	}

	*co2_data = val.val1;
	return 0;
}

int co2_sensor_set_p(struct sensor_value *pressure){
	return sensor_attr_set(dev, SENSOR_CHAN_CO2, SENSOR_ATTR_SCD4X_AMBIENT_PRESSURE, pressure);
	return 0;
}