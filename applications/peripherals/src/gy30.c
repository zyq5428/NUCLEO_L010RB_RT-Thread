#include <rtthread.h>
#include <rtdevice.h>
#include "gy30.h"

#define LOG_TAG "gy30"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>

static struct rt_i2c_bus_device *i2c_bus = RT_NULL;     /* I2C总线设备句柄 */
static rt_bool_t initialized = RT_FALSE;                /* 传感器初始化状态 */

/* Write command */
void GY30_WriteCommand(uint8_t command)
{
    rt_i2c_master_send(i2c_bus, GY30_I2C_ADDRESS, RT_I2C_WR, &command, 1);
}

/* Continuous measurement mode  */
uint8_t GY30_ContinuouslyMode(uint8_t *data, uint8_t command)
{
    rt_i2c_master_send(i2c_bus, GY30_I2C_ADDRESS, RT_I2C_WR, &command, 1);
    rt_thread_mdelay(180); /* H-resolution mode measurement.( max. 180ms. ) */
    rt_i2c_master_recv(i2c_bus, GY30_I2C_ADDRESS, RT_I2C_RD, data, 2);
    return RT_EOK;
}

/* One time measurement mode  */
uint8_t GY30_OnetimeMode(uint8_t *data, uint8_t command)
{
    rt_i2c_master_send(i2c_bus, GY30_I2C_ADDRESS, RT_I2C_WR, &command, 1);
    rt_thread_mdelay(24);  /* L-resolution mode measurement.( max. 24ms. ) */
    rt_i2c_master_recv(i2c_bus, GY30_I2C_ADDRESS, RT_I2C_RD, data, 2);
    return RT_EOK;
}

/* GY-30 Measurement */
uint16_t GY30_Measurement(void)
{
    uint8_t data[2] = {0x00, 0x00};
    uint16_t lux;
    if(RT_EOK == GY30_ContinuouslyMode(data, GY30_Continuously_H_mode))
            lux = (uint16_t)( ((data[0] << 8) + data[1]) / 1.2 );   /* Limited minimum precision 1 lux */
    else {
        LOG_E("Measurement Fail\r\n");
        return 0xFFFF;  /* The measurement result cannot be greater than 0xFFFF (result/1.2) */
    }

    return lux;
}

/* GY-30 Init */
uint8_t GY30_Init(const char *name)
{
    i2c_bus = (struct rt_i2c_bus_device *)rt_device_find(name);
    if (i2c_bus == RT_NULL)
    {
        LOG_E("can not find %s device", name);
        return RT_ERROR;
    }
    /* A little delay */
    rt_thread_mdelay(100);

    /* Init GY-30 */
    GY30_WriteCommand(GY30_Power_On);

    initialized = RT_TRUE;

    /* Return OK */
    return RT_EOK;
}
