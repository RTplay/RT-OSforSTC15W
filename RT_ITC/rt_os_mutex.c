#include "rt_os.h"
#include "rt_os_private.h"

//�������ṹ��
typedef struct os_mutex {
    u8              OSMutexState;           /* ������״̬��0����ռ�ã�1������                          */
    u8              OSMutexPendTbl;         /* �ȴ������������б�                                      */
    u8              OSMutexOwnerTaskID;     /* ������ӵ��������ID                                      */
    u8              OSMutexOwnerPrio;       /* ������ӵ���ߵ����ȼ����������ȼ��ָ�                    */
    u8              OSMutexCnt;             /* ������Ƕ�ײ���������ӵ����Ƕ��ʹ��                      */
} OS_MUTEX;

#ifdef MUTEX_ENABLE
static OS_MUTEX mutex[OS_MUTEX_SIZE];

// ��ʼ��������
// OS_ERR_MUTXE_ID_INVALID��Ч����id
u8 os_mutex_init(u8 mutex_index)
{
    if (mutex_index >= OS_MUTEX_SIZE)
        return OS_ERR_MUTXE_ID_INVALID;
    mutex[mutex_index].OSMutexState = 1;
    mutex[mutex_index].OSMutexPendTbl = 0;
    mutex[mutex_index].OSMutexOwnerTaskID = 0xFF;
    mutex[mutex_index].OSMutexOwnerPrio = 0;
    mutex[mutex_index].OSMutexCnt = 0;
    return OS_ERR_NONE;
}

// ����ʹ��һ��������
// ticksΪ��ʱ���ã����Ϊ0�����õȴ�
// OS_ERR_NONE���óɹ�����������ӵ�л�����
// OS_ERR_TIMEOUTָ���ġ���ʱʱ�䡱�ڵĻ����������á�
// OS_ERR_MUTXE_ID_INVALID��Ч����id
u8 os_mutex_pend(u8 mutex_index, u16 ticks)
{
    if (mutex_index >= OS_MUTEX_SIZE)
        return OS_ERR_MUTXE_ID_INVALID;

    OS_ENTER_CRITICAL();
    if (mutex[mutex_index].OSMutexOwnerTaskID == os_task_running_ID) { //�������Ƕ����
        mutex[mutex_index].OSMutexCnt++;
        OS_EXIT_CRITICAL();
    } else if (mutex[mutex_index].OSMutexState == 1) { //��������Ч
        mutex[mutex_index].OSMutexState = 0;      //��������ռ��������
        mutex[mutex_index].OSMutexOwnerTaskID = os_task_running_ID;
        mutex[mutex_index].OSMutexOwnerPrio = os_tcb[os_task_running_ID].OSTCBPrio;
        mutex[mutex_index].OSMutexCnt++;
        OS_EXIT_CRITICAL();
    } else { // �޷���ȡ����
        mutex[mutex_index].OSMutexPendTbl |= 0x01<<os_task_running_ID;  //���뻥����������ȴ���
        os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_MUTEX;         //���Լ���״̬��Ϊ�������ȴ�
        os_tcb[os_task_running_ID].OSTCBDly = ticks; //����ʱΪ 0�������޵ȴ�
        if (os_tcb[os_task_running_ID].OSTCBPrio > mutex[mutex_index].OSMutexOwnerPrio) { //���ӵ���ߵ����ȼ���
            os_tcb[(mutex[mutex_index].OSMutexOwnerTaskID)].OSTCBPrio = os_tcb[os_task_running_ID].OSTCBPrio; //��������ӵ�������ȼ����������������
        }
        OS_EXIT_CRITICAL();
        OS_TASK_SW(); //���µ���

        // ���ٴν���ʱ������OSTCBDlyֵ�ж��Ƿ��ǳ�ʱ���µġ�
        if(os_tcb[os_task_running_ID].OSTCBDly == 0) {
            mutex[mutex_index].OSMutexPendTbl &= ~(0x01<<os_task_running_ID);  //���Լ�������ȴ��������
            return OS_ERR_TIMEOUT;
        } else { // ���û�г�ʱ����Ҫ��OSTCBDly��0��������������Ҫ�ĵ���
            OS_ENTER_CRITICAL();
            os_tcb[os_task_running_ID].OSTCBDly = 0;
            mutex[mutex_index].OSMutexState = 0;      //��������ռ��������
            mutex[mutex_index].OSMutexOwnerTaskID = os_task_running_ID;
            mutex[mutex_index].OSMutexOwnerPrio = os_tcb[os_task_running_ID].OSTCBPrio;
            mutex[mutex_index].OSMutexPendTbl &= ~(0x01<<os_task_running_ID);  //���Լ�������ȴ��������
            mutex[mutex_index].OSMutexCnt++;
            OS_EXIT_CRITICAL();
        }
    }

    return OS_ERR_NONE;
}

// �ͷŻ�����
// OS_ERR_NONE���óɹ������������źš�
// OS_ERR_NOT_MUTEX_OWNER�ͷŻ�������������MUTEX�������ߡ�
// OS_ERR_NOT_MUTEXδ����
// OS_ERR_MUTXE_ID_INVALID��Ч����id
u8 os_mutex_post(u8 mutex_index)
{
    if (mutex_index >= OS_MUTEX_SIZE)
        return OS_ERR_MUTXE_ID_INVALID;
    if (mutex[mutex_index].OSMutexOwnerTaskID != os_task_running_ID)
        return OS_ERR_NOT_MUTEX_OWNER;
    if (mutex[mutex_index].OSMutexState == 1)
        return OS_ERR_NOT_MUTEX;

    OS_ENTER_CRITICAL();
    mutex[mutex_index].OSMutexCnt--;
    if (mutex[mutex_index].OSMutexCnt == 0) { // Ƕ��������
        if (mutex[mutex_index].OSMutexPendTbl == 0) { //������ȴ�������
            mutex[mutex_index].OSMutexState = 1;      //����������
            mutex[mutex_index].OSMutexOwnerTaskID = 0xFF;
            mutex[mutex_index].OSMutexOwnerPrio = 0;
        } 
        else { // ������ȴ�������
            char i = 0, j = 0;
            u8 highest_prio_id = 0;
            u8 task_sequence = 0;//��ǰ����������ж�����һ����0
            mutex[mutex_index].OSMutexState = 1;      //����������
            mutex[mutex_index].OSMutexOwnerTaskID = 0xFF;
            // ���ҵȴ��û��������������ȼ���߲�������ִ��ʱ��Ͽ������������Ϊ����̬
            for (; i<TASK_SIZE; i++) { //�ҵ����ȼ�������񣬲�������������������е����������������������û������������
                if ((mutex[mutex_index].OSMutexPendTbl >> i) & 0x01) {
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
            // ��������ȼ�������״̬��λrdy���������������
            os_tcb[highest_prio_id].OSTCBStatus = OS_STAT_RDY;
            os_tcb[os_task_running_ID].OSTCBPrio = mutex[mutex_index].OSMutexOwnerPrio; // �޸��Լ������ȼ������ҽ��е���
            os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_RDY;
            OS_EXIT_CRITICAL();
            OS_TASK_SW();
            return OS_ERR_NONE;
        }
    }

    OS_EXIT_CRITICAL();
    return OS_ERR_NONE;
}

// �޵ȴ����󻥳���
// OS_ERR_NONE���óɹ�����������ӵ�л�����
// OS_ERR_PEND_LOCKED���Ѿ���ռ��
// OS_ERR_MUTXE_ID_INVALID��Ч����id
u8 os_mutex_accept(u8 mutex_index)
{
    if (mutex_index >= OS_MUTEX_SIZE)
        return OS_ERR_MUTXE_ID_INVALID;
    OS_ENTER_CRITICAL();
    if (mutex[mutex_index].OSMutexOwnerTaskID == os_task_running_ID) { //�������Ƕ����
        mutex[mutex_index].OSMutexCnt++;
        OS_EXIT_CRITICAL();
    } else if (mutex[mutex_index].OSMutexState == 1) { //��������Ч
        mutex[mutex_index].OSMutexState = 0;      //��������ռ��������
        mutex[mutex_index].OSMutexOwnerTaskID = os_task_running_ID;
        mutex[mutex_index].OSMutexOwnerPrio = os_tcb[os_task_running_ID].OSTCBPrio;
        mutex[mutex_index].OSMutexCnt++;
        OS_EXIT_CRITICAL();
    } else { // �޷���ȡ����
        OS_EXIT_CRITICAL();
        return OS_ERR_PEND_LOCKED;
    }

    return OS_ERR_NONE;
}

// ��ѯ����״̬
// OS_ERR_PEND_UNLOCK��δ��ռ��
// OS_ERR_PEND_LOCKED���Ѿ���ռ��
// OS_ERR_MUTXE_ID_INVALID��Ч����id
u8 os_mutex_query(u8 mutex_index)
{
    if (mutex_index >= OS_MUTEX_SIZE)
        return OS_ERR_MUTXE_ID_INVALID;
    OS_ENTER_CRITICAL();
    if (mutex[mutex_index].OSMutexState == 1) {
        OS_EXIT_CRITICAL();
        return OS_ERR_PEND_UNLOCK;
    }
    else {
        OS_EXIT_CRITICAL();
        return OS_ERR_PEND_LOCKED;
    }
}

#endif