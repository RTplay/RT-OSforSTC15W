#include "rt_os.h"
#include "rt_os_private.h"

//任务调度函数，只能在非中断中使用
void OS_TASK_SW(void)
{
    u8 i = TASK_SIZE - 1;
    OS_ENTER_CRITICAL();
#pragma asm
    PUSH     ACC
    PUSH     B
    PUSH     DPH
    PUSH     DPL
    PUSH     PSW
    MOV      PSW,#00H
    PUSH     AR0
    PUSH     AR1
    PUSH     AR2
    PUSH     AR3
    PUSH     AR4
    PUSH     AR5
    PUSH     AR6
    PUSH     AR7
#pragma endasm
    os_tcb[os_task_running_ID].OSTCBStkPtr = SP;
//切换任务栈
    for (; i>0; i--)
    {
        if (os_rdy_tbl & (1<<i))
            break;
    }
    os_task_running_ID = i;
    SP = os_tcb[os_task_running_ID].OSTCBStkPtr;
#pragma asm
    POP      AR7
    POP      AR6
    POP      AR5
    POP      AR4
    POP      AR3
    POP      AR2
    POP      AR1
    POP      AR0
    POP      PSW
    POP      DPL
    POP      DPH
    POP      B
    POP      ACC
#pragma endasm
    OS_EXIT_CRITICAL();
}
