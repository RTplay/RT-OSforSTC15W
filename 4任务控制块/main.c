#include	"config.h"
#include    "rt_os.h"
#include	"delay.h"

void OS_TASK_SW(void);

void idle_task_1(void)
{
    while (1) {
        delay_ms(100);
    }
}

//void OS_TASK_SW(void)
//{
//    EA=0;

//#pragma asm
//    PUSH     ACC
//    PUSH     B
//    PUSH     DPH
//    PUSH     DPL
//    PUSH     PSW
//    MOV      PSW,#00H
//    PUSH     AR0
//    PUSH     AR1
//    PUSH     AR2
//    PUSH     AR3
//    PUSH     AR4
//    PUSH     AR5
//    PUSH     AR6
//    PUSH     AR7
//#pragma endasm
//    os_task_stack_top = SP;
////切换任务栈
//    SP = os_task_stack_top;
//#pragma asm
//    POP      AR7
//    POP      AR6
//    POP      AR5
//    POP      AR4
//    POP      AR3
//    POP      AR2
//    POP      AR1
//    POP      AR0
//    POP      PSW
//    POP      DPL
//    POP      DPH
//    POP      B
//    POP      ACC
//#pragma endasm

//    EA=1;
//}

/******************** 主函数 **************************/
void main(void)
{
    os_init();
    os_start_task();
}

void int0_int (void) interrupt INT0_VECTOR {
    u8 i;
    unsigned int counter = 0xffff;
    EA = 0;
    IE0 = 0;
    for(i=0; i<5; i++) {
        counter = 0x0fff;
        while( (~IE0) && (counter--) );
        if(IE0) {
            IE0=0;
        }
        EA = 1;
    }
}


