#ifndef   __RT_OS_H
#define   __RT_OS_H

#ifdef __cplusplus
extern "C" {
#endif

#include	"config.h"

#define STACK_TYPE idata u8
#define STACK_MAGIC 0xAC

#define TASK_SIZE 8

#define OS_ENTER_CRITICAL()        EA = 0
#define OS_EXIT_CRITICAL()         EA = 1
#define NULL 0

#define  OS_TASKID_SELF            0xFFu                /* Indicate SELF priority                      */


typedef void (*idle_user)(void);

/*
*********************************************************************************************************
*                                             ERROR CODES
*********************************************************************************************************
*/
#define OS_NO_ERR                     0u
    
#define OS_TASK_ID_EXIST              40u
#define OS_TASK_ID_ERR                41u
#define OS_TASK_ID_INVALID            42u

#define OS_SEM_OVF                    50u

#define OS_TASK_DEL_ERR               60u
#define OS_TASK_DEL_IDLE              61u
#define OS_TASK_DEL_REQ               62u
#define OS_TASK_DEL_ISR               63u

#define OS_TASK_SUSPEND_TASKID        90u
#define OS_TASK_SUSPEND_IDLE          91u

#define OS_TASK_RESUME_TASKID         100u

u8 os_task_create(void (*task)(void), u8 taskID, u8 *pstack, u8 stack_size);
void os_init(void);
void os_start_task(void);
u8  os_task_suspend (u8 taskID);
u8  os_task_resume (u8 taskID);
void os_registered_idle_user(idle_user idle_func);
u8 get_stack_used(u8 *pstack, u8 stack_size);


#ifdef __cplusplus
}
#endif

#endif