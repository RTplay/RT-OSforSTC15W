#ifndef   RT_OS_PRIVATE_H
#define   RT_OS_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include	"config.h"

/*
*********************************************************************************************************
*                                            PRIVATE VARIABLES
*********************************************************************************************************
*/
extern  u8           os_rdy_tbl;                           /* Table of tasks which are ready to run    */
extern  u8           os_task_running_num;
/*
*********************************************************************************************************
*                                          TASK CONTROL BLOCK
*********************************************************************************************************
*/
typedef struct os_tcb {
    u8              *OSTCBStkPtr;           /* 指向堆栈栈顶的指针                                      */
#ifdef STACK_DETECT_MODE
    u8              OSTCBStkSize;           /* 堆栈的大小                                              */
#endif
} OS_TCB;




#ifdef __cplusplus
}
#endif

#endif