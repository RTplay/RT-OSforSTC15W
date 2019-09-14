#include "rt_os.h"
#include "rt_os_private.h"


u8 os_rdy_tbl = 0;
u8 os_task_running_ID = 0;
OS_TCB os_tcb[TASK_SIZE] = {0};


//操作系统初始化
void os_init(void)
{
    tick_timer_init();
    os_task0_create();
}

//开始任务调度,从最低优先级的任务的开始
void os_start_task(void)
{
    tick_timer_start();
    os_task_running_ID = 0;
    SP = os_tcb[OS_IDLE_TASKID].OSTCBStkPtr - 13;
}

//描述：调用此函数去挂起一个任务，如果传送到os_task_suspend()的任务的ID是要挂起的任务或者是
//OS_TASKID_SELF，那么这个任务将被挂起。
//参数：taskID：需要挂起任务的ID。如果指定OS_TASKID_SELF，那么这个任务将自己挂起，再发生再
//次调度。
//返回：OS_NO_ERR：如果请求的任务被挂起。
//OS_TASK_SUSPEND_IDLE：如果想挂起空闲任务
//OS_TASK_ID_INVALID：想挂起任务优先级不合理
//OS_TASK_SUSPEND_TASKID：需要挂起的任务不存在。
u8  os_task_suspend (u8 taskID)
{
    BOOLEAN self;
    if (taskID == OS_IDLE_TASKID) {
        return (OS_TASK_SUSPEND_IDLE);
    }
    OS_ENTER_CRITICAL();
    if (taskID >= TASK_SIZE && taskID != OS_TASKID_SELF) {
        OS_EXIT_CRITICAL();
        return (OS_TASK_ID_INVALID);
    }
    if ((taskID < TASK_SIZE) && (os_tcb[taskID].OSTCBStkPtr == 0)) {
            OS_EXIT_CRITICAL();
            return (OS_TASK_SUSPEND_TASKID);                         // return（90）
    }

    if (taskID == OS_TASKID_SELF) {
        os_tcb[os_task_running_ID].OSTCBDly = 0;
        os_rdy_tbl &= ~(0x01<<os_task_running_ID); //清除任务就绪表标志
        self = TRUE;
    } else {
        os_tcb[taskID].OSTCBDly = 0;
        os_rdy_tbl &= ~(0x01<<taskID); //清除任务就绪表标志
        self = FALSE;
    }

    OS_EXIT_CRITICAL();
    if (self == TRUE) {
        OS_TASK_SW();
    }
    return (OS_NO_ERR);                                        //return（0)
}

//描述：调用此函数以恢复先前挂起的任务。
//参数：taskID是恢复任务的优先级。
//返回：OS_NO_ERR：如果请求的任务被恢复。
//OS_TASK_ID_INVALID：想恢复任务优先级不合理
//OS_TASK_RESUME_TASKID：需要恢复的任务不存在。
//OS_TASK_NOT_SUSPENDED：如果要恢复的任务尚未挂起
u8  os_task_resume (u8 taskID)
{
    if (taskID >= TASK_SIZE) {                              /* Make sure task priority is valid      */
        return (OS_TASK_ID_INVALID);
    }

    OS_ENTER_CRITICAL();
    if (os_tcb[taskID].OSTCBStkPtr == 0) {                 /* Task to suspend must exist            */
        OS_EXIT_CRITICAL();
        return (OS_TASK_RESUME_TASKID);                 // return（90）
    }

    os_tcb[taskID].OSTCBDly = 0;
    os_rdy_tbl |= 0x01<<taskID;                             //任务就绪表已经准备好
    OS_EXIT_CRITICAL();
    OS_TASK_SW();

    return (OS_NO_ERR);
}
