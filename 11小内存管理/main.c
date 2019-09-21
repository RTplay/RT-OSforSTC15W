#include    <stdio.h>
#include	"config.h"
#include    "rt_os.h"
#include	"delay.h"
#include    "debug_uart.h"

typedef struct msg_type {
    u8 type;
    void *user_data;
}MSG_TYPE;


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
#ifdef SYSTEM_DETECT_MODE
    OS_SS *ss = get_sys_statistics();
#endif
    printf("app_task_1 in\r\n");
    while (1) {
#ifdef SYSTEM_DETECT_MODE
        u8 i;
        printf("ID\tCPU\tSTATUS\tUSED STACK\r\n");
        for (i=0; i<TASK_SIZE; i++) {
            if (ss[i].OSSSStatus != OS_STAT_DEFAULT) {//任务已
                printf("%bu\t%bu%%\t", i, ss[i].OSSSCyclesTot);
                switch (ss[i].OSSSStatus)
                {
                    case OS_STAT_RUNNING:
                        printf("RUN");
                    break;
                    case OS_STAT_RDY:
                        printf("RDY");
                    break;
                    case OS_STAT_SLEEP:
                        printf("SLEEP");
                    break;
                    case OS_STAT_SUSPEND:
                        printf("SUSPEND");
                    break;
                    case OS_STAT_DEAD:
                        printf("DEAD");
                    break;
                    case OS_STAT_MUTEX:
                        printf("MUTEX");
                    break;
                    case OS_STAT_SEM:
                        printf("SEM");
                    break;
                    case OS_STAT_MSGQ:
                        printf("MSGQ");
                    break;
                    case OS_STAT_FLAG:
                        printf("FLAG");
                    break;
                    default:
                        printf("unknown");
                }
                printf("\t%bu\r\n", ss[i].OSSSMaxUsedStk);
            }
        }
#else
        debug_print("app_task_1\r\n");
#endif
        os_tick_sleep(100);
    }
}

/******************** 主函数 **************************/
void main(void)
{
    u8 *a, *b, *c, *d;
    RestForDownload();
    debug_uart_init();
    //printf("STC15F2K60S2 RT-OS Test Prgramme!\r\n");
    os_init();
    a = os_malloc(4);
    *a = 7;
    b = os_malloc(8);
    *b = 8;
    c = os_malloc(4);
    *c = 8;
    d = os_malloc(4);
    os_memset(d, 9, 4);
    os_free(b);
    d = os_realloc(d, 6);
    os_free(a);
    os_free(c);
    os_free(d);
    os_task_create(app_task_1, 1, 20, OS_DEFAULT_TIME_QUANTA, app1_stack, APP_STACK_SIZE);
    os_start_task();
    while(1);
}

