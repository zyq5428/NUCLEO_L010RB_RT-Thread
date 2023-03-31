#include <rtthread.h>
#include <rtdevice.h>
#include "ssd1306.h"
#include "alex.h"
#include "type_to_ascii.h"

#define LOG_TAG "oled_sample"
#define LOG_LVL LOG_LVL_INFO
#include <ulog.h>

#define THREAD_PRIORITY         23
#define THREAD_STACK_SIZE       1024
#define THREAD_TIMESLICE        20

static rt_thread_t tid = RT_NULL;

#define SSD1306_I2C_BUS_NAME          "i2c1"  /* 传感器连接的I2C总线设备名称 */

static time_t now;
static char lux_menu[]       = "Lux :     ";
static char sec_clear[]           = "   ";

#define CLEAR_POSITION              8

static void display_menu(void)
{
    ssd1306_Fill(Black);
    /* 获取时间 */
    now = time(RT_NULL);
    ssd1306_SetCursor(8, 0);
    ssd1306_WriteString(ctime(&now)+8, Font_7x10, White);
    ssd1306_SetCursor(8+7*CLEAR_POSITION, 0);
    ssd1306_WriteString(sec_clear, Font_7x10, White);
    ssd1306_SetCursor(28, 40);
    ssd1306_WriteString(lux_menu, Font_7x10, White);
    ssd1306_UpdateScreen();
}

/* 线程的入口函数 */
static void oled_entry(void *parameter)
{
    MsgData_t msg;
    Gy30Data_t *sensor_data_ptr;
    uint16_t lux = 0;
    unsigned char size;

    ssd1306_Init(SSD1306_I2C_BUS_NAME);
    display_menu();

    while (1)
    {
        /* 从消息队列中接收消息到 msg 中 */
        if (rt_mq_recv(&mq, (void*)&msg, sizeof(MsgData_t), RT_WAITING_FOREVER) == RT_EOK)
        {
            /* 成功接收到消息，进行相应的数据处理 */
            sensor_data_ptr = msg.data_ptr;
            lux = sensor_data_ptr->lux;

            /* 将数据转换成字符 */
            int_to_ascii( (unsigned int)lux, &lux_menu[6], &size, 4, ' ' );

            /* 获取时间 */
            now = time(RT_NULL);
            LOG_D("%s", ctime(&now));
            ssd1306_SetCursor(8, 0);
            ssd1306_WriteString(ctime(&now)+8, Font_7x10, White);
            ssd1306_SetCursor(8+7*CLEAR_POSITION, 0);
            ssd1306_WriteString(sec_clear, Font_7x10, White);
            ssd1306_SetCursor(28, 40);
            ssd1306_WriteString(lux_menu, Font_7x10, White);
            ssd1306_UpdateScreen();
        } else {
            LOG_E("recv a message failed.");
        }
    }
}

/* 线程的初始化 */
int oled_sample_start(void)
{
    /* 创建线程，名称是thread，入口是thread_entry */
    tid = rt_thread_create("oled",
                            oled_entry,
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
