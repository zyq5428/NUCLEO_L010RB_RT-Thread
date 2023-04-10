#include <rtthread.h>
#include <rtdevice.h>
#include "gy30.h"
#include "alex.h"

#define LOG_TAG "gy30_sample"
#define LOG_LVL LOG_LVL_INFO
#include <ulog.h>

#define THREAD_PRIORITY         21
#define THREAD_STACK_SIZE       768
#define THREAD_TIMESLICE        10

static rt_thread_t tid = RT_NULL;

#define GY30_I2C_BUS_NAME          "i2c2"  /* 传感器连接的I2C总线设备名称 */
#define GY30_DATA_NAX              10  /* 读取数组的最大数据个数 */

/* 线程的入口函数 */
static void gy30_entry(void *parameter)
{
    rt_uint32_t recv;
    rt_err_t result = RT_EOK;
    rt_uint8_t cnt = 0;
    uint16_t lux = 0;

    static Gy30Data_t sensor_data[GY30_DATA_NAX];
    MsgData_t msg = {
            .from       = TASK_GY30,
            .to         = TASK_OLED,
            .data_ptr   = RT_NULL,
            .data_size  = sizeof(sensor_data)
    };

    GY30_Init(GY30_I2C_BUS_NAME);

    while (1)
    {
        if ( cnt == GY30_DATA_NAX )
            cnt = 0;

        /* 永久方式等待信号量 */
        result = rt_event_recv(rtc_event, (EVENT_FLAG3 | EVENT_FLAG5),
                RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                RT_WAITING_FOREVER, &recv);
        if (result != RT_EOK)
        {
            LOG_D("take a semaphore, failed.");
        } else {
            /* 测量光照值 */
            lux = GY30_Measurement();
            LOG_D("lux is : %d", lux);

            sensor_data[cnt].lux = lux;
            msg.data_ptr = (void *)(&sensor_data[cnt]);

            /* 发送这个消息指针给 mq 消息队列 */
            result = rt_mq_send(&mq, (void*)&msg, sizeof(MsgData_t));
            if (result != RT_EOK)
            {
                LOG_E("rt_mq_send ERR.");
                rt_thread_mdelay(500);
            }

            cnt++;
        }
    }
}

/* 线程的初始化 */
int gy30_sample_start(void)
{
    /* 创建线程，名称是thread，入口是thread_entry */
    tid = rt_thread_create("gy30",
                            gy30_entry,
                            RT_NULL,
                            THREAD_STACK_SIZE,
                            THREAD_PRIORITY,
                            THREAD_TIMESLICE);

    /* 如果获得线程控制块，启动这个线程  */
    if (tid != RT_NULL)
        rt_thread_startup(tid);
    else
        LOG_E("thread [%s] create failed", LOG_TAG);

    return RT_EOK;
}
