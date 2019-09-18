#ifndef   __RT_OS_PRIVATE_H
#define   __RT_OS_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include	"config.h"
#include    "rt_os.h"

#define OS_IDLE_TASKID 0u
#define OS_IDLE_TASK_PRIO 0u

/*
*********************************************************************************************************
*                           TASK PEND STATUS (Status codes for OSTCBStatPend)
*********************************************************************************************************
*/
#define  OS_STAT_PEND_OK                0u  /* Pending status OK, not pending, or pending complete     */
#define  OS_STAT_PEND_TO                1u  /* Pending timed out                                       */
#define  OS_STAT_PEND_ABORT             2u  /* Pending aborted                                         */

/*
*********************************************************************************************************
*                                        OS_EVENT types
*********************************************************************************************************
*/
#define  OS_EVENT_TYPE_UNUSED           0u
#define  OS_EVENT_TYPE_USED             1u

/*
*********************************************************************************************************
*                                          TASK CONTROL BLOCK
*********************************************************************************************************
*/
typedef struct os_tcb {
    u8              OSTCBStkPtr;            /* ָ���ջջ����ָ��                                      */
    u16             OSTCBDly;               /* ����ȴ��Ľ�����                                        */
    u16             OSTCBStatus;            /* ����״̬                                                */
    u8              OSTCBPrio;              /* �������ȼ�                                              */
    u8              OSTCBTimeQuanta;        /* ����ʱ��Ƭ����                                          */
    u8              OSTCBTimeQuantaCtr;     /* ��ǰʱ��Ƭ��ʣ�೤��                                    */
#ifdef SYSTEM_DETECT_MODE
    u8             *OSTCBStkBottomPtr;      /* ��ջ�ĵײ�ָ��                                          */
    u8              OSTCBCyclesTot;         /* ��ǰ�������еĽ�����                                    */
    u8              OSTCBStkSize;           /* ��ջ�Ĵ�С                                              */
#endif
#if (defined MUTEX_ENABLE) || (defined SEM_ENABLE) || (defined MSGQ_ENABLE) || (defined FLAG_ENABLE)
    u8              OSTCBStatPend;          /* �������״̬                                            */
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
extern  OS_SS        sys_stat[TASK_SIZE];

void os_task0_create(void);
void OS_TASK_SW(void);
void tick_timer_init(void);
void tick_timer_start(void);

u8      get_stack_used(u8 *pstack, u8 stack_size);

#ifdef __cplusplus
}
#endif

#endif