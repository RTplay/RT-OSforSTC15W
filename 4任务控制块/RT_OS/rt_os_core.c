#include "rt_os.h"
#include "rt_os_private.h"

u8 os_rdy_tbl = 0;
u8 os_task_running_num = 0;
OS_TCB os_tcb[TASK_SIZE];

//��������
u8 os_task_create(void (*task)(void), u8 taskID, u8 *pstack, u8 stack_size)
{
    if (taskID >= TASK_SIZE)
        return (OS_TASK_NUM_INVALID);
    if (os_tcb[taskID].OSTCBStkPtr != NULL)
        return (OS_TASK_NUM_EXIST);
    
#ifdef STACK_DETECT_MODE
{
    u8 i = 0;
    for (; i<stack_size; i++)
        pstack[i] = STACK_MAGIC;
}
#else
    stack_size = stack_size; //�������뾯��
#endif
    *pstack++ = (unsigned int)task; //�������ĵ�ַ��λѹ���ջ��
    *pstack = (unsigned int)task>>8; //�������ĵ�ַ��λѹ���ջ��
    pstack += 13;
    
    os_tcb[taskID].OSTCBStkPtr = pstack; //���˹���ջ��ջ�������浽��ջ��������
    os_rdy_tbl |= 0x01<<taskID; //����������Ѿ�׼����
    
    return (OS_NO_ERR);
}

idle_user idle_user_func = NULL;
//��������
void idle_task_0(void)
{
    while (1) {
        if (idle_user_func)
            idle_user_func();
    }
}

#define TASK0_STACK_LEN 20
STACK_TYPE stack[TASK0_STACK_LEN]; //����һ�� 20 �ֽڵľ�̬����ջ
//����ϵͳ��ʼ��
void os_init(void)
{
    os_task_create(idle_task_0, 0, stack, TASK0_STACK_LEN);
}

//��ʼ�������,��������ȼ�������Ŀ�ʼ
void os_start_task(void)
{
    os_task_running_num = 0;
    SP = (u8)os_tcb[0].OSTCBStkPtr - 13;
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
