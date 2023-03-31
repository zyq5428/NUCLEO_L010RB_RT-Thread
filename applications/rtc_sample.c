#include <rtthread.h>
#include <rtdevice.h>
#include "alex.h"

#define LOG_TAG "rtc_sample"
#define LOG_LVL LOG_LVL_INFO
#include <ulog.h>

#define THREAD_PRIORITY         18
#define THREAD_STACK_SIZE       1024
#define THREAD_TIMESLICE        5

#define RTC_NAME        "rtc"

static rt_thread_t tid = RT_NULL;

#define MINUTE_TIMESLICE        20

void user_alarm_callback(rt_alarm_t alarm, time_t timestamp)
{
    static u_int8_t sec = 0;

    if ( sec < (u_int8_t)(60 / MINUTE_TIMESLICE) )
        sec++;
    else {
        /* 释放信号量 */
        rt_sem_release(rtc_sem);
        sec = 0;
    }
}

/* 线程的入口函数 */
static void rtc_entry(void *parameter)
{
    rt_err_t ret = RT_EOK;

    /* RTC相关参数  */
    time_t now;
    rt_device_t device = RT_NULL;

    /* Alarm相关参数  */
    struct rt_alarm_setup setup;
    struct rt_alarm * alarm = RT_NULL;
    struct tm p_tm;

    /*寻找设备*/
    device = rt_device_find(RTC_NAME);
    if (!device)
    {
      LOG_E("find %s failed!", RTC_NAME);
      return ;
    }
    /*初始化RTC设备*/
    if(rt_device_open(device, 0) != RT_EOK)
    {
      LOG_E("open %s failed!", RTC_NAME);
      return ;
    }
    /* 设置日期 */
    ret = set_date(2023, 1, 1);
    if (ret != RT_EOK)
    {
        LOG_E("set RTC date failed\n");
    }
    /* 设置时间 */
    ret = set_time(0, 0, 0);
    if (ret != RT_EOK)
    {
        LOG_E("set RTC time failed\n");
    }

    /* 获取当前时间戳，并把下3秒时间设置为闹钟时间 */
    now = time(RT_NULL) + 3;
    gmtime_r(&now,&p_tm);

    setup.flag = RT_ALARM_SECOND;
//    setup.flag = RT_ALARM_MINUTE;
    setup.wktime.tm_year = p_tm.tm_year;
    setup.wktime.tm_mon = p_tm.tm_mon;
    setup.wktime.tm_mday = p_tm.tm_mday;
    setup.wktime.tm_wday = p_tm.tm_wday;
    setup.wktime.tm_hour = p_tm.tm_hour;
    setup.wktime.tm_min = p_tm.tm_min;
    setup.wktime.tm_sec = p_tm.tm_sec;

    LOG_D("启动闹钟");

    alarm = rt_alarm_create(user_alarm_callback, &setup);
    if(RT_NULL != alarm)
    {
        rt_alarm_start(alarm);
    }
    else
    {
        LOG_E("rtc alarm create failed");
    }
}

/* 线程的初始化 */
int rtc_sample_start(void)
{
    /* 创建线程，名称是thread，入口是thread_entry */
    tid = rt_thread_create("rtc",
                            rtc_entry,
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
