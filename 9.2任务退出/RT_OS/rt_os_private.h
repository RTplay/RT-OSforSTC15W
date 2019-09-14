#ifndef   __RT_OS_PRIVATE_H
#define   __RT_OS_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include	"config.h"

#define OS_IDLE_TASKID 0u
#define OS_IDLE_TASK_PRIO 0u
/*
*********************************************************************************************************
*                                          TASK CONTROL BLOCK
*********************************************************************************************************
*/
typedef struct os_tcb {
    u8              OSTCBStkPtr;            /* 指向堆栈栈顶的指针                                      */
    u16             OSTCBDly;               /* 任务等待的节拍数                                        */
    u16             OSTCBStatus;            /* 任务状态                                                */
    u8              OSTCBPrio;              /* 任务优先级                                              */
    u8              OSTCBTimeQuanta;        /* 任务时间片总数                                          */
    u8              OSTCBTimeQuantaCtr;     /* 当前时间片的剩余长度                                    */
#ifdef STACK_DETECT_MODE
    u8              OSTCBStkSize;           /* 堆栈的大小                                              */
#endif
} OS_TCB;


/*
*********************************************************************************************************
*                                            PRIVATE VARIABLES
*********************************************************************************************************
*/
extern  u8           os_task_running_ID;
extern  OS_TCB       os_tcb[TASK_SIZE];
extern  u8           os_core_start;
extern  u8           os_task_run[TASK_SIZE];

void os_task0_create(void);
void OS_TASK_SW(void);
void tick_timer_init(void);
void tick_timer_start(void);

#ifdef __cplusplus
}
#endif

#endif