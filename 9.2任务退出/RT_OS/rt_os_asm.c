#include "rt_os.h"
#include "rt_os_private.h"

//������Ⱥ�����ֻ���ڷ��ж���ʹ��
void OS_TASK_SW(void)
{
    char i = 0, j = 0;
    u8 highest_prio_id = 0;
    u8 task_sequence = 0;//��ǰ����������ж�����һ����0
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
//�л�����ջ
    for (; i<TASK_SIZE; i++) { //�ҵ����ȼ�������񣬲�������������������е����������������������û������������
        if (os_tcb[i].OSTCBStatus == OS_STAT_RDY) {
            if (os_tcb[i].OSTCBPrio > os_tcb[highest_prio_id].OSTCBPrio) {
                highest_prio_id = i;
                //���Ҹ����ȼ��������ж����е���λ
                task_sequence = 0xFF;//�ȼ�������������������
                for (j=0; j<TASK_SIZE; j++) {
                    if (os_task_run[j] == i) {
                        task_sequence = j;
                        break;
                    }
                    if (os_task_run[j] == 0xFF)
                         break;
                }
            } else if (os_tcb[i].OSTCBPrio == os_tcb[highest_prio_id].OSTCBPrio) {
                //�������ҵ��ĸ����ȼ��������ж����е���λ
                u8 temp_task_sequence = 0xFF;//ͬ���ȼ�ʹ�õ���ʱ��������
                for (j=0; j<TASK_SIZE; j++) { //�����µ�ͬ����������������е���λ
                    if (os_task_run[j] == i) {
                        temp_task_sequence = j;
                        break;
                    }
                    if (os_task_run[j] == 0xFF)
                         break;
                }
                if (temp_task_sequence > task_sequence) { //�˴�����û�п���������ͬ���ȼ���û����������������е������������������е�һ�����ҵ�������
                    highest_prio_id = i;
                    task_sequence = temp_task_sequence;
                }
            }
        }
    }
    os_task_running_ID = highest_prio_id;
    //�ѵ�ǰ����������������������
    {
        u8 temp_id = os_task_running_ID, temp_temp_id;
        if (task_sequence == 0xFF) { //������������У�ֱ��ͷ��
            for (j=0; j<TASK_SIZE; j++) {
                if (os_task_run[j] == 0xFF) {
                    os_task_run[j] = temp_id;
                    break;
                }
                temp_temp_id = os_task_run[j];
                os_task_run[j] = temp_id;
                temp_id = temp_temp_id;
            }
        } else { //������������У�������λ��ǰ��
            for (j = task_sequence; j>0; j--) {
                os_task_run[j] = os_task_run[j-1];
            }
            os_task_run[0] = os_task_running_ID;
        }
    }
    if (os_tcb[os_task_running_ID].OSTCBTimeQuantaCtr == 0)//����ǰ���е�ʱ��Ƭ��ֵ
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

/********************* Timer0�жϺ���************************/
void timer0_int (void) interrupt TIMER0_VECTOR {
    u8 i;
    BOOLEAN need_schedule = FALSE;
#pragma asm
    MOV AR1,AR1
#pragma endasm
    for(i=0; i<TASK_SIZE; i++) { //����ʱ��
        if(os_tcb[i].OSTCBDly) {
            os_tcb[i].OSTCBDly--;
            if(os_tcb[i].OSTCBDly == 0) { //������ʱ�ӵ�ʱ,�������ɶ�ʱ����ʱ�Ĳ���
                //os_rdy_tbl |= (0x01<<i); //ʹ�����ھ���������λ
                os_tcb[i].OSTCBStatus = OS_STAT_RDY;
                need_schedule = TRUE;//��Ϊʱ�䵽�����ܸ߼�����׼����������Ҫ����
            }
        }
    }
    //ʱ��Ƭ��ת�����߼�
    if (os_tcb[os_task_running_ID].OSTCBTimeQuantaCtr == 0) { //��ǰ��������ʱ��Ƭ�ľ���ִ���ж����������
        need_schedule = TRUE; //��Ϊʱ��Ƭ�ľ�����Ҫ����
    } else { //ʱ��Ƭδ���������Լ�
        os_tcb[os_task_running_ID].OSTCBTimeQuantaCtr--;
    }
    if (need_schedule == TRUE) { //��������ִ�д���
        char i = 0, j = 0;
        u8 highest_prio_id = 0;
        u8 task_sequence = 0;//��ǰ����������ж�����һ����0
        //SP -= 2;
        os_tcb[os_task_running_ID].OSTCBStkPtr = SP;
        os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_RDY;
        //�л�����ջ
        for (; i<TASK_SIZE; i++) { //�ҵ����ȼ�������񣬲�������������������е����������������������û������������
            if (os_tcb[i].OSTCBStatus == OS_STAT_RDY) {
                if (os_tcb[i].OSTCBPrio > os_tcb[highest_prio_id].OSTCBPrio) {
                    highest_prio_id = i;
                    //���Ҹ����ȼ��������ж����е���λ
                    task_sequence = 0xFF;//�ȼ�������������������
                    for (j=0; j<TASK_SIZE; j++) {
                        if (os_task_run[j] == i) {
                            task_sequence = j;
                            break;
                        }
                        if (os_task_run[j] == 0xFF)
                         break;
                    }
                } else if (os_tcb[i].OSTCBPrio == os_tcb[highest_prio_id].OSTCBPrio) {
                    //�������ҵ��ĸ����ȼ��������ж����е���λ
                    u8 temp_task_sequence = 0xFF;//ͬ���ȼ�ʹ�õ���ʱ��������
                    for (j=0; j<TASK_SIZE; j++) { //�����µ�ͬ����������������е���λ
                        if (os_task_run[j] == i) {
                            temp_task_sequence = j;
                            break;
                        }
                        if (os_task_run[j] == 0xFF)
                         break;
                    }
                    if (temp_task_sequence > task_sequence) { //�˴�����û�п���������ͬ���ȼ���û����������������е������������������е�һ�����ҵ�������
                        highest_prio_id = i;
                        task_sequence = temp_task_sequence;
                    }
                }
            }
        }
        os_task_running_ID = highest_prio_id;
        //�ѵ�ǰ����������������������
        {
            u8 temp_id = os_task_running_ID, temp_temp_id;
            if (task_sequence == 0xFF) { //������������У�ֱ��ͷ��
                for (j=0; j<TASK_SIZE; j++) {
                    if (os_task_run[j] == 0xFF) {
                        os_task_run[j] = temp_id;
                        break;
                    }
                    temp_temp_id = os_task_run[j];
                    os_task_run[j] = temp_id;
                    temp_id = temp_temp_id;
                }
            } else { //������������У�������λ��ǰ��
                for (j = task_sequence; j>0; j--) {
                    os_task_run[j] = os_task_run[j-1];
                }
                os_task_run[0] = os_task_running_ID;
            }
        }
        if (os_tcb[os_task_running_ID].OSTCBTimeQuantaCtr == 0) //����ǰ���е�ʱ��Ƭ��ֵ
            os_tcb[os_task_running_ID].OSTCBTimeQuantaCtr = os_tcb[os_task_running_ID].OSTCBTimeQuanta;
        os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_RUNNING;
        SP = os_tcb[os_task_running_ID].OSTCBStkPtr;
    }
}
