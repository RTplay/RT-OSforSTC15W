#ifndef   __RT_OS_H
#define   __RT_OS_H

#ifdef __cplusplus
extern "C" {
#endif

#include	"config.h"

//任务堆栈类型
#define STACK_TYPE idata u8
//堆栈使用量魔数
#define STACK_MAGIC 0xAC
//系统节拍时间，单位是ms
#define OS_TICK_RATE_MS 10
//默认时间片，10个节拍
#define OS_DEFAULT_TIME_QUANTA 10
//最大任务数量，目前不能大于8
#define TASK_SIZE 8
//互斥锁数量，开启MUTEX_ENABLE后有效
#define OS_MUTEX_SIZE   5
//互斥锁数量，开启SEM_ENABLE后有效
#define OS_SEM_SIZE   5


#define OS_ENTER_CRITICAL()        EA = 0
#define OS_EXIT_CRITICAL()         EA = 1
#ifndef NULL
    #define NULL 0
#endif
#define  OS_TASKID_SELF            0xFFu                /* Indicate SELF priority                      */

/**************************************系统统计使用的结构体*********************************************/
/**************************************如果功能未开启会读取为0******************************************/
typedef struct sys_statistics {
    u8              OSSSStatus;             /* 任务状态                                                */
    u8              OSSSCyclesTot;          /* 每100个时钟周期占用的节拍数                             */
    u8              OSSSMaxUsedStk;         /* 堆栈的最大使用量                                        */
} OS_SS;

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
#define  OS_STAT_MUTEX           0x0020u    /* Pending on mutex                                        */
#define  OS_STAT_SEM             0x0040u    /* Pending on semaphore                                    */
#define  OS_STAT_MBOX            0x1000u    /* Pending on mailbox                                      */
#define  OS_STAT_Q               0x0080u    /* Pending on queue                                        */
#define  OS_STAT_MUTUAL          0x0100u    /* Pending on mutual exclusion semaphore                   */
#define  OS_STAT_FLAG            0x0200u    /* Pending on event flag group                             */


/*
*********************************************************************************************************
*                                             ERROR CODES
*********************************************************************************************************
*/
#define OS_ERR_NONE                       0u
#define OS_ERR_PDATA_NULL                 9u

#define OS_ERR_TIMEOUT                    10u
#define OS_ERR_PEND_LOCKED                13u
#define OS_ERR_PEND_UNLOCK                14u

#define OS_ERR_TASK_ID_EXIST              40u
#define OS_ERR_TASK_ID_ERR                41u
#define OS_ERR_TASK_ID_INVALID            42u
#define OS_ERR_PRIO_ERR                   43u
#define OS_ERR_PRIO_INVALID               44u

#define OS_ERR_SEM_OVF                    50u
#define OS_ERR_SEM_ID_INVALID             51u
#define OS_ERR_SEM_UNAVAILABLE            52u

#define OS_ERR_TASK_DEL_ERR               60u
#define OS_ERR_TASK_DEL_IDLE              61u
#define OS_ERR_TASK_DEL_REQ               62u
#define OS_ERR_TASK_DEL_ISR               63u
#define OS_ERR_TASK_SUSPEND_TASKID        64u
#define OS_ERR_TASK_SUSPEND_IDLE          65u
#define OS_ERR_TASK_RESUME_TASKID         66u
#define OS_ERR_TASK_NOT_SUSPENDED         67u

#define OS_ERR_NOT_MUTEX_OWNER            100u
#define OS_ERR_MUTXE_ID_INVALID           101u
#define OS_ERR_NOT_MUTEX                  102u




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

#ifdef MUTEX_ENABLE
u8      os_mutex_init(u8 mutex_index);
u8      os_mutex_pend(u8 mutex_index, u16 ticks);
u8      os_mutex_post(u8 mutex_index);
u8      os_mutex_accept(u8 mutex_index);
u8      os_mutex_query(u8 mutex_index);
#endif

#ifdef SEM_ENABLE
u8      os_sem_init(u8 sem_index, u8 initial_value);
u8      os_sem_pend(u8 sem_index, u16 ticks);
u8      os_sem_post(u8 sem_index);
u8      os_sem_accept(u8 sem_index);
u8      os_sem_query(u8 sem_index, u8 *sem_count);
u8      os_sem_set(u8 sem_index, u8 sem_count);
#endif

#ifdef SYSTEM_DETECT_MODE
OS_SS *get_sys_statistics(void);
#endif

#ifdef __cplusplus
}
#endif

#endif