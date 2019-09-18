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
    printf("app_task_1 in\r\n");
    while (1) {
        u8 ret;
        OS_FLAGS flag_1;
        printf("app_task_1 os_flag_pend...\r\n");
        flag_1 = os_flag_pend(0, 250, &ret);
        if (ret == OS_ERR_NONE) {
            printf("app_task_1 os_flag_pend ok\r\n");
        }
        else if (ret == OS_ERR_TIMEOUT) {
            printf("app_task_1 os_msgq_pend timeout\r\n");
        }
        else if (ret == OS_ERR_PEND_ABORT) {
            printf("app_task_1 os_msgq_pend abort\r\n");
        }
    }
}

#define EVENT_1 0x01
#define EVENT_2 0x02
#define EVENT_3 0x04

static STACK_TYPE app2_stack[APP_STACK_SIZE]; //建立一个 30 字节的静态区堆栈
void app_task_2(void)
{
    printf("app_task_2 in\r\n");
    while (1) {
        u8 err;
        os_tick_sleep(150);
        printf("app_task_2 os_flag_post EVENT_2\r\n");
        os_flag_post(0, EVENT_2, OS_FLAG_SET, &err);
    }
}

static STACK_TYPE app3_stack[APP_STACK_SIZE]; //建立一个 30 字节的静态区堆栈
void app_task_3(void)
{
    printf("app_task_3 in\r\n");
    while (1) {
        u8 err;
        os_tick_sleep(150);
        printf("app_task_3 os_flag_post EVENT_1\r\n");
        os_flag_post(0, EVENT_1, OS_FLAG_SET, &err);
    }
}

static STACK_TYPE app4_stack[APP_STACK_SIZE]; //建立一个 30 字节的静态区堆栈
void app_task_4(void)
{
#ifdef SYSTEM_DETECT_MODE
    OS_SS *ss = get_sys_statistics();
#endif
    printf("app_task_4 in\r\n");
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
        debug_print("app_task_4\r\n");
#endif
        os_tick_sleep(100);
    }
}

/******************** 主函数 **************************/
void main(void)
{
    RestForDownload();
    debug_uart_init();
    printf("STC15F2K60S2 RT-OS Test Prgramme!\r\n");
    os_flag_init(0, 0, EVENT_1+EVENT_2, OS_FLAG_WAIT_SET_ALL+OS_FLAG_CONSUME);
    os_init();
    os_task_create(app_task_1, 1, 20, OS_DEFAULT_TIME_QUANTA, app1_stack, APP_STACK_SIZE);
    os_task_create(app_task_2, 2, 15, OS_DEFAULT_TIME_QUANTA, app2_stack, APP_STACK_SIZE);
    os_task_create(app_task_3, 3, 10, OS_DEFAULT_TIME_QUANTA, app3_stack, APP_STACK_SIZE);
    os_task_create(app_task_4, 4,  5, OS_DEFAULT_TIME_QUANTA, app4_stack, APP_STACK_SIZE);
    os_start_task();
    while(1);
}

