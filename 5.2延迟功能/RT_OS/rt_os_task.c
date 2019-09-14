#include "rt_os.h"
#include "rt_os_private.h"
#include    "debug_uart.h"
#include	"delay.h"

//建立任务
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
    stack_size = stack_size; //消除编译警告
#endif
    *pstack++ = (u16)task; //将函数的地址高位压入堆栈，
    *pstack = (u16)task>>8; //将函数的地址低位压入堆栈，
{
    u8 i = 0;
    for (; i < 13; i++)        //初次被任务调度函数pop时需要清空堆栈内容，避免原有内容导致错误
        *(++pstack) = 0;
}
    os_tcb[taskID].OSTCBStkPtr = (u8)pstack; //将人工堆栈的栈顶，保存到堆栈的数组中
    os_rdy_tbl |= 0x01<<taskID; //任务就绪表已经准备好
    EA = 1;
    return (OS_NO_ERR);
}

idle_user idle_user_func = NULL;
//空闲任务
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
static STACK_TYPE stack[TASK0_STACK_LEN]; //建立一个 20 字节的静态区堆栈

void os_task0_create(void)
{
    os_task_create(idle_task_0, OS_IDLE_TASKID, stack, TASK0_STACK_LEN);
}

//注册空闲任务用户接口
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
