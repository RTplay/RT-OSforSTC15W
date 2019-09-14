#include "rt_os.h"
#include "rt_os_private.h"

//任务调度函数，只能在非中断中使用
void OS_TASK_SW(void)
{
    char i = 0, j = 0;
    u8 highest_prio_id = 0;
    u8 task_sequence = 0;//当前任务的已运行队列中一定是0
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
    for (; i<TASK_SIZE; i++) { //找到优先级最高任务，并且是在已运行任务队列的最后，如果已运行任务队列中没有则优先运行
        if (os_tcb[i].OSTCBStatus == OS_STAT_RDY) {
            if (os_tcb[i].OSTCBPrio > os_tcb[highest_prio_id].OSTCBPrio) {
                highest_prio_id = i;
                //查找高优先级在已运行队列中的排位
                task_sequence = 0xFF;//先假设这个排在最最最后面
                for (j=0; j<TASK_SIZE; j++) {
                    if (os_task_run[j] == i) {
                        task_sequence = j;
                        break;
                    }
                    if (os_task_run[j] == 0xFF)
                         break;
                }
            } else if (os_tcb[i].OSTCBPrio == os_tcb[highest_prio_id].OSTCBPrio) {
                //查找新找到的高优先级在已运行队列中的排位
                u8 temp_task_sequence = 0xFF;//同优先级使用的临时任务序列
                for (j=0; j<TASK_SIZE; j++) { //查找新的同级任务在任务队列中的排位
                    if (os_task_run[j] == i) {
                        temp_task_sequence = j;
                        break;
                    }
                    if (os_task_run[j] == 0xFF)
                         break;
                }
                if (temp_task_sequence > task_sequence) { //此处我们没有考虑两个相同优先级都没在已运行任务队列中的情况，这种情况下运行第一个被找到的任务
                    highest_prio_id = i;
                    task_sequence = temp_task_sequence;
                }
            }
        }
    }
    os_task_running_ID = highest_prio_id;
    //把当前任务插入已运行任务队列中
    {
        u8 temp_id = os_task_running_ID, temp_temp_id;
        if (task_sequence == 0xFF) { //不在任务队列中，直接头插
            for (j=0; j<TASK_SIZE; j++) {
                if (os_task_run[j] == 0xFF) {
                    os_task_run[j] = temp_id;
                    break;
                }
                temp_temp_id = os_task_run[j];
                os_task_run[j] = temp_id;
                temp_id = temp_temp_id;
            }
        } else { //已在任务队列中，在所在位置前移
            for (j = task_sequence; j>0; j--) {
                os_task_run[j] = os_task_run[j-1];
            }
            os_task_run[0] = os_task_running_ID;
        }
    }
    if (os_tcb[os_task_running_ID].OSTCBTimeQuantaCtr == 0)//给当前运行的时间片赋值
        os_tcb[os_task_running_ID].OSTCBTimeQuantaCtr = os_tcb[os_task_running_ID].OSTCBTimeQuanta;
    os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_RUNNING;
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

/********************* Timer0中断函数************************/
void timer0_int (void) interrupt TIMER0_VECTOR {
    u8 i;
    BOOLEAN need_schedule = FALSE;
#pragma asm
    MOV AR1,AR1
#pragma endasm
    for(i=0; i<TASK_SIZE; i++) { //任务时钟
        if(os_tcb[i].OSTCBDly) {
            os_tcb[i].OSTCBDly--;
            if(os_tcb[i].OSTCBDly == 0) { //当任务时钟到时,必须是由定时器减时的才行
                //os_rdy_tbl |= (0x01<<i); //使任务在就绪表中置位
                os_tcb[i].OSTCBStatus = OS_STAT_RDY;
                need_schedule = TRUE;//因为时间到，可能高级任务准备就绪，需要调度
            }
        }
    }
    //时间片轮转计数逻辑
    if (os_tcb[os_task_running_ID].OSTCBTimeQuantaCtr == 0) { //当前运行任务时间片耗尽，执行中断下任务调度
        need_schedule = TRUE; //因为时间片耗尽，需要调度
    } else { //时间片未到，进行自减
        os_tcb[os_task_running_ID].OSTCBTimeQuantaCtr--;
    }
    if (need_schedule == TRUE) { //调度任务执行代码
        char i = 0, j = 0;
        u8 highest_prio_id = 0;
        u8 task_sequence = 0;//当前任务的已运行队列中一定是0
        //SP -= 2;
        os_tcb[os_task_running_ID].OSTCBStkPtr = SP;
        os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_RDY;
        //切换任务栈
        for (; i<TASK_SIZE; i++) { //找到优先级最高任务，并且是在已运行任务队列的最后，如果已运行任务队列中没有则优先运行
            if (os_tcb[i].OSTCBStatus == OS_STAT_RDY) {
                if (os_tcb[i].OSTCBPrio > os_tcb[highest_prio_id].OSTCBPrio) {
                    highest_prio_id = i;
                    //查找高优先级在已运行队列中的排位
                    task_sequence = 0xFF;//先假设这个排在最最最后面
                    for (j=0; j<TASK_SIZE; j++) {
                        if (os_task_run[j] == i) {
                            task_sequence = j;
                            break;
                        }
                        if (os_task_run[j] == 0xFF)
                         break;
                    }
                } else if (os_tcb[i].OSTCBPrio == os_tcb[highest_prio_id].OSTCBPrio) {
                    //查找新找到的高优先级在已运行队列中的排位
                    u8 temp_task_sequence = 0xFF;//同优先级使用的临时任务序列
                    for (j=0; j<TASK_SIZE; j++) { //查找新的同级任务在任务队列中的排位
                        if (os_task_run[j] == i) {
                            temp_task_sequence = j;
                            break;
                        }
                        if (os_task_run[j] == 0xFF)
                         break;
                    }
                    if (temp_task_sequence > task_sequence) { //此处我们没有考虑两个相同优先级都没在已运行任务队列中的情况，这种情况下运行第一个被找到的任务
                        highest_prio_id = i;
                        task_sequence = temp_task_sequence;
                    }
                }
            }
        }
        os_task_running_ID = highest_prio_id;
        //把当前任务插入已运行任务队列中
        {
            u8 temp_id = os_task_running_ID, temp_temp_id;
            if (task_sequence == 0xFF) { //不在任务队列中，直接头插
                for (j=0; j<TASK_SIZE; j++) {
                    if (os_task_run[j] == 0xFF) {
                        os_task_run[j] = temp_id;
                        break;
                    }
                    temp_temp_id = os_task_run[j];
                    os_task_run[j] = temp_id;
                    temp_id = temp_temp_id;
                }
            } else { //已在任务队列中，在所在位置前移
                for (j = task_sequence; j>0; j--) {
                    os_task_run[j] = os_task_run[j-1];
                }
                os_task_run[0] = os_task_running_ID;
            }
        }
        if (os_tcb[os_task_running_ID].OSTCBTimeQuantaCtr == 0) //给当前运行的时间片赋值
            os_tcb[os_task_running_ID].OSTCBTimeQuantaCtr = os_tcb[os_task_running_ID].OSTCBTimeQuanta;
        os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_RUNNING;
        SP = os_tcb[os_task_running_ID].OSTCBStkPtr;
    }
}
