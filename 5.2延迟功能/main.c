#include	"config.h"
#include    "rt_os.h"
#include	"delay.h"
#include    "debug_uart.h"

void RestForDownload(void)//使用复位功能使单片机进入下载模式
{
    if((PCON&0x10)==0) { //如果POF位=0
        PCON=PCON|0x10; //将POF位置1
        IAP_CONTR=0x60; //软复位,从ISP监控区启动
    } else {
        PCON=PCON&0xef; //将POF位清零
    }
}

static STACK_TYPE app1_stack[20]; //建立一个 20 字节的静态区堆栈
void app_task_1(void)
{
    while (1) {
        debug_print("app_task_1\r\n");
        os_tick_sleep(100);
    }
}

static STACK_TYPE app2_stack[20]; //建立一个 20 字节的静态区堆栈
void app_task_2(void)
{
    while (1) {
        debug_print("app_task_2\r\n");
        os_tick_sleep(200);
    }
}

/******************** 主函数 **************************/
void main(void)
{
    RestForDownload();
    debug_uart_init();
    debug_print("STC15F2K60S2 UART1 Test Prgramme!\r\n");
    
    os_init();
    os_task_create(app_task_1, 1, app1_stack, 20);
    os_task_create(app_task_2, 2, app2_stack, 20);
    os_start_task();
    while(1);
}

