#include    <stdio.h>
#include	"config.h"
#include    "rt_os.h"
#include	"delay.h"
#include    "debug_uart.h"

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

static STACK_TYPE app3_stack[APP_STACK_SIZE]; //建立一个 30 字节的静态区堆栈
void app_task_3(void)
{
    while (1) {
        printf("app_task_3\r\n");
        delay_ms(20);
        os_tick_sleep(100);
    }
}

static STACK_TYPE app1_stack[APP_STACK_SIZE]; //建立一个 30 字节的静态区堆栈
void app_task_1(void)
{
    while (1) {
        printf("app_task_1\r\n");
        delay_ms(30);
        os_tick_sleep(100);
    }
}

static STACK_TYPE app2_stack[APP_STACK_SIZE]; //建立一个 30 字节的静态区堆栈
void app_task_2(void)
{
#ifdef SYSTEM_DETECT_MODE
    OS_SS *ss = get_sys_statistics();
#endif
    while (1) {
#ifdef SYSTEM_DETECT_MODE
        u8 i;
        printf("ID\tCPU\tSTATUS\tUSED STACK\r\n");
        for (i=0; i<TASK_SIZE; i++) {
            if (ss[i].OSSSStatus != OS_STAT_DEFAULT) {//任务已启动
                printf("%bu\t%bu%%\t%bu\t%bu\r\n", i, ss[i].OSSSCyclesTot, ss[i].OSSSStatus, ss[i].OSSSMaxUsedStk);
            }
        }
#else
        debug_print("app_task_2\r\n");
#endif
        os_tick_sleep(100);
    }
}



/******************** 主函数 **************************/
void main(void)
{
    RestForDownload();
    debug_uart_init();
    debug_print("STC15F2K60S2 RT-OS Test Prgramme!\r\n");
    os_init();
    os_task_create(app_task_1, 1, 20, OS_DEFAULT_TIME_QUANTA, app1_stack, APP_STACK_SIZE);
    os_task_create(app_task_2, 2, 10, OS_DEFAULT_TIME_QUANTA, app2_stack, APP_STACK_SIZE);
    os_task_create(app_task_3, 3, 15, OS_DEFAULT_TIME_QUANTA, app3_stack, APP_STACK_SIZE);
    os_start_task();
    while(1);
}

