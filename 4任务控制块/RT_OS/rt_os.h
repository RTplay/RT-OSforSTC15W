#ifndef   RT_OS_H
#define   RT_OS_H

#ifdef __cplusplus
extern "C" {
#endif

#include	"config.h"

#define STACK_TYPE idata u8
#define STACK_MAGIC 0xAC

#define TASK_SIZE 8

#define NULL 0


typedef void (*idle_user)(void);

/*
*********************************************************************************************************
*                                             ERROR CODES
*********************************************************************************************************
*/
#define OS_NO_ERR                     0u
    
#define OS_TASK_NUM_EXIST             40u
#define OS_TASK_NUM_ERR               41u
#define OS_TASK_NUM_INVALID           42u

#define OS_SEM_OVF                    50u

#define OS_TASK_DEL_ERR               60u
#define OS_TASK_DEL_IDLE              61u
#define OS_TASK_DEL_REQ               62u
#define OS_TASK_DEL_ISR               63u
    
u8 os_task_create(void (*task)(void), u8 taskID, u8 *pstack, u8 stack_size);
void os_init(void);
void os_start_task(void);
void os_registered_idle_user(idle_user idle_func);
u8 get_stack_used(u8 *pstack, u8 stack_size);
    
    
#ifdef __cplusplus
}
#endif

#endif