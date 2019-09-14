#ifndef   __RT_OS_H
#define   __RT_OS_H

#ifdef __cplusplus
extern "C" {
#endif

#include	"config.h"

#define STACK_TYPE idata u8
#define STACK_MAGIC 0xAC
#define OS_TICK_RATE_MS 10
#define OS_DEFAULT_TIME_QUANTA 10
#define TASK_SIZE 8

#define OS_ENTER_CRITICAL()        EA = 0
#define OS_EXIT_CRITICAL()         EA = 1
#define NULL 0

#define  OS_TASKID_SELF            0xFFu                /* Indicate SELF priority                      */


typedef void (*idle_user)(void);

/*
*********************************************************************************************************
*                              TASK STATUS (Bit definition for OSTCBStatus)
*********************************************************************************************************
*/
#define  OS_STAT_DEFAULT         0x0000u    /* 初始化的默认值                                          */
#define  OS_STAT_RUNNING         0x0001u    /* 运行中                                                  */
#define  OS_STAT_RDY             0x0002u    /* Ready to run                                            */
#define  OS_STAT_SLEEP           0x0004u    /* Task is sleeping                                        */
#define  OS_STAT_SUSPEND         0x0008u    /* Task is suspended                                       */
#define  OS_STAT_DEAD            0x0010u    /* Task is exit                                            */
#define  OS_STAT_SEM             0x0020u    /* Pending on semaphore                                    */
#define  OS_STAT_MBOX            0x0040u    /* Pending on mailbox                                      */
#define  OS_STAT_Q               0x0080u    /* Pending on queue                                        */
#define  OS_STAT_MUTUAL          0x0100u    /* Pending on mutual exclusion semaphore                   */
#define  OS_STAT_FLAG            0x0200u    /* Pending on event flag group                             */


/*
*********************************************************************************************************
*                                             ERROR CODES
*********************************************************************************************************
*/
#define OS_NO_ERR                     0u
    
#define OS_TASK_ID_EXIST              40u
#define OS_TASK_ID_ERR                41u
#define OS_TASK_ID_INVALID            42u
#define OS_PRIO_ERR                   43u
#define OS_PRIO_INVALID               44u

#define OS_SEM_OVF                    50u

#define OS_TASK_DEL_ERR               60u
#define OS_TASK_DEL_IDLE              61u
#define OS_TASK_DEL_REQ               62u
#define OS_TASK_DEL_ISR               63u

#define OS_TASK_SUSPEND_TASKID        90u
#define OS_TASK_SUSPEND_IDLE          91u

#define OS_TASK_RESUME_TASKID         100u
#define OS_TASK_NOT_SUSPENDED         101u

u8      os_task_create(void (*task)(void), u8 taskID, u8 task_prio, u8 time_quanta, u8 *pstack, u8 stack_size);
void    os_init(void);
void    os_start_task(void);
u8      os_task_suspend(u8 taskID);
u8      os_task_resume(u8 taskID);
void    os_task_abandon(void);
void    os_task_exit(void);
u8      os_task_change_prio(u8 taskID, u8 new_prio);
void    os_tick_sleep(u16 ticks);
void    os_registered_idle_user(idle_user idle_func);
u8      get_stack_used (u8 *pstack, u8 stack_size);


#ifdef __cplusplus
}
#endif

#endif