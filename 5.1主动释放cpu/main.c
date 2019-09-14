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

static STACK_TYPE app_stack[20]; //����һ�� 20 �ֽڵľ�̬����ջ
void app_task_1(void)
{
    while (1) {
        debug_print("app_task_1\r\n");
        delay_ms(1000);
        os_task_resume (0);
        //os_task_suspend (OS_TASKID_SELF);
    }
}

/******************** ������ **************************/
void main(void)
{
    RestForDownload();
    debug_uart_init();
    debug_print("STC15F2K60S2 UART1 Test Prgramme!\r\n");
    
    os_init();
    os_task_create(app_task_1, 1, app_stack, 20);
    os_start_task();
    while(1);
}

