/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date         Author        Notes
 * 2018-12-04   balanceTWK    first version
 */

#include "board.h"
#include<rtthread.h>
#include<rtdevice.h>

#ifdef BSP_USING_ONCHIP_RTC


#ifndef HAL_RTCEx_BKUPRead
#define HAL_RTCEx_BKUPRead(x1, x2) (~BKUP_REG_DATA)
#endif
#ifndef HAL_RTCEx_BKUPWrite
#define HAL_RTCEx_BKUPWrite(x1, x2, x3)
#endif
#ifndef RTC_BKP_DR1
#define RTC_BKP_DR1 RT_NULL
#endif

//#define DRV_DEBUG
#define LOG_TAG             "drv.rtc"
#include <drv_log.h>

#define BKUP_REG_DATA 0xA5A5

static struct rt_device rtc;

static RTC_HandleTypeDef RTC_Handler;

static time_t get_rtc_timestamp(void)
{
    RTC_TimeTypeDef RTC_TimeStruct = {0};
    RTC_DateTypeDef RTC_DateStruct = {0};
    struct tm tm_new;

    HAL_RTC_GetTime(&RTC_Handler, &RTC_TimeStruct, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&RTC_Handler, &RTC_DateStruct, RTC_FORMAT_BIN);

    tm_new.tm_sec  = RTC_TimeStruct.Seconds;
    tm_new.tm_min  = RTC_TimeStruct.Minutes;
    tm_new.tm_hour = RTC_TimeStruct.Hours;
    tm_new.tm_mday = RTC_DateStruct.Date;
    tm_new.tm_mon  = RTC_DateStruct.Month - 1;
    tm_new.tm_year = RTC_DateStruct.Year + 100;

    LOG_D("get rtc time.");
    return mktime(&tm_new);
}

static rt_err_t set_rtc_time_stamp(time_t time_stamp)
{
    RTC_TimeTypeDef RTC_TimeStruct = {0};
    RTC_DateTypeDef RTC_DateStruct = {0};
    struct tm *p_tm;

    p_tm = localtime(&time_stamp);
    if (p_tm->tm_year < 100)
    {
        return -RT_ERROR;
    }

    RTC_TimeStruct.Seconds = p_tm->tm_sec ;
    RTC_TimeStruct.Minutes = p_tm->tm_min ;
    RTC_TimeStruct.Hours   = p_tm->tm_hour;
    RTC_DateStruct.Date    = p_tm->tm_mday;
    RTC_DateStruct.Month   = p_tm->tm_mon + 1 ;
    RTC_DateStruct.Year    = p_tm->tm_year - 100;
    RTC_DateStruct.WeekDay = p_tm->tm_wday + 1;

    if (HAL_RTC_SetTime(&RTC_Handler, &RTC_TimeStruct, RTC_FORMAT_BIN) != HAL_OK)
    {
        return -RT_ERROR;
    }
    if (HAL_RTC_SetDate(&RTC_Handler, &RTC_DateStruct, RTC_FORMAT_BIN) != HAL_OK)
    {
        return -RT_ERROR;
    }

    LOG_D("set rtc time.");
    HAL_RTCEx_BKUPWrite(&RTC_Handler, RTC_BKP_DR1, BKUP_REG_DATA);
    return RT_EOK;
}

/* 后添加的设置闹钟的接口 2022-06-08 */
static rt_err_t set_rtc_alarm_stamp(struct rt_rtc_wkalarm wkalarm)
{
    RTC_AlarmTypeDef sAlarm = { 0 };

    if (wkalarm.enable == RT_FALSE)
    {
        if (HAL_RTC_DeactivateAlarm(&RTC_Handler, RTC_ALARM_A) != HAL_OK)
        {
            return -RT_ERROR;
        }

        LOG_D("stop rtc alarm.");
    }
    else
    {
        /* Enable the Alarm A */
        sAlarm.AlarmTime.Hours = wkalarm.tm_hour;
        sAlarm.AlarmTime.Minutes = wkalarm.tm_min;
        sAlarm.AlarmTime.Seconds = wkalarm.tm_sec;
        sAlarm.AlarmTime.SubSeconds = 0x0;
        sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
        sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
        sAlarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;   // 日期或者星期无效
        sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
        sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
        sAlarm.AlarmDateWeekDay = 1;
        sAlarm.Alarm = RTC_ALARM_A;     // 这里要和 CubeMX 配置的一致，均为 Alarm-A，修改的话两个都要修改
        if (HAL_RTC_SetAlarm_IT(&RTC_Handler, &sAlarm, RTC_FORMAT_BIN) != HAL_OK)
        {
            return -RT_ERROR;
        }

        LOG_D("set rtc alarm.");
    }

    return RT_EOK;

    /*
     * sAlarm.AlarmMask  闹钟掩码字段选择，即选择闹钟时间哪些字段无效。
     *     RTC_ALARMMASK_NONE                （全部有效）
     *     RTC_ALARMMASK_DATEWEEKDAY         （日期或星期无效）
     *     RTC_ALARMMASK_HOURS               （小时无效）
     *     RTC_ALARMMASK_MINUTES             （分钟无效）
     *     RTC_ALARMMASK_SECONDS             （秒钟无效）
     *     RTC_ALARMMASK_ALL                 （全部无效）
     *
     * sAlarm.AlarmDateWeekDaySel  闹钟日期或者星期选择。要想这个配置有效，AlarmMask 不能配置为 RTC_AlarmMask_DateWeekDay，否则会被 MASK 掉
     *     RTC_ALARMDATEWEEKDAYSEL_DATE      （日期）
     *     RTC_ALARMDATEWEEKDAYSEL_WEEKDAY   （星期）
     *
     * sAlarm.AlarmDateWeekDay  具体的日期或者日期
     *     当 AlarmDateWeekDaySel 设置成星期时，该成员 取值范围：1~7
     *     当 AlarmDateWeekDaySel 设置成日期，该成员取值范围：1~31
     */
}

/* 后添加的获取闹钟时间的接口 2022-06-08 */
struct rt_rtc_wkalarm get_rtc_alarm_stamp(void)
{
    RTC_AlarmTypeDef sAlarm = { 0 };

    struct rt_rtc_wkalarm wkalarm;

    if (HAL_RTC_GetAlarm(&RTC_Handler, &sAlarm, RTC_ALARM_A, RTC_FORMAT_BIN) != HAL_OK)
    {
        LOG_E("get rtc alarm fail!.");
    }

    wkalarm.tm_sec = sAlarm.AlarmTime.Seconds;
    wkalarm.tm_min = sAlarm.AlarmTime.Minutes;
    wkalarm.tm_hour = sAlarm.AlarmTime.Hours;

    LOG_D("get rtc alarm.");

    return wkalarm;
}

/**
  * @brief This function handles RTC global interrupt through EXTI lines 17, 19 and 20 and LSE CSS interrupt through EXTI line 19.
  */
void RTC_IRQHandler(void)
{
  /* USER CODE BEGIN RTC_IRQn 0 */

  /* USER CODE END RTC_IRQn 0 */
  HAL_RTC_AlarmIRQHandler(&RTC_Handler);
  /* USER CODE BEGIN RTC_IRQn 1 */

  /* USER CODE END RTC_IRQn 1 */
}

/* 后添加的设置闹钟中断的回调函数接口 2022-06-08 */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    rt_alarm_update(&rtc, 1);   // 该函数的两个形参没有用到，传什么都可以，作用是发送一个事件集，唤醒 alarmsvc 线程
}

static void rt_rtc_init(void)
{
#ifndef SOC_SERIES_STM32H7
    __HAL_RCC_PWR_CLK_ENABLE();
#endif

    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
#ifdef BSP_RTC_USING_LSI
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    RCC_OscInitStruct.LSEState = RCC_LSE_OFF;
    RCC_OscInitStruct.LSIState = RCC_LSI_ON;
#else
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    RCC_OscInitStruct.LSEState = RCC_LSE_ON;
    RCC_OscInitStruct.LSIState = RCC_LSI_OFF;
#endif
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
}

static rt_err_t rt_rtc_config(struct rt_device *dev)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    HAL_PWR_EnableBkUpAccess();
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
#ifdef BSP_RTC_USING_LSI
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
#else
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
#endif
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

    /* Enable RTC Clock */
    __HAL_RCC_RTC_ENABLE();

    RTC_Handler.Instance = RTC;
    if (HAL_RTCEx_BKUPRead(&RTC_Handler, RTC_BKP_DR1) != BKUP_REG_DATA)
    {
        LOG_I("RTC hasn't been configured, please use <date> command to config.");

#if defined(SOC_SERIES_STM32F1)
        RTC_Handler.Init.OutPut = RTC_OUTPUTSOURCE_NONE;
        RTC_Handler.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
#elif defined(SOC_SERIES_STM32F0)

        /* set the frequency division */
#ifdef BSP_RTC_USING_LSI
        RTC_Handler.Init.AsynchPrediv = 0XA0;
        RTC_Handler.Init.SynchPrediv = 0xFA;
#else
        RTC_Handler.Init.AsynchPrediv = 0X7F;
        RTC_Handler.Init.SynchPrediv = 0x0130;
#endif /* BSP_RTC_USING_LSI */

        RTC_Handler.Init.HourFormat = RTC_HOURFORMAT_24;
        RTC_Handler.Init.OutPut = RTC_OUTPUT_DISABLE;
        RTC_Handler.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
        RTC_Handler.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
#elif defined(SOC_SERIES_STM32L0) || defined(SOC_SERIES_STM32F2) || defined(SOC_SERIES_STM32F4) || defined(SOC_SERIES_STM32F7) || defined(SOC_SERIES_STM32L4) || defined(SOC_SERIES_STM32H7)

        /* set the frequency division */
#ifdef BSP_RTC_USING_LSI
        RTC_Handler.Init.AsynchPrediv = 0X7D;
#else
        RTC_Handler.Init.AsynchPrediv = 0X7F;
#endif /* BSP_RTC_USING_LSI */
        RTC_Handler.Init.SynchPrediv = 0XFF;

        RTC_Handler.Init.HourFormat = RTC_HOURFORMAT_24;
        RTC_Handler.Init.OutPut = RTC_OUTPUT_DISABLE;
        RTC_Handler.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
        RTC_Handler.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
#endif
        if (HAL_RTC_Init(&RTC_Handler) != HAL_OK)
        {
            return -RT_ERROR;
        }
    }
    return RT_EOK;
}

static rt_err_t rt_rtc_control(rt_device_t dev, int cmd, void *args)
{
    rt_err_t result = RT_EOK;
    RT_ASSERT(dev != RT_NULL);
    switch (cmd)
    {
    case RT_DEVICE_CTRL_RTC_GET_TIME:
        *(rt_uint32_t *)args = get_rtc_timestamp();
        LOG_D("RTC: get rtc_time %x\n", *(rt_uint32_t *)args);
        break;

    case RT_DEVICE_CTRL_RTC_SET_TIME:
        if (set_rtc_time_stamp(*(rt_uint32_t *)args))
        {
            result = -RT_ERROR;
        }
#ifdef RT_USING_ALARM
        rt_alarm_update(&rtc, 1);
#endif
        LOG_D("RTC: set rtc_time %x\n", *(rt_uint32_t *)args);
        break;
    case RT_DEVICE_CTRL_RTC_SET_ALARM:
        if (set_rtc_alarm_stamp(*(struct rt_rtc_wkalarm *) args))
        {
            result = -RT_ERROR;
        }
        LOG_D("RTC: set rtc_alarm tme : hour: %d , min: %d , sec:  %d \n", *(struct rt_rtc_wkalarm * )args->tm_hour,
                *(struct rt_rtc_wkalarm * )args->tm_min, *(struct rt_rtc_wkalarm * )args->tm_sec);
        break;

    case RT_DEVICE_CTRL_RTC_GET_ALARM:
        *(struct rt_rtc_wkalarm *) args = get_rtc_alarm_stamp();
        LOG_D("RTC: get rtc_alarm time : hour: %d , min: %d , sec:  %d \n", *(struct rt_rtc_wkalarm * )args->tm_hour,
                *(struct rt_rtc_wkalarm * )args->tm_min, *(struct rt_rtc_wkalarm * )args->tm_sec);
        break;
    }

    return result;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops rtc_ops =
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    rt_rtc_control
};
#endif

static rt_err_t rt_hw_rtc_register(rt_device_t device, const char *name, rt_uint32_t flag)
{
    RT_ASSERT(device != RT_NULL);

    rt_rtc_init();
    if (rt_rtc_config(device) != RT_EOK)
    {
        return -RT_ERROR;
    }
#ifdef RT_USING_DEVICE_OPS
    device->ops         = &rtc_ops;
#else
    device->init        = RT_NULL;
    device->open        = RT_NULL;
    device->close       = RT_NULL;
    device->read        = RT_NULL;
    device->write       = RT_NULL;
    device->control     = rt_rtc_control;
#endif
    device->type        = RT_Device_Class_RTC;
    device->rx_indicate = RT_NULL;
    device->tx_complete = RT_NULL;
    device->user_data   = RT_NULL;

    /* register a character device */
    return rt_device_register(device, name, flag);
}

int rt_hw_rtc_init(void)
{
    rt_err_t result;
    result = rt_hw_rtc_register(&rtc, "rtc", RT_DEVICE_FLAG_RDWR);
    if (result != RT_EOK)
    {
        LOG_E("rtc register err code: %d", result);
        return result;
    }
    LOG_D("rtc init success");
    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_rtc_init);

#endif /* BSP_USING_ONCHIP_RTC */
