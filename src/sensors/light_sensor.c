#include "light_sensor.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/drivers/sensor/veml7700.h>
#include <zephyr/kernel.h>

#include <zephyr/sys/__assert.h>

#define LIGHT_SENSOR VEML7700
#define LIGHT_NODE   DT_NODELABEL(light_sensor)

#if DT_NODE_HAS_COMPAT_STATUS(LIGHT_NODE, vishay_veml7700, okay)
static const struct device *const dev = DEVICE_DT_GET_ANY(vishay_veml7700);

SENSOR_DT_READ_IODEV(light_iodev, DT_COMPAT_GET_ANY_STATUS_OKAY(vishay_veml7700),
		     {SENSOR_CHAN_LIGHT, 0}, {SENSOR_CHAN_VEML7700_WHITE_RAW_COUNTS, 0});
#endif

#define LOG_LEVEL 1
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(light_sensor);

RTIO_DEFINE(light_ctx, 1, 1);

int light_sensor_init(void)
{
	__ASSERT(dev != NULL, "No Light Sensor Found.");

	if (!device_is_ready(dev)) {
		printk("\nError: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.\n",
		       dev->name);
		return EBUSY;
	}
}

int light_sensor_get_data(struct sensor_value *lux, struct sensor_value *white)
{
	sensor_sample_fetch(dev);
	sensor_channel_get(dev, SENSOR_CHAN_LIGHT, lux);
	sensor_channel_get(dev, SENSOR_CHAN_VEML7700_WHITE_RAW_COUNTS, white);
	// LOG_PRINTK("LUX: %d, WHITE: %d \n", lux->val1, white->val1);
}
