#include "rt_os.h"
#include "rt_os_private.h"


u8 OSSchedLockNestingCtr = 0;
u8 os_task_running_ID = 0;
OS_TCB os_tcb[TASK_SIZE] = {0};
u8 os_core_start = 0;
xdata u8 os_task_run[TASK_SIZE];

//操作系统初始化
u8 os_init(void)
{
    u8 i = 0;
    for (; i<TASK_SIZE; i++)
        os_task_run[i] = 0xFF;
    tick_timer_init();
    os_task0_create();
#ifdef MEM_ENABLE
    return os_mem_init();
#else
    return OS_ERR_NONE;
#endif
}

//开始任务调度,从最低优先级的任务的开始
void os_start_task(void)
{
    char i = 0;
    u8 highest_prio_id = 0;

    tick_timer_start();
    for (; i<TASK_SIZE; i++)//这里不用判断已运行任务队列，因为已运行任务队列是空的，之需要运行第一个找到的最高优先级任务
    {
        if ((os_tcb[i].OSTCBStatus == OS_STAT_RDY) && (os_tcb[i].OSTCBPrio > os_tcb[highest_prio_id].OSTCBPrio))
            highest_prio_id = i;
    }
    os_task_running_ID = highest_prio_id;
    os_tcb[highest_prio_id].OSTCBStatus = OS_STAT_RUNNING;
    //系统首次运行一定需要赋值剩余时间片，并且已运行任务队列一定是第一个
    os_tcb[highest_prio_id].OSTCBTimeQuantaCtr = os_tcb[highest_prio_id].OSTCBTimeQuanta;
    os_core_start = 1;//系统开始运行标志
    SP = os_tcb[highest_prio_id].OSTCBStkPtr - 13;
}

//主动放弃CPU使用权给同级任务，当自身仍然是唯一的最高级时任务不会切换，还会继续运行
void os_task_abandon(void)
{
    OS_TASK_SW();
}

//任务退出时进行调用
void os_task_exit(void)
{
    os_tcb[os_task_running_ID].OSTCBTimeQuantaCtr = 0;
    os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_DEAD;
    OS_TASK_SW();
}

//描述：调用此函数去挂起一个任务，如果传送到os_task_suspend()的任务的ID是要挂起的任务或者是
//OS_TASKID_SELF，那么这个任务将被挂起。
//参数：taskID：需要挂起任务的ID。如果指定OS_TASKID_SELF，那么这个任务将自己挂起，再发生再
//次调度。
//返回：OS_ERR_NONE：如果请求的任务被挂起。
//OS_ERR_TASK_SUSPEND_IDLE：如果想挂起空闲任务
//OS_ERR_TASK_ID_INVALID：想挂起任务优先级不合理
//OS_ERR_TASK_SUSPEND_TASKID：需要挂起的任务不存在。
u8  os_task_suspend (u8 taskID)
{
    BOOLEAN self;
    if (taskID == OS_IDLE_TASKID) {
        return (OS_ERR_TASK_SUSPEND_IDLE);
    }
    if (taskID >= TASK_SIZE && taskID != OS_TASKID_SELF) {
        return (OS_ERR_TASK_ID_INVALID);
    }
    
    OS_ENTER_CRITICAL();
    if ((taskID < TASK_SIZE) && (os_tcb[taskID].OSTCBStkPtr == 0)) {
            OS_EXIT_CRITICAL();
            return (OS_ERR_TASK_SUSPEND_TASKID);                         // return（90）
    }

    if (taskID == OS_TASKID_SELF) {
        os_tcb[os_task_running_ID].OSTCBDly = 0;
        //os_rdy_tbl &= ~(0x01<<os_task_running_ID); //清除任务就绪表标志
        os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_SUSPEND;
        self = TRUE;
    } else {
        os_tcb[taskID].OSTCBDly = 0;
        //os_rdy_tbl &= ~(0x01<<taskID); //清除任务就绪表标志
        os_tcb[taskID].OSTCBStatus = OS_STAT_SUSPEND;
        self = FALSE;
    }

    OS_EXIT_CRITICAL();
    if (self == TRUE) {
        OS_TASK_SW();
    }
    return (OS_ERR_NONE);                                        //return（0）
}

//描述：调用此函数以恢复先前挂起的任务。
//参数：taskID是恢复任务的优先级。
//返回：OS_ERR_NONE：如果请求的任务被恢复。
//OS_ERR_TASK_ID_INVALID：想恢复任务ID不合理
//OS_ERR_TASK_RESUME_TASKID：需要恢复的任务不存在。
//OS_ERR_TASK_NOT_SUSPENDED：如果要恢复的任务尚未挂起
u8  os_task_resume (u8 taskID)
{
    if (taskID >= TASK_SIZE) {                             /* Make sure task priority is valid      */
        return (OS_ERR_TASK_ID_INVALID);
    }

    OS_ENTER_CRITICAL();
    if (os_tcb[taskID].OSTCBStkPtr == 0) {                 /* Task to suspend must exist            */
        OS_EXIT_CRITICAL();
        return (OS_ERR_TASK_RESUME_TASKID);                 // return（90）
    }
    if (os_tcb[taskID].OSTCBStatus != OS_STAT_SUSPEND) {   /* Task must be suspend                  */
        OS_EXIT_CRITICAL();
        return (OS_ERR_TASK_NOT_SUSPENDED);                 // return（101）
    }
    
    os_tcb[taskID].OSTCBDly = 0;
    //os_rdy_tbl |= 0x01<<taskID;                             //任务就绪表已经准备好
    os_tcb[taskID].OSTCBStatus = OS_STAT_RDY;
    OS_EXIT_CRITICAL();
    OS_TASK_SW();

    return (OS_ERR_NONE);
}
