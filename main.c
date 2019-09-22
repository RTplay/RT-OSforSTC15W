#include    <stdio.h>
#include	"config.h"
#include    "rt_os.h"
#include	"delay.h"
#include    "debug_uart.h"
//#include    "RTWing.h"

typedef struct msg_type {
    u8 type;
    void *user_data;
} MSG_TYPE;


#define APP_STACK_SIZE  30
void RestForDownload(void)//使用复位功能使单片机进入下载模式
{
    if((PCON&0x10)==0) { //如果POF位=0
        PCON=PCON|0x10; //将POF位置1
        IAP_CONTR=0x60; //软复位,从ISP监控区启动
    } else {
        PCON=PCON&0xef; //将POF位清零
    }
}

static STACK_TYPE app1_stack[APP_STACK_SIZE]; //建立一个 30 字节的静态区堆栈
void app_task_1(void)
{
    while (1) {
        os_tick_sleep(1);
        //try_to_get_data(RTWingProcess);
    }
}

/******************** 主函数 **************************/
void main(void)
{
    RestForDownload();
    debug_uart_init();
    //InitRTWing();
    debug_print("STC15F2K60S2 RT-OS Test Prgramme!\r\n");
    //PrintRTWing();
    os_init();
    os_task_create(app_task_1, 1, 20, OS_DEFAULT_TIME_QUANTA, app1_stack, APP_STACK_SIZE);
    os_start_task();
    while(1);
}

