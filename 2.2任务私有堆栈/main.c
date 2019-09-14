#include	"config.h"
#include    "RT-OS_type.h"
#include	"delay.h"

#define TASK0_STACK_LEN 20
STACK_TYPE stack[TASK0_STACK_LEN]; //����һ�� 20 �ֽڵľ�̬����ջ

void start_task_with_stack(void (*pfun)(), u8 *pstack, u8 stack_len)
{
#ifdef STACK_DETECT_MODE
    u8 i = 0;
    for (; i<stack_len; i++)
        pstack[i] = STACK_MAGIC;
#else
    stack_len = stack_len; //�������뾯��
#endif
    *pstack++ = (unsigned int)pfun; //�������ĵ�ַ��λѹ���ջ��
    *pstack = (unsigned int)pfun>>8; //�������ĵ�ַ��λѹ���ջ��
    SP = (u8)pstack; //����ջָ��ָ���˹���ջ��ջ��
}

#ifdef STACK_DETECT_MODE
u8 get_stack_used(u8 *pstack, u8 stack_len)
{
    u8 i = stack_len-1;
    u8 unused = 0;
    while (STACK_MAGIC == pstack[i]) {
        unused++;
        if (0 == i)
            break;
        else
            i--;
    }
    return stack_len-unused;
}
#endif

void task_0(void)
{
    u8 stack_used;
    while (1) {
        delay_ms(100);
        stack_used = get_stack_used(stack, TASK0_STACK_LEN);
    }
}

/******************** ������ **************************/
void main(void)
{
    start_task_with_stack(task_0, stack, TASK0_STACK_LEN);
}




