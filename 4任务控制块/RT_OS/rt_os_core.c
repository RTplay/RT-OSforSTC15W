#include "rt_os.h"
#include "rt_os_private.h"

u8 os_rdy_tbl = 0;
u8 os_task_running_num = 0;
OS_TCB os_tcb[TASK_SIZE];

//建立任务
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
    stack_size = stack_size; //消除编译警告
#endif
    *pstack++ = (unsigned int)task; //将函数的地址高位压入堆栈，
    *pstack = (unsigned int)task>>8; //将函数的地址低位压入堆栈，
    pstack += 13;
    
    os_tcb[taskID].OSTCBStkPtr = pstack; //将人工堆栈的栈顶，保存到堆栈的数组中
    os_rdy_tbl |= 0x01<<taskID; //任务就绪表已经准备好
    
    return (OS_NO_ERR);
}

idle_user idle_user_func = NULL;
//空闲任务
void idle_task_0(void)
{
    while (1) {
        if (idle_user_func)
            idle_user_func();
    }
}

#define TASK0_STACK_LEN 20
STACK_TYPE stack[TASK0_STACK_LEN]; //建立一个 20 字节的静态区堆栈
//操作系统初始化
void os_init(void)
{
    os_task_create(idle_task_0, 0, stack, TASK0_STACK_LEN);
}

//开始任务调度,从最低优先级的任务的开始
void os_start_task(void)
{
    os_task_running_num = 0;
    SP = (u8)os_tcb[0].OSTCBStkPtr - 13;
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
