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
void RestForDownload(void)//ʹ�ø�λ����ʹ��Ƭ����������ģʽ
{
    if((PCON&0x10)==0) { //���POFλ=0
        PCON=PCON|0x10; //��POFλ��1
        IAP_CONTR=0x60; //��λ,��ISP���������
    } else {
        PCON=PCON&0xef; //��POFλ����
    }
}

static STACK_TYPE app1_stack[APP_STACK_SIZE]; //����һ�� 30 �ֽڵľ�̬����ջ
void app_task_1(void)
{
    while (1) {
        os_tick_sleep(1);
        //try_to_get_data(RTWingProcess);
    }
}

/******************** ������ **************************/
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

