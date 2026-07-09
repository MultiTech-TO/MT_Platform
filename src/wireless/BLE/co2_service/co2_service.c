#include "co2_service.h"

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/byteorder.h>

const struct bt_uuid *base_uuid =
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x6ae9d9ac, 0xa8fa, 0x4d08, 0xae9c, 0x8b2695f13a8a));

#define CO2_SERVICE_UUID    0x1700
#define CO2_VALUE_CHAR_UUID 0x1701
#define TP_VALUE_CHAR_UUID  0x1702
#define HUM_VALUE_CHAR_UUID	0x1703
#define CO2_CHAR_NAME       "CO2 (PPM)"
#define TP_CHAR_NAME        "Temperature (Celsius)"
#define HUM_CHAR_NAME		"Humidity (\%)"

uint16_t current_co2 = 400;
uint16_t current_temp = 24;
uint16_t current_humidity = 50;

static ssize_t read_u16(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	const uint16_t *u16 = attr->user_data;
	uint16_t value = sys_cpu_to_le16(*u16);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(value));
}

// characteristic properties callback
static ssize_t measure_interval_read_cb(struct bt_conn *conn,
					const struct bt_gatt_attr *attr,
					void *buf,
					uint16_t len,
					uint16_t offset);
static ssize_t temperture_type_read_cb(struct bt_conn *conn,
				       const struct bt_gatt_attr *attr,
				       void *buf,
				       uint16_t len,
				       uint16_t offset);

BT_GATT_SERVICE_DEFINE(co2_svc,
		       	BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_16(CO2_SERVICE_UUID)),
		       	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(CO2_VALUE_CHAR_UUID),
					      BT_GATT_CHRC_READ,
					      BT_GATT_PERM_READ,
					      read_u16,
					      NULL,
					      &current_co2),
		       	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(TP_VALUE_CHAR_UUID),
					      BT_GATT_CHRC_READ,
					      BT_GATT_PERM_READ,
					      read_u16,
					      NULL,
					      &current_temp),
				BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(HUM_VALUE_CHAR_UUID),
					      BT_GATT_CHRC_READ,
					      BT_GATT_PERM_READ,
					      read_u16,
					      NULL,
					      &current_humidity)
);

// Health Thermometer example GATT configuration for reference
// BT_GATT_SERVICE_DEFINE(hts_svc,
//                        BT_GATT_PRIMARY_SERVICE(BT_UUID_HTS),
//                        BT_GATT_CHARACTERISTIC(BT_UUID_HTS_MEASUREMENT,
//                                               BT_GATT_CHRC_INDICATE,
//                                               BT_GATT_PERM_NONE,
//                                               NULL,
//                                               NULL,
//                                               NULL),
//                        BT_GATT_CCC(temperature_measurement_ccc_changed,
//                                    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
//                        BT_GATT_CHARACTERISTIC(BT_UUID_HTS_TEMP_TYP,
//                                               BT_GATT_CHRC_READ,
//                                               BT_GATT_PERM_READ,
//                                               temperture_type_read_cb,
//                                               NULL,
//                                               &temperature_type),
//                        BT_GATT_CHARACTERISTIC(BT_UUID_HTS_TEMP_INT,
//                                               BT_GATT_CHRC_NOTIFY,
//                                               BT_GATT_PERM_NONE,
//                                               NULL,
//                                               NULL,
//                                               NULL),
//                        BT_GATT_CCC(intermediate_temperature_ccc_changed,
//                                    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
//                        BT_GATT_CHARACTERISTIC(BT_UUID_HTS_INTERVAL,
//                                               BT_GATT_CHRC_READ,
//                                               BT_GATT_PERM_READ,
//                                               measure_interval_read_cb,
//                                               NULL,
//                                               &measurement_interval),
//                        BT_GATT_CCC(measurement_interval_ccc_changed,
//                                    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
//                        );

/**************************************************************************/ /**
									      * Health thermometer
									      * indicate temperature
									      * function.
									      *****************************************************************************/


__weak void temperature_measurement_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	(void)attr;
	(void)value;
}

__weak void intermediate_temperature_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	(void)attr;
	(void)value;
}

__weak void measurement_interval_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	(void)attr;
	(void)value;
}

__weak void temperature_measurement_indicate_cb(struct bt_conn *conn,
						struct bt_gatt_indicate_params *params,
						uint8_t err)
{
	(void)conn;
	(void)params;
	(void)err;
}

__weak void temperature_measurement_indicate_destroy(struct bt_gatt_indicate_params *params)
{
	(void)params;
}

int co2_service_init(void)
{
}

int co2_service_run(void)
{
}

void co2_service_update(co2_data_t *data){
	current_co2 = data->co2.val1;
	current_temp = data->temperature.val1 * 100 + data->temperature.val2;
	current_humidity = data->humidity.val1 * 100 + data->humidity.val2;
}