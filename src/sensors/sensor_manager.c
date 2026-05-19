#include "sensor_manager.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>

#include "co2_sensor.h"
#include "pressure_sensor.h"
#include "sensor_bus.h"

LOG_MODULE_REGISTER(sensors, 3);
int sensors_init(void){
    co2_sensor_init();
    pressure_sensor_init();
}

int64_t task_tick = 0;
int64_t read_tick = 0;
bool co2_read_started = false;

enum {
    INACTIVE,
    ACTIVE
} SENSOR_STATE_e;

int sensor_run(void){
    struct sensor_value p_val = {0};
    struct sensor_value co2_val = {0};
    if(SENSOR_STATE_e == ACTIVE){
        int sc = pressure_sensor_read(&p_val);
        if(sc != 0){
            SENSOR_STATE_e = INACTIVE;
            LOG_ERR("PRESSURE READ ERR %i", sc);
            return -1;
        }
        
        k_msleep(1);
        p_val.val1 *= 10;
        p_val.val1 += p_val.val2 / 10000;
        LOG_INF("Pressure: %u.%u", p_val.val1, p_val.val2);
        sc = co2_sensor_set_p(&p_val);
        if(sc != 0){
            LOG_ERR("CO2 P SET ERR %i", sc);
            SENSOR_STATE_e = INACTIVE;
            return -1;
        }
        sc = co2_sensor_read(&co2_val);
        if(sc != 0){
            LOG_ERR("CO2 READ ERR %i", sc);
        }
        LOG_INF("CO2 %u", co2_val.val1);
        task_tick = k_uptime_get();
        SENSOR_STATE_e = INACTIVE;
    }
    else{
        while(k_uptime_get() - task_tick <= 30000){
            k_sleep(K_MSEC(200));
        }
        SENSOR_STATE_e = ACTIVE;
    }
    
}