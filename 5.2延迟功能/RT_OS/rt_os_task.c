#include "rt_os.h"
#include "rt_os_private.h"
#include    "debug_uart.h"
#include	"delay.h"

//��������
u8 os_task_create(void (*task)(void), u8 taskID, u8 *pstack, u8 stack_size)
{
    if (taskID >= TASK_SIZE)
        return (OS_TASK_ID_INVALID);
    if (os_tcb[taskID].OSTCBStkPtr != 0)
        return (OS_TASK_ID_EXIST);
    EA = 0;
#ifdef STACK_DETECT_MODE
{
    u8 i = 0;
    for (; i<stack_size; i++)
        pstack[i] = STACK_MAGIC;
}
#else
    stack_size = stack_size; //�������뾯��
#endif
    *pstack++ = (u16)task; //�������ĵ�ַ��λѹ���ջ��
    *pstack = (u16)task>>8; //�������ĵ�ַ��λѹ���ջ��
{
    u8 i = 0;
    for (; i < 13; i++)        //���α�������Ⱥ���popʱ��Ҫ��ն�ջ���ݣ�����ԭ�����ݵ��´���
        *(++pstack) = 0;
}
    os_tcb[taskID].OSTCBStkPtr = (u8)pstack; //���˹���ջ��ջ�������浽��ջ��������
    os_rdy_tbl |= 0x01<<taskID; //����������Ѿ�׼����
    EA = 1;
    return (OS_NO_ERR);
}

idle_user idle_user_func = NULL;
//��������
void idle_task_0(void)
{
    while (1) {
        if (idle_user_func)
            idle_user_func();
        if (os_rdy_tbl != 1)
            OS_TASK_SW ();
    }
}

#define TASK0_STACK_LEN 20
static STACK_TYPE stack[TASK0_STACK_LEN]; //����һ�� 20 �ֽڵľ�̬����ջ

void os_task0_create(void)
{
    os_task_create(idle_task_0, OS_IDLE_TASKID, stack, TASK0_STACK_LEN);
}

//ע����������û��ӿ�
void os_registered_idle_user(idle_user idle_func)
{
    idle_user_func = idle_func;
}

#ifdef STACK_DETECT_MODE
u8 get_stack_used(u8 *pstack, u8 stack_size)
{
    u8 i = stack_size-1;
    u8 unused = 0;
    while (STACK_MAGIC == pstack[i]) {
        unused++;
        if (0 == i)
            break;
        else
            i--;
    }
    return stack_size - unused;
}
#endif
