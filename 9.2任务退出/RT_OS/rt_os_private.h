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
    u8              OSTCBStkPtr;            /* ָ���ջջ����ָ��                                      */
    u16             OSTCBDly;               /* ����ȴ��Ľ�����                                        */
    u16             OSTCBStatus;            /* ����״̬                                                */
    u8              OSTCBPrio;              /* �������ȼ�                                              */
    u8              OSTCBTimeQuanta;        /* ����ʱ��Ƭ����                                          */
    u8              OSTCBTimeQuantaCtr;     /* ��ǰʱ��Ƭ��ʣ�೤��                                    */
#ifdef STACK_DETECT_MODE
    u8              OSTCBStkSize;           /* ��ջ�Ĵ�С                                              */
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