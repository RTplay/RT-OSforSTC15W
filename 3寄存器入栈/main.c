#include	"config.h"
#include    "RT-OS_type.h"
#include	"delay.h"

#define TASK0_STACK_LEN 20
STACK_TYPE stack[TASK0_STACK_LEN]; //建立一个 20 字节的静态区堆栈
u8 os_task_stack_top; //记录堆栈顶部地址

void OS_TASK_SW(void);

void start_task_with_stack(void (*pfun)(), u8 *pstack, u8 stack_size)
{
#ifdef STACK_DETECT_MODE
    u8 i = 0;
    for (; i<stack_size; i++)
        pstack[i] = STACK_MAGIC;
#else
    stack_size = stack_size; //消除编译警告
#endif
    *pstack++ = (unsigned int)pfun; //将函数的地址高位压入堆栈，
    *pstack = (unsigned int)pfun>>8; //将函数的地址低位压入堆栈，
    SP = (u8)pstack; //将堆栈指针指向人工堆栈的栈顶
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

void task_0(void)
{
    u8 stack_used;
    while (1) {
        delay_ms(100);
#ifdef STACK_DETECT_MODE
        stack_used = get_stack_used(stack, TASK0_STACK_LEN);
#endif
        OS_TASK_SW();
    }
}

void OS_TASK_SW(void)
{
    EA=0;

#pragma asm
    PUSH     ACC
    PUSH     B
    PUSH     DPH
    PUSH     DPL
    PUSH     PSW
    MOV      PSW,#00H
    PUSH     AR0
    PUSH     AR1
    PUSH     AR2
    PUSH     AR3
    PUSH     AR4
    PUSH     AR5
    PUSH     AR6
    PUSH     AR7
#pragma endasm
    os_task_stack_top = SP;
//切换任务栈
    SP = os_task_stack_top;
#pragma asm
    POP      AR7
    POP      AR6
    POP      AR5
    POP      AR4
    POP      AR3
    POP      AR2
    POP      AR1
    POP      AR0
    POP      PSW
    POP      DPL
    POP      DPH
    POP      B
    POP      ACC
#pragma endasm

    EA=1;
}

/******************** 主函数 **************************/
void main(void)
{

    start_task_with_stack(task_0, stack, TASK0_STACK_LEN);
}

void int0_int (void) interrupt INT0_VECTOR {
    u8 i;
    unsigned int counter = 0xffff;
    EA = 0;
    IE0 = 0;
    for(i=0; i<5; i++) {
        counter = 0x0fff;
        while( (~IE0) && (counter--) );
        if(IE0) {
            IE0=0;
        }
        EA = 1;
    }
}


