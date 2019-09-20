#include "rt_os.h"
#include "rt_os_private.h"


u8 OSSchedLockNestingCtr = 0;
u8 os_task_running_ID = 0;
OS_TCB os_tcb[TASK_SIZE] = {0};
u8 os_core_start = 0;
xdata u8 os_task_run[TASK_SIZE];

//����ϵͳ��ʼ��
u8 os_init(void)
{
    u8 i = 0;
    for (; i<TASK_SIZE; i++)
        os_task_run[i] = 0xFF;
    tick_timer_init();
    os_task0_create();
#ifdef MEM_ENABLE
    return os_mem_init();
#else
    return OS_ERR_NONE;
#endif
}

//��ʼ�������,��������ȼ�������Ŀ�ʼ
void os_start_task(void)
{
    char i = 0;
    u8 highest_prio_id = 0;

    tick_timer_start();
    for (; i<TASK_SIZE; i++)//���ﲻ���ж�������������У���Ϊ��������������ǿյģ�֮��Ҫ���е�һ���ҵ���������ȼ�����
    {
        if ((os_tcb[i].OSTCBStatus == OS_STAT_RDY) && (os_tcb[i].OSTCBPrio > os_tcb[highest_prio_id].OSTCBPrio))
            highest_prio_id = i;
    }
    os_task_running_ID = highest_prio_id;
    os_tcb[highest_prio_id].OSTCBStatus = OS_STAT_RUNNING;
    //ϵͳ�״�����һ����Ҫ��ֵʣ��ʱ��Ƭ�������������������һ���ǵ�һ��
    os_tcb[highest_prio_id].OSTCBTimeQuantaCtr = os_tcb[highest_prio_id].OSTCBTimeQuanta;
    os_core_start = 1;//ϵͳ��ʼ���б�־
    SP = os_tcb[highest_prio_id].OSTCBStkPtr - 13;
}

//��������CPUʹ��Ȩ��ͬ�����񣬵�������Ȼ��Ψһ����߼�ʱ���񲻻��л��������������
void os_task_abandon(void)
{
    OS_TASK_SW();
}

//�����˳�ʱ���е���
void os_task_exit(void)
{
    os_tcb[os_task_running_ID].OSTCBTimeQuantaCtr = 0;
    os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_DEAD;
    OS_TASK_SW();
}

//���������ô˺���ȥ����һ������������͵�os_task_suspend()�������ID��Ҫ��������������
//OS_TASKID_SELF����ô������񽫱�����
//������taskID����Ҫ���������ID�����ָ��OS_TASKID_SELF����ô��������Լ������ٷ�����
//�ε��ȡ�
//���أ�OS_ERR_NONE�������������񱻹���
//OS_ERR_TASK_SUSPEND_IDLE�����������������
//OS_ERR_TASK_ID_INVALID��������������ȼ�������
//OS_ERR_TASK_SUSPEND_TASKID����Ҫ��������񲻴��ڡ�
u8  os_task_suspend (u8 taskID)
{
    BOOLEAN self;
    if (taskID == OS_IDLE_TASKID) {
        return (OS_ERR_TASK_SUSPEND_IDLE);
    }
    if (taskID >= TASK_SIZE && taskID != OS_TASKID_SELF) {
        return (OS_ERR_TASK_ID_INVALID);
    }
    
    OS_ENTER_CRITICAL();
    if ((taskID < TASK_SIZE) && (os_tcb[taskID].OSTCBStkPtr == 0)) {
            OS_EXIT_CRITICAL();
            return (OS_ERR_TASK_SUSPEND_TASKID);                         // return��90��
    }

    if (taskID == OS_TASKID_SELF) {
        os_tcb[os_task_running_ID].OSTCBDly = 0;
        //os_rdy_tbl &= ~(0x01<<os_task_running_ID); //�������������־
        os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_SUSPEND;
        self = TRUE;
    } else {
        os_tcb[taskID].OSTCBDly = 0;
        //os_rdy_tbl &= ~(0x01<<taskID); //�������������־
        os_tcb[taskID].OSTCBStatus = OS_STAT_SUSPEND;
        self = FALSE;
    }

    OS_EXIT_CRITICAL();
    if (self == TRUE) {
        OS_TASK_SW();
    }
    return (OS_ERR_NONE);                                        //return��0��
}

//���������ô˺����Իָ���ǰ���������
//������taskID�ǻָ���������ȼ���
//���أ�OS_ERR_NONE�������������񱻻ָ���
//OS_ERR_TASK_ID_INVALID����ָ�����ID������
//OS_ERR_TASK_RESUME_TASKID����Ҫ�ָ������񲻴��ڡ�
//OS_ERR_TASK_NOT_SUSPENDED�����Ҫ�ָ���������δ����
u8  os_task_resume (u8 taskID)
{
    if (taskID >= TASK_SIZE) {                             /* Make sure task priority is valid      */
        return (OS_ERR_TASK_ID_INVALID);
    }

    OS_ENTER_CRITICAL();
    if (os_tcb[taskID].OSTCBStkPtr == 0) {                 /* Task to suspend must exist            */
        OS_EXIT_CRITICAL();
        return (OS_ERR_TASK_RESUME_TASKID);                 // return��90��
    }
    if (os_tcb[taskID].OSTCBStatus != OS_STAT_SUSPEND) {   /* Task must be suspend                  */
        OS_EXIT_CRITICAL();
        return (OS_ERR_TASK_NOT_SUSPENDED);                 // return��101��
    }
    
    os_tcb[taskID].OSTCBDly = 0;
    //os_rdy_tbl |= 0x01<<taskID;                             //����������Ѿ�׼����
    os_tcb[taskID].OSTCBStatus = OS_STAT_RDY;
    OS_EXIT_CRITICAL();
    OS_TASK_SW();

    return (OS_ERR_NONE);
}
