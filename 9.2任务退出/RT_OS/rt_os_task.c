#include "rt_os.h"
#include "rt_os_private.h"
#include    "debug_uart.h"
#include	"delay.h"

//��������
u8 os_task_create(void (*task)(void), u8 taskID, u8 task_prio, u8 time_quanta, u8 *pstack, u8 stack_size)
{
    if (taskID >= TASK_SIZE)
        return (OS_TASK_ID_INVALID);
    if (task_prio == OS_IDLE_TASK_PRIO)//��������idle��ͬ���ȼ�
        return (OS_PRIO_INVALID);
    if (os_tcb[taskID].OSTCBStkPtr != 0)
        return (OS_TASK_ID_EXIST);
    OS_ENTER_CRITICAL();
#ifdef STACK_DETECT_MODE
{
    u8 i = 0;
    for (; i<stack_size; i++)
        pstack[i] = STACK_MAGIC;
}
#else
    stack_size = stack_size; //�������뾯��
#endif
    *pstack++ = (u16)task; //�������ĵ�ַ��λѹ���ջ��
    *pstack = (u16)task>>8; //�������ĵ�ַ��λѹ���ջ��
{
    u8 i = 0;
    for (; i < 13; i++)        //���α�������Ⱥ���popʱ��Ҫ��ն�ջ���ݣ�����ԭ�����ݵ��´���
        *(++pstack) = 0;
}
    os_tcb[taskID].OSTCBStkPtr = (u8)pstack; //���˹���ջ��ջ�������浽��ջ��������
    //os_rdy_tbl |= 0x01<<taskID; //����������Ѿ�׼����
    os_tcb[taskID].OSTCBStatus = OS_STAT_RDY;
    os_tcb[taskID].OSTCBPrio = task_prio;
    if ( time_quanta == 0 )
        os_tcb[taskID].OSTCBTimeQuanta = OS_DEFAULT_TIME_QUANTA;
    else
        os_tcb[taskID].OSTCBTimeQuanta = time_quanta;
    if (os_core_start) { //��������Ѿ����������һ���������
        os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_RDY; //�Ƚ��Լ���λ����̬
        OS_EXIT_CRITICAL();
        OS_TASK_SW();
    }
    else
        OS_EXIT_CRITICAL();
    return (OS_NO_ERR);
}

// �˹�����������̬������������ȼ��� ��ע�⣬�µ����ȼ�������á�
// ���أ�OS_NO_ERR:�ǳɹ�
// OS_TASK_ID_INVALID�������ͼ�ı�idle��������ȼ��������񲻴��ڻ�������ID���������������
// OS_PRIO_INVALID: �����ͼ����Ϊidle��������ȼ���0���ǲ������
// OS_PRIO_ERR: ��������
u8 os_task_change_prio (u8 taskID, u8 new_prio)
{
    if (taskID == OS_IDLE_TASKID)
        return (OS_TASK_ID_INVALID);
    if (new_prio == OS_IDLE_TASK_PRIO)
        return (OS_PRIO_INVALID);  
    if (taskID >= TASK_SIZE && taskID != OS_TASKID_SELF)
        return (OS_TASK_ID_INVALID);
    
    OS_ENTER_CRITICAL();
    if ((taskID < TASK_SIZE) && (os_tcb[taskID].OSTCBStkPtr == 0)) {
            OS_EXIT_CRITICAL();
            return (OS_TASK_ID_INVALID);
    }

    if ((taskID == OS_TASKID_SELF) || (taskID == os_task_running_ID)){
        os_tcb[os_task_running_ID].OSTCBPrio = new_prio;
        OS_EXIT_CRITICAL();
    } else {
        os_tcb[taskID].OSTCBPrio = new_prio;
        os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_RDY;
        OS_EXIT_CRITICAL();
        OS_TASK_SW();
    }

    return (OS_NO_ERR);                                        //return��0)
}



/************************************************************************************************/
/*************************************����������ش���*******************************************/
idle_user idle_user_func = NULL;
//��������
void idle_task_0(void)
{
    debug_print("idle_task_0 in\r\n");
    while (1) {
        char i = 0;
        u8 highest_prio_id = 0;
        if (idle_user_func)
            idle_user_func();
        OS_ENTER_CRITICAL();
        os_tcb[OS_IDLE_TASKID].OSTCBStatus = OS_STAT_RDY;
        for (; i<TASK_SIZE; i++) {
            if ((os_tcb[i].OSTCBStatus == OS_STAT_RDY) && (os_tcb[i].OSTCBPrio > os_tcb[highest_prio_id].OSTCBPrio))
                highest_prio_id = i;
        }
        if (highest_prio_id != 0) {//������ǿ��������Լ����ͽ����л�����
            OS_EXIT_CRITICAL();
            OS_TASK_SW ();
        }
        else {
            OS_EXIT_CRITICAL();
        }
    }
}

#define TASK0_STACK_LEN 20
static STACK_TYPE idle_stack[TASK0_STACK_LEN]; //����һ�� 20 �ֽڵľ�̬����ջ

void os_task0_create(void)
{
    u8* stack = idle_stack;
    OS_ENTER_CRITICAL();
#ifdef STACK_DETECT_MODE
{
    u8 i = 0;
    for (; i<TASK0_STACK_LEN; i++)
        stack[i] = STACK_MAGIC;
}
#endif
    *stack++ = (u16)idle_task_0; //�������ĵ�ַ��λѹ���ջ��
    *stack = (u16)idle_task_0>>8; //�������ĵ�ַ��λѹ���ջ��
{
    u8 i = 0;
    for (; i < 13; i++)        //���α�������Ⱥ���popʱ��Ҫ��ն�ջ���ݣ�����ԭ�����ݵ��´���
        *(++stack) = 0;
}
    os_tcb[OS_IDLE_TASKID].OSTCBStkPtr = (u8)stack; //���˹���ջ��ջ�������浽��ջ��������
    os_tcb[OS_IDLE_TASKID].OSTCBStatus = OS_STAT_RDY;
    os_tcb[OS_IDLE_TASKID].OSTCBPrio = OS_IDLE_TASK_PRIO;
    os_tcb[OS_IDLE_TASKID].OSTCBTimeQuanta = OS_DEFAULT_TIME_QUANTA;
    OS_EXIT_CRITICAL();
}

//ע����������û��ӿ�
void os_registered_idle_user(idle_user idle_func)
{
    idle_user_func = idle_func;
}

/************************************************************************************************/
/*************************************��ջͳ����ش���*******************************************/
#ifdef STACK_DETECT_MODE
u8 get_stack_used(u8 *pstack, u8 stack_size)
{
    u8 i = stack_size-1;
    u8 unused = 0;
    while (STACK_MAGIC == pstack[i]) {
        unused++;
        if (0 == i)
            break;
        else
            i--;
    }
    return stack_size - unused;
}
#endif
