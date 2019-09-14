#include "rt_os.h"
#include "rt_os_private.h"
#include    "debug_uart.h"
#include	"delay.h"

//建立任务
u8 os_task_create(void (*task)(void), u8 taskID, u8 task_prio, u8 time_quanta, u8 *pstack, u8 stack_size)
{
    if (taskID >= TASK_SIZE)
        return (OS_TASK_ID_INVALID);
    if (task_prio == OS_IDLE_TASK_PRIO)//不允许与idle相同优先级
        return (OS_PRIO_INVALID);
    if (os_tcb[taskID].OSTCBStkPtr != 0)
        return (OS_TASK_ID_EXIST);
    OS_ENTER_CRITICAL();
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
    //os_rdy_tbl |= 0x01<<taskID; //任务就绪表已经准备好
    os_tcb[taskID].OSTCBStatus = OS_STAT_RDY;
    os_tcb[taskID].OSTCBPrio = task_prio;
    if ( time_quanta == 0 )
        os_tcb[taskID].OSTCBTimeQuanta = OS_DEFAULT_TIME_QUANTA;
    else
        os_tcb[taskID].OSTCBTimeQuanta = time_quanta;
    if (os_core_start) { //如果任务已经启动则进行一次任务调度
        os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_RDY; //先将自己置位就绪态
        OS_EXIT_CRITICAL();
        OS_TASK_SW();
    }
    else
        OS_EXIT_CRITICAL();
    return (OS_NO_ERR);
}

// 此功能允许您动态更改任务的优先级。 请注意，新的优先级必须可用。
// 返回：OS_NO_ERR:是成功
// OS_TASK_ID_INVALID：如果试图改变idle任务的优先级或者任务不存在或者任务ID超过最大任务数量
// OS_PRIO_INVALID: 如果试图更改为idle任务的优先级（0）是不允许的
// OS_PRIO_ERR: 其他错误
u8 os_task_change_prio (u8 taskID, u8 new_prio)
{
    if (taskID == OS_IDLE_TASKID)
        return (OS_TASK_ID_INVALID);
    if (new_prio == OS_IDLE_TASK_PRIO)
        return (OS_PRIO_INVALID);  
    if (taskID >= TASK_SIZE && taskID != OS_TASKID_SELF)
        return (OS_TASK_ID_INVALID);
    
    OS_ENTER_CRITICAL();
    if ((taskID < TASK_SIZE) && (os_tcb[taskID].OSTCBStkPtr == 0)) {
            OS_EXIT_CRITICAL();
            return (OS_TASK_ID_INVALID);
    }

    if ((taskID == OS_TASKID_SELF) || (taskID == os_task_running_ID)){
        os_tcb[os_task_running_ID].OSTCBPrio = new_prio;
        OS_EXIT_CRITICAL();
    } else {
        os_tcb[taskID].OSTCBPrio = new_prio;
        os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_RDY;
        OS_EXIT_CRITICAL();
        OS_TASK_SW();
    }

    return (OS_NO_ERR);                                        //return（0)
}



/************************************************************************************************/
/*************************************空闲任务相关代码*******************************************/
idle_user idle_user_func = NULL;
//空闲任务
void idle_task_0(void)
{
    debug_print("idle_task_0 in\r\n");
    while (1) {
        char i = 0;
        u8 highest_prio_id = 0;
        if (idle_user_func)
            idle_user_func();
        OS_ENTER_CRITICAL();
        os_tcb[OS_IDLE_TASKID].OSTCBStatus = OS_STAT_RDY;
        for (; i<TASK_SIZE; i++) {
            if ((os_tcb[i].OSTCBStatus == OS_STAT_RDY) && (os_tcb[i].OSTCBPrio > os_tcb[highest_prio_id].OSTCBPrio))
                highest_prio_id = i;
        }
        if (highest_prio_id != 0) {//如果不是空闲任务自己，就进行切换任务
            OS_EXIT_CRITICAL();
            OS_TASK_SW ();
        }
        else {
            OS_EXIT_CRITICAL();
        }
    }
}

#define TASK0_STACK_LEN 20
static STACK_TYPE idle_stack[TASK0_STACK_LEN]; //建立一个 20 字节的静态区堆栈

void os_task0_create(void)
{
    u8* stack = idle_stack;
    OS_ENTER_CRITICAL();
#ifdef STACK_DETECT_MODE
{
    u8 i = 0;
    for (; i<TASK0_STACK_LEN; i++)
        stack[i] = STACK_MAGIC;
}
#endif
    *stack++ = (u16)idle_task_0; //将函数的地址高位压入堆栈，
    *stack = (u16)idle_task_0>>8; //将函数的地址低位压入堆栈，
{
    u8 i = 0;
    for (; i < 13; i++)        //初次被任务调度函数pop时需要清空堆栈内容，避免原有内容导致错误
        *(++stack) = 0;
}
    os_tcb[OS_IDLE_TASKID].OSTCBStkPtr = (u8)stack; //将人工堆栈的栈顶，保存到堆栈的数组中
    os_tcb[OS_IDLE_TASKID].OSTCBStatus = OS_STAT_RDY;
    os_tcb[OS_IDLE_TASKID].OSTCBPrio = OS_IDLE_TASK_PRIO;
    os_tcb[OS_IDLE_TASKID].OSTCBTimeQuanta = OS_DEFAULT_TIME_QUANTA;
    OS_EXIT_CRITICAL();
}

//注册空闲任务用户接口
void os_registered_idle_user(idle_user idle_func)
{
    idle_user_func = idle_func;
}

/************************************************************************************************/
/*************************************堆栈统计相关代码*******************************************/
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
