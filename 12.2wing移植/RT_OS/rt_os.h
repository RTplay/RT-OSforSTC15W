#ifndef   __RT_OS_H
#define   __RT_OS_H

#ifdef __cplusplus
extern "C" {
#endif

#include	"config.h"

//任务堆栈类型
#define STACK_TYPE idata u8
//堆栈使用量魔数
#define STACK_MAGIC             0xAC
//系统节拍时间，单位是ms
#define OS_TICK_RATE_MS         10
//默认时间片，10个节拍
#define OS_DEFAULT_TIME_QUANTA  10
//最大任务数量，目前不能大于8
#define TASK_SIZE               8
//互斥锁数量，开启MUTEX_ENABLE后有效
#define OS_MUTEX_SIZE           5
//信号量数量，开启SEM_ENABLE后有效
#define OS_SEM_SIZE             5
//消息队列数量，开启MSGQ_ENABLE后有效
#define OS_MSGQ_SIZE            2
//消息队列数量，开启FLAG_ENABLE后有效
#define OS_FLAG_SIZE            2
//OS_FLAGS数据类型的位大小（8,16或32）
#define OS_FLAGS_NBITS          8
//动态内存分配大小，开启MEM_ENABLE后有效
#define OS_MEM_SIZE             30

extern u8 OSSchedLockNestingCtr;

#define CPU_ENTER_CRITICAL()   EA = 0
#define CPU_EXIT_CRITICAL()    EA = 1
//#define OS_ENTER_CRITICAL()   EA = 0
//#define OS_EXIT_CRITICAL()    EA = 1
#define  OS_ENTER_CRITICAL()                                  \
    do {                                                      \
        CPU_ENTER_CRITICAL();                                 \
        OSSchedLockNestingCtr++;                              \
    } while (0)

#define  OS_EXIT_CRITICAL()                                   \
    do {                                                      \
        OSSchedLockNestingCtr--;                              \
        if (OSSchedLockNestingCtr == 0) {                     \
            CPU_EXIT_CRITICAL();                              \
        }                                                     \
    } while (0)

#ifndef NULL
    #define NULL 0
#endif
#define  OS_TASKID_SELF            0xFFu                /* Indicate SELF priority                      */

/**************************************系统统计使用的结构体*********************************************/
/**************************************如果功能未开启会读取为0******************************************/
typedef struct sys_statistics {
    u16             OSSSStatus;             /* 任务状态                                                */
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
#define  OS_STAT_MSGQ            0x0080u    /* Pending on queue                                        */
#define  OS_STAT_FLAG            0x0100u    /* Pending on event flag group                             */
#define  OS_STAT_MUTUAL          0x0200u    /* Pending on mutual exclusion semaphore                   */


/*
*********************************************************************************************************
*                                             ERROR CODES
*********************************************************************************************************
*/
#define OS_ERR_NONE                       0u
#define OS_ERR_INVALID_OPT                7u
#define OS_ERR_PDATA_NULL                 9u

#define OS_ERR_TIMEOUT                    10u
#define OS_ERR_PEND_LOCKED                13u
#define OS_ERR_PEND_UNLOCK                14u
#define OS_ERR_PEND_ABORT                 15u

#define OS_ERR_MSGQ_FULL                  30u
#define OS_ERR_MSGQ_EMPTY                 31u
#define OS_ERR_MSGQ_ID_INVALID            32u

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

#define OS_ERR_TASK_WAITING               73u

#define OS_ERR_MEM_INVALID_PART           90u
#define OS_ERR_MEM_INVALID_BLKS           91u
#define OS_ERR_MEM_INVALID_SIZE           92u
#define OS_ERR_MEM_NO_FREE_BLKS           93u
#define OS_ERR_MEM_FULL                   94u
#define OS_ERR_MEM_INVALID_PBLK           95u
#define OS_ERR_MEM_INVALID_PMEM           96u
#define OS_ERR_MEM_INVALID_PDATA          97u
#define OS_ERR_MEM_INVALID_ADDR           98u
#define OS_ERR_MEM_NAME_TOO_LONG          99u

#define OS_ERR_NOT_MUTEX_OWNER            100u
#define OS_ERR_MUTXE_ID_INVALID           101u
#define OS_ERR_NOT_MUTEX                  102u

#define OS_ERR_FLAG_ID_INVALID            110u
#define OS_ERR_FLAG_WAIT_TYPE             111u
#define OS_ERR_FLAG_NOT_RDY               112u
#define OS_ERR_FLAG_INVALID_OPT           113u
#define OS_ERR_FLAG_GRP_DEPLETED          114u
#define OS_ERR_FLAG_USED                  115u
#define OS_ERR_FLAG_UNUSED                116u
#define OS_ERR_FLAG_CORE                  117u

/*
*********************************************************************************************************
*                                     os_msgq_post_opt() OPTIONS
*
*********************************************************************************************************
*/
#define  OS_POST_OPT_NONE            0x00u  /* NO option selected                                      */
#define  OS_POST_OPT_FRONT           0x01u  /* Post to highest priority task waiting                   */
#define  OS_POST_OPT_NO_SCHED        0x02u  /* Do not call the scheduler if this option is selected    */

/*
*********************************************************************************************************
*                                         EVENT FLAGS
*********************************************************************************************************
*/
#define  OS_FLAG_WAIT_CLR_ALL           0u  /* Wait for ALL    the bits specified to be CLR (i.e. 0)   */
#define  OS_FLAG_WAIT_CLR_AND           0u

#define  OS_FLAG_WAIT_CLR_ANY           1u  /* Wait for ANY of the bits specified to be CLR (i.e. 0)   */
#define  OS_FLAG_WAIT_CLR_OR            1u

#define  OS_FLAG_WAIT_SET_ALL           2u  /* Wait for ALL    the bits specified to be SET (i.e. 1)   */
#define  OS_FLAG_WAIT_SET_AND           2u

#define  OS_FLAG_WAIT_SET_ANY           3u  /* Wait for ANY of the bits specified to be SET (i.e. 1)   */
#define  OS_FLAG_WAIT_SET_OR            3u

#define  OS_FLAG_CONSUME             0x80u  /* Consume the flags if condition(s) satisfied             */


#define  OS_FLAG_CLR                    0u
#define  OS_FLAG_SET                    1u

/*
*********************************************************************************************************
*       Possible values for 'opt' argument of OSSemDel(), OSMboxDel(), OSQDel() and OSMutexDel()
*********************************************************************************************************
*/
#define  OS_DEL_NO_PEND                 0u
#define  OS_DEL_ALWAYS                  1u

u8      os_task_create(void (*task)(void), u8 taskID, u8 task_prio, u8 time_quanta, u8 *pstack, u8 stack_size);
u8      os_init(void);
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

#ifdef MSGQ_ENABLE
u8      os_msgq_init(u8 msgq_index, void **q_start, u8 q_size);
u8      os_msgq_flush(u8 msgq_index);
void   *os_msgq_pend (u8 msgq_index, u16 ticks, u8 *err);
u8      os_msgq_post(u8 msgq_index, void *pmsg);
u8      os_msgq_post_front(u8 msgq_index, void *pmsg);
void   *os_msgq_accept(u8 msgq_index, u8 *err);
u8      os_msgq_query (u8 msgq_index, u8 *err);
u8      os_msgq_post_opt (u8 msgq_index, void *pmsg, u8 opt);
#endif

#ifdef FLAG_ENABLE
#if OS_FLAGS_NBITS == 8u                    /* Determine the size of OS_FLAGS (8, 16 or 32 bits)       */
typedef  u8    OS_FLAGS;
#endif

#if OS_FLAGS_NBITS == 16u
typedef  u16   OS_FLAGS;
#endif

#if OS_FLAGS_NBITS == 32u
typedef  u32   OS_FLAGS;
#endif
u8          os_flag_init (u8 flag_index, OS_FLAGS  init_flags, OS_FLAGS  flags, u8 wait_type);
u8          os_flag_release (u8 flag_index, u8 opt);
OS_FLAGS    os_flag_accept (u8 flag_index, u8 *err);
OS_FLAGS    os_flag_pend (u8 flag_index, u16 ticks, u8 *err);
OS_FLAGS    os_flag_post (u8 flag_index, OS_FLAGS flags, u8 opt, u8 *err);
#endif

#ifdef MEM_ENABLE
void       *os_malloc (u16 c_size);
void        os_free (void *free_ptr);
void       *os_calloc (u16 nmemb, u16 c_size);
void       *os_realloc (void *ptr, u16 c_size);
void       *os_memset(void *s, u8 c, u16 n);
void       *os_memcpy(void *dest, const void *src, u16 n);
#endif

#ifdef SYSTEM_DETECT_MODE
OS_SS      *get_sys_statistics(void);
#endif

#ifdef __cplusplus
}
#endif

#endif