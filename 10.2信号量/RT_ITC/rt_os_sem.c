#include "rt_os.h"
#include "rt_os_private.h"

//�������ṹ��
typedef struct os_sem {
    u8              OSSemPendTbl;         /* �ȴ��ź����������б�                                      */
    u8              OSSemCnt;             /* �ź�������                                                */
} OS_SEM;

#ifdef SEM_ENABLE
static OS_SEM sem[OS_SEM_SIZE];

// ��ʼ���ź���
// OS_ERR_SEM_ID_INVALID��Ч���ź���id
u8 os_sem_init(u8 sem_index, u8 initial_value)
{
    if (sem_index >= OS_SEM_SIZE)
        return OS_ERR_SEM_ID_INVALID;
    sem[sem_index].OSSemPendTbl = 0;
    sem[sem_index].OSSemCnt = initial_value;
    return OS_ERR_NONE;
}

// �ȴ�һ���ź���
// ticksΪ��ʱ���ã����Ϊ0�����õȴ�
// OS_ERR_NONE���óɹ������������Ѿ�����һ���ź���
// OS_ERR_TIMEOUTָ���ġ���ʱʱ�䡱�ڵ��ź��������á�
// OS_ERR_SEM_ID_INVALID��Ч���ź���id
u8 os_sem_pend(u8 sem_index, u16 ticks)
{
    if (sem_index >= OS_SEM_SIZE)
        return OS_ERR_SEM_ID_INVALID;

    OS_ENTER_CRITICAL();
    if (sem[sem_index].OSSemCnt > 0) { //�п����ź���
        sem[sem_index].OSSemCnt--;     //����
        OS_EXIT_CRITICAL();
    } else { // �޷���ȡ���ź���
        sem[sem_index].OSSemPendTbl |= 0x01<<os_task_running_ID;  //�����ź���������ȴ���
        os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_SEM;         //���Լ���״̬��Ϊ�ź����ȴ�
        os_tcb[os_task_running_ID].OSTCBDly = ticks; //����ʱΪ 0�������޵ȴ�
        OS_EXIT_CRITICAL();
        OS_TASK_SW(); //���µ���

        // ���ٴν���ʱ������OSTCBDlyֵ�ж��Ƿ��ǳ�ʱ���µġ�
        if(os_tcb[os_task_running_ID].OSTCBDly == 0) {
            sem[sem_index].OSSemPendTbl &= ~(0x01<<os_task_running_ID);  //���Լ�������ȴ��������
            return OS_ERR_TIMEOUT;
        } else { // ���û�г�ʱ����Ҫ��OSTCBDly��0��������������Ҫ�ĵ���
            OS_ENTER_CRITICAL();
            os_tcb[os_task_running_ID].OSTCBDly = 0;
            sem[sem_index].OSSemPendTbl &= ~(0x01<<os_task_running_ID);  //���Լ�������ȴ��������
            sem[sem_index].OSSemCnt--;     //����
            OS_EXIT_CRITICAL();
        }
    }

    return OS_ERR_NONE;
}

// �ͷ�һ���ź���
// OS_ERR_NONE���óɹ�������һ���ź�����
// OS_ERR_SEM_ID_INVALID��Ч���ź���id
u8 os_sem_post(u8 sem_index)
{
    if (sem_index >= OS_SEM_SIZE)
        return OS_ERR_SEM_ID_INVALID;

    OS_ENTER_CRITICAL();
    sem[sem_index].OSSemCnt++;

    if (sem[sem_index].OSSemPendTbl != 0) { //������ȴ��ź���
        char i = 0, j = 0;
        u8 highest_prio_id = 0;
        u8 task_sequence = 0;//��ǰ����������ж�����һ����0
        // ���ҵȴ����ź������������ȼ���߲�������ִ��ʱ��Ͽ������������Ϊ����̬
        for (; i<TASK_SIZE; i++) { //�ҵ����ȼ�������񣬲�������������������е����������������������û������������
            if ((sem[sem_index].OSSemPendTbl >> i) & 0x01) {
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
        os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_RDY; // ���Լ�����Ϊrdy
        OS_EXIT_CRITICAL();
        OS_TASK_SW();
        return OS_ERR_NONE;
    }

    OS_EXIT_CRITICAL();
    return OS_ERR_NONE;
}

// �޵ȴ������ź���
// OS_ERR_NONE���óɹ������������Ѿ�����һ���ź���
// OS_ERR_SEM_UNAVAILABLEû�п��õ��ź���
// OS_ERR_SEM_ID_INVALID��Ч���ź���id
u8 os_sem_accept(u8 sem_index)
{
    if (sem_index >= OS_SEM_SIZE)
        return OS_ERR_SEM_ID_INVALID;
    OS_ENTER_CRITICAL();
    if (sem[sem_index].OSSemCnt > 0) { //�п����ź���
        sem[sem_index].OSSemCnt--;     //����
        OS_EXIT_CRITICAL();
    } else { // �޷���ȡ���ź���
        OS_EXIT_CRITICAL();
        return OS_ERR_SEM_UNAVAILABLE;
    }
    return OS_ERR_NONE;
}

// ��ѯ�ź����ĸ���
// OS_ERR_NONE���óɹ�
// OS_ERR_PDATA_NULL�û�����nullָ��
// OS_ERR_SEM_ID_INVALID��Ч���ź���id
u8 os_sem_query(u8 sem_index, u8 *sem_count)
{
    if (sem_index >= OS_SEM_SIZE)
        return OS_ERR_SEM_ID_INVALID;
    if (sem_count == NULL)
        return OS_ERR_PDATA_NULL;
    OS_ENTER_CRITICAL();
    *sem_count = sem[sem_index].OSSemCnt;
    OS_EXIT_CRITICAL();
    return OS_ERR_NONE;
}

// �����ź�������
// OS_ERR_NONE���óɹ�
// OS_ERR_SEM_ID_INVALID��Ч���ź���id
u8 os_sem_set(u8 sem_index, u8 sem_count)
{
    if (sem_index >= OS_SEM_SIZE)
        return OS_ERR_SEM_ID_INVALID;
    OS_ENTER_CRITICAL();
    sem[sem_index].OSSemCnt = sem_count;
    OS_EXIT_CRITICAL();
    return OS_ERR_NONE;
}

#endif






