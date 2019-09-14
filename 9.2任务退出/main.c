#include	"config.h"
#include    "rt_os.h"
#include	"delay.h"
#include    "debug_uart.h"

void RestForDownload(void)//ʹ�ø�λ����ʹ��Ƭ����������ģʽ
{
    if((PCON&0x10)==0) { //���POFλ=0
        PCON=PCON|0x10; //��POFλ��1
        IAP_CONTR=0x60; //��λ,��ISP���������
    } else {
        PCON=PCON&0xef; //��POFλ����
    }
}

static STACK_TYPE app3_stack[25]; //����һ�� 25 �ֽڵľ�̬����ջ
void app_task_3(void)
{
    while (1) {
        debug_print("app_task_3\r\n");
        os_tick_sleep(100);
    }
}

static STACK_TYPE app1_stack[25]; //����һ�� 25 �ֽڵľ�̬����ջ
void app_task_1(void)
{
    u8 cnt = 0;
    while (1) {
        if (cnt++ == 3)
            os_task_exit();
        debug_print("app_task_1\r\n");
        os_tick_sleep(100);
    }
}

static STACK_TYPE app2_stack[25]; //����һ�� 25 �ֽڵľ�̬����ջ
void app_task_2(void)
{
    os_task_create(app_task_3, 3, 15, OS_DEFAULT_TIME_QUANTA, app3_stack, 25);
    while (1) {
        debug_print("app_task_2\r\n");
        os_tick_sleep(100);
    }
}



/******************** ������ **************************/
void main(void)
{
    RestForDownload();
    debug_uart_init();
    debug_print("STC15F2K60S2 RT-OS Test Prgramme!\r\n");
    os_init();
    os_task_create(app_task_1, 1, 20, OS_DEFAULT_TIME_QUANTA, app1_stack, 25);
    os_task_create(app_task_2, 2, 10, OS_DEFAULT_TIME_QUANTA, app2_stack, 25);
    os_start_task();
    while(1);
}

