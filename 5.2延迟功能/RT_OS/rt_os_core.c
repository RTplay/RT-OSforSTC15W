#include "rt_os.h"
#include "rt_os_private.h"


u8 os_rdy_tbl = 0;
u8 os_task_running_ID = 0;
OS_TCB os_tcb[TASK_SIZE] = {0};


//����ϵͳ��ʼ��
void os_init(void)
{
    tick_timer_init();
    os_task0_create();
}

//��ʼ�������,��������ȼ�������Ŀ�ʼ
void os_start_task(void)
{
    tick_timer_start();
    os_task_running_ID = 0;
    SP = os_tcb[OS_IDLE_TASKID].OSTCBStkPtr - 13;
}

//���������ô˺���ȥ����һ������������͵�os_task_suspend()�������ID��Ҫ��������������
//OS_TASKID_SELF����ô������񽫱�����
//������taskID����Ҫ���������ID�����ָ��OS_TASKID_SELF����ô��������Լ������ٷ�����
//�ε��ȡ�
//���أ�OS_NO_ERR�������������񱻹���
//OS_TASK_SUSPEND_IDLE�����������������
//OS_TASK_ID_INVALID��������������ȼ�������
//OS_TASK_SUSPEND_TASKID����Ҫ��������񲻴��ڡ�
u8  os_task_suspend (u8 taskID)
{
    BOOLEAN self;
    if (taskID == OS_IDLE_TASKID) {
        return (OS_TASK_SUSPEND_IDLE);
    }
    OS_ENTER_CRITICAL();
    if (taskID >= TASK_SIZE && taskID != OS_TASKID_SELF) {
        OS_EXIT_CRITICAL();
        return (OS_TASK_ID_INVALID);
    }
    if ((taskID < TASK_SIZE) && (os_tcb[taskID].OSTCBStkPtr == 0)) {
            OS_EXIT_CRITICAL();
            return (OS_TASK_SUSPEND_TASKID);                         // return��90��
    }

    if (taskID == OS_TASKID_SELF) {
        os_tcb[os_task_running_ID].OSTCBDly = 0;
        os_rdy_tbl &= ~(0x01<<os_task_running_ID); //�������������־
        self = TRUE;
    } else {
        os_tcb[taskID].OSTCBDly = 0;
        os_rdy_tbl &= ~(0x01<<taskID); //�������������־
        self = FALSE;
    }

    OS_EXIT_CRITICAL();
    if (self == TRUE) {
        OS_TASK_SW();
    }
    return (OS_NO_ERR);                                        //return��0)
}

//���������ô˺����Իָ���ǰ���������
//������taskID�ǻָ���������ȼ���
//���أ�OS_NO_ERR�������������񱻻ָ���
//OS_TASK_ID_INVALID����ָ��������ȼ�������
//OS_TASK_RESUME_TASKID����Ҫ�ָ������񲻴��ڡ�
//OS_TASK_NOT_SUSPENDED�����Ҫ�ָ���������δ����
u8  os_task_resume (u8 taskID)
{
    if (taskID >= TASK_SIZE) {                              /* Make sure task priority is valid      */
        return (OS_TASK_ID_INVALID);
    }

    OS_ENTER_CRITICAL();
    if (os_tcb[taskID].OSTCBStkPtr == 0) {                 /* Task to suspend must exist            */
        OS_EXIT_CRITICAL();
        return (OS_TASK_RESUME_TASKID);                 // return��90��
    }

    os_tcb[taskID].OSTCBDly = 0;
    os_rdy_tbl |= 0x01<<taskID;                             //����������Ѿ�׼����
    OS_EXIT_CRITICAL();
    OS_TASK_SW();

    return (OS_NO_ERR);
}
