#include "rt_os.h"
#include "rt_os_private.h"

//��Ϣ���нṹ��
typedef struct os_msgque {
    void          **OSMQStart;          /* ��Ϣ������ʼ��ַ                        */
    void          **OSMQEnd;            /* ָ��������ݽ�β��ָ��                  */
    void          **OSMQIn;             /* ָ����һ����Ϣ��������е�λ��          */
    void          **OSMQOut;            /* ָ��Ӷ�������ȡ��һ����Ϣ��λ�õ�ָ��  */
    u8              OSMQSize;           /* ���д�С�������Ŀ����                  */
    u8              OSMQEntries;        /* �����е�ǰ����Ŀ��                      */
    u8              OSMQPendTbl;        /* �ȴ��ź����������б�                    */
} OS_MQ;

#ifdef MSGQ_ENABLE
static OS_MQ msgque[OS_MSGQ_SIZE];

// ��ʼ����Ϣ����
// OS_ERR_PDATA_NULL�û�ָ��Ϊnull
// OS_ERR_MSGQ_ID_INVALID��Ч����Ϣ����id
u8 os_msgq_init(u8 msgq_index, void **q_start, u8 q_size)
{
    if (q_start == NULL) {
        return OS_ERR_PDATA_NULL;
    }
    if (msgq_index >= OS_MSGQ_SIZE) {
        return OS_ERR_MSGQ_ID_INVALID;
    }
    OS_ENTER_CRITICAL();
    msgque[msgq_index].OSMQStart    = q_start;               /*      Initialize the queue                 */
    msgque[msgq_index].OSMQEnd      = &q_start[q_size];
    msgque[msgq_index].OSMQIn       = q_start;
    msgque[msgq_index].OSMQOut      = q_start;
    msgque[msgq_index].OSMQSize     = q_size;
    msgque[msgq_index].OSMQEntries  = 0u;
    msgque[msgq_index].OSMQPendTbl  = 0u;
    OS_EXIT_CRITICAL();
    return OS_ERR_NONE;
}

// �����Ϣ����
// OS_ERR_PDATA_NULL�û�ָ��Ϊnull
// OS_ERR_MSGQ_ID_INVALID��Ч����Ϣ����id
u8 os_msgq_flush (u8 msgq_index)
{
    if (msgq_index >= OS_MSGQ_SIZE) {
        return OS_ERR_MSGQ_ID_INVALID;
    }
    if (msgque[msgq_index].OSMQStart == NULL) {
        return OS_ERR_PDATA_NULL;
    }
    OS_ENTER_CRITICAL();
    msgque[msgq_index].OSMQIn       = msgque[msgq_index].OSMQStart;
    msgque[msgq_index].OSMQOut      = msgque[msgq_index].OSMQStart;
    msgque[msgq_index].OSMQEntries  = 0u;
    OS_EXIT_CRITICAL();
    return (OS_ERR_NONE);
}

// ����Ϣ���ж�ȡһ����Ϣ
// OS_ERR_PDATA_NULL�û�ָ��Ϊnull
// OS_ERR_MSGQ_ID_INVALID��Ч����Ϣ����id
void *os_msgq_pend (u8 msgq_index, u16 ticks, u8 *err)
{
    void *pmsg;
    if (msgq_index >= OS_MSGQ_SIZE) {
        *err = OS_ERR_MSGQ_ID_INVALID;
        return NULL;
    }
    if (msgque[msgq_index].OSMQStart == NULL) {
        *err = OS_ERR_PDATA_NULL;
        return NULL;
    }
    OS_ENTER_CRITICAL();
    if (msgque[msgq_index].OSMQEntries > 0u) {                          /* See if any messages in the queue                   */
        pmsg = *msgque[msgq_index].OSMQOut++;                           /* Yes, extract oldest message from the queue         */
        msgque[msgq_index].OSMQEntries--;                               /* Update the number of entries in the queue          */
        if (msgque[msgq_index].OSMQOut == msgque[msgq_index].OSMQEnd) { /* Wrap OUT pointer if we are at the end of the queue */
            msgque[msgq_index].OSMQOut = msgque[msgq_index].OSMQStart;
        }
        OS_EXIT_CRITICAL();
        return (pmsg);                                                  /* Return message received                            */
    }
    // û����Ϣ�ɶ�ȡ
    msgque[msgq_index].OSMQPendTbl |= 0x01<<os_task_running_ID;         /* ������Ϣ���е�����ȴ���                           */
    os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_MSGQ;
    os_tcb[os_task_running_ID].OSTCBDly = ticks;
    if (ticks) {
        os_tcb[os_task_running_ID].OSTCBStatPend = OS_STAT_PEND_TO;
    }
    else {
        os_tcb[os_task_running_ID].OSTCBStatPend = OS_STAT_PEND_OK;
    }
    OS_EXIT_CRITICAL();
    OS_TASK_SW(); //���µ���
    
    // ���ٴν���ʱ������OSTCBDlyֵ�ж��Ƿ��ǳ�ʱ���µġ�
    if((os_tcb[os_task_running_ID].OSTCBDly == 0) && (os_tcb[os_task_running_ID].OSTCBStatPend == OS_STAT_PEND_TO)) {
        msgque[msgq_index].OSMQPendTbl &= ~(0x01<<os_task_running_ID);  //���Լ�������ȴ��������
        *err = OS_ERR_TIMEOUT;
        return NULL;
    } else { // ���û�г�ʱ����Ҫ��OSTCBDly��0��������������Ҫ�ĵ���
        OS_ENTER_CRITICAL();
        os_tcb[os_task_running_ID].OSTCBDly = 0;
        msgque[msgq_index].OSMQPendTbl &= ~(0x01<<os_task_running_ID);  //���Լ�������ȴ��������
        pmsg = *msgque[msgq_index].OSMQOut++;                            /* Yes, extract oldest message from the queue         */
        msgque[msgq_index].OSMQEntries--;                                /* Update the number of entries in the queue          */
        if (msgque[msgq_index].OSMQOut == msgque[msgq_index].OSMQEnd) {  /* Wrap OUT pointer if we are at the end of the queue */
            msgque[msgq_index].OSMQOut = msgque[msgq_index].OSMQStart;
        }
        *err = OS_ERR_NONE;
        OS_EXIT_CRITICAL();
    }

    return (pmsg);                                                       /* Return received message                            */
}

// ������Ϣ����Ϣ����
// OS_ERR_PDATA_NULL�û�ָ��Ϊnull
// OS_ERR_MSGQ_ID_INVALID��Ч����Ϣ����id
// OS_ERR_MSGQ_FULL������
u8 os_msgq_post(u8 msgq_index, void *pmsg)
{
    if (msgq_index >= OS_MSGQ_SIZE) {
        return OS_ERR_MSGQ_ID_INVALID;
    }
    if ((pmsg) == NULL) {
        return OS_ERR_PDATA_NULL;
    }

    OS_ENTER_CRITICAL();
    if (msgque[msgq_index].OSMQEntries >= msgque[msgq_index].OSMQSize) {    /* Make sure queue is not full                  */
        OS_EXIT_CRITICAL();
        return (OS_ERR_MSGQ_FULL);
    }
    *msgque[msgq_index].OSMQIn++ = pmsg;                                    /* Insert message into queue                    */
    msgque[msgq_index].OSMQEntries++;                                       /* Update the nbr of entries in the queue       */
    if (msgque[msgq_index].OSMQIn == msgque[msgq_index].OSMQEnd) {          /* Wrap IN ptr if we are at end of queue        */
        msgque[msgq_index].OSMQIn = msgque[msgq_index].OSMQStart;
    }
    if (msgque[msgq_index].OSMQPendTbl != 0) {                              /* See if any task pending on queue             */
        char i = 0, j = 0;
        u8 highest_prio_id = 0;
        u8 task_sequence = 0;//��ǰ����������ж�����һ����0
        // ���ҵȴ�����Ϣ���������ȼ���߲�������ִ��ʱ��Ͽ������������Ϊ����̬
        for (; i<TASK_SIZE; i++) { //�ҵ����ȼ�������񣬲�������������������е����������������������û������������
            if ((msgque[msgq_index].OSMQPendTbl >> i) & 0x01) {
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
    return (OS_ERR_NONE);
}

// �˺�������Ϣ���͵����У�����os_msgq_post��ͬ����Ϣ��������ǰ�˶����Ƕ��е�ĩβ�� ʹ��os_msgq_post_front���Է��͡������ȼ�����Ϣ��
// OS_ERR_PDATA_NULL�û�ָ��Ϊnull
// OS_ERR_MSGQ_ID_INVALID��Ч����Ϣ����id
// OS_ERR_MSGQ_FULL������
u8 os_msgq_post_front(u8 msgq_index, void *pmsg)
{
    if (msgq_index >= OS_MSGQ_SIZE) {
        return OS_ERR_MSGQ_ID_INVALID;
    }
    if ((pmsg) == NULL) {
        return OS_ERR_PDATA_NULL;
    }

    OS_ENTER_CRITICAL();
    if (msgque[msgq_index].OSMQEntries >= msgque[msgq_index].OSMQSize) {   /* Make sure queue is not full                   */
        OS_EXIT_CRITICAL();
        return (OS_ERR_MSGQ_FULL);
    }
    if (msgque[msgq_index].OSMQOut == msgque[msgq_index].OSMQStart) {      /* Wrap OUT ptr if we are at the 1st queue entry */
        msgque[msgq_index].OSMQOut = msgque[msgq_index].OSMQEnd;
    }
    msgque[msgq_index].OSMQOut--;
    *msgque[msgq_index].OSMQOut = pmsg;                                    /* Insert message into queue                      */
    msgque[msgq_index].OSMQEntries++;                                      /* Update the nbr of entries in the queue         */
    
    if (msgque[msgq_index].OSMQPendTbl != 0) {                             /* See if any task pending on queue               */
                                                                           /* Ready highest priority task waiting on event   */
        char i = 0, j = 0;
        u8 highest_prio_id = 0;
        u8 task_sequence = 0;//��ǰ����������ж�����һ����0
        // ���ҵȴ�����Ϣ���������ȼ���߲�������ִ��ʱ��Ͽ������������Ϊ����̬
        for (; i<TASK_SIZE; i++) { //�ҵ����ȼ�������񣬲�������������������е����������������������û������������
            if ((msgque[msgq_index].OSMQPendTbl >> i) & 0x01) {
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
    return (OS_ERR_NONE);
}

// �˺����������Բ鿴��Ϣ�Ƿ���á� ��os_msgq_pend��ͬ�������Ϣ�����ã�os_msgq_accept��������������
// OS_ERR_PDATA_NULL�û�ָ��Ϊnull
// OS_ERR_MSGQ_ID_INVALID��Ч����Ϣ����id
// OS_ERR_MSGQ_EMPTY��Ϣ����Ϊ��
void *os_msgq_accept(u8 msgq_index, u8 *err)
{
    void *pmsg = NULL;
    if (msgq_index >= OS_MSGQ_SIZE) {
        *err = OS_ERR_MSGQ_ID_INVALID;
        return NULL;
    }
    if (msgque[msgq_index].OSMQStart == NULL) {
        *err = OS_ERR_PDATA_NULL;
        return NULL;
    }
    OS_ENTER_CRITICAL();
    if (msgque[msgq_index].OSMQEntries > 0u) {                          /* See if any messages in the queue                   */
        pmsg = *msgque[msgq_index].OSMQOut++;                           /* Yes, extract oldest message from the queue         */
        msgque[msgq_index].OSMQEntries--;                               /* Update the number of entries in the queue          */
        if (msgque[msgq_index].OSMQOut == msgque[msgq_index].OSMQEnd) { /* Wrap OUT pointer if we are at the end of the queue */
            msgque[msgq_index].OSMQOut = msgque[msgq_index].OSMQStart;
        }
        *err = OS_ERR_NONE;
    }
    else {                                                              // û����Ϣ�ɶ�ȡ
        *err = OS_ERR_MSGQ_EMPTY;
    }
    OS_EXIT_CRITICAL();
    return (pmsg);                                                       /* Return received message                            */
}

// �˺�����ȡ��Ϣ���е�����������
// OS_ERR_MSGQ_ID_INVALID��Ч����Ϣ����id
u8 os_msgq_query (u8 msgq_index, u8 *err)
{
    if (msgq_index >= OS_MSGQ_SIZE) {
        *err = OS_ERR_MSGQ_ID_INVALID;
        return 0;
    }
    *err = OS_ERR_NONE;
    return (msgque[msgq_index].OSMQEntries);
}

// �˺�������Ϣ���͵����С� ����Ӵ˵����Լ��ٴ����С����Ϊ�������滻os_msgq_post��os_msgq_post_front��
// optȷ��ִ�е�POST���ͣ�
// OS_POST_OPT_NONE POST�������ȴ�������os_msgq_post��ͬ��
// OS_POST_OPT_FRONT POSTΪLIFO��ģ��os_msgq_post_front��
// OS_POST_OPT_NO_SCHED��ʾ������õ��ȳ���
// ����ֵ
// OS_ERR_PDATA_NULL�û�ָ��Ϊnull
// OS_ERR_MSGQ_ID_INVALID��Ч����Ϣ����id
// OS_ERR_MSGQ_FULL������
u8 os_msgq_post_opt (u8 msgq_index, void *pmsg, u8 opt)
{
    if (msgq_index >= OS_MSGQ_SIZE) {
        return OS_ERR_MSGQ_ID_INVALID;
    }
    if ((pmsg) == NULL) {
        return OS_ERR_PDATA_NULL;
    }

    OS_ENTER_CRITICAL();
    if (msgque[msgq_index].OSMQEntries >= msgque[msgq_index].OSMQSize) {    /* Make sure queue is not full                  */
        OS_EXIT_CRITICAL();
        return (OS_ERR_MSGQ_FULL);
    }
    
    if ((opt & OS_POST_OPT_FRONT) != 0x00u) {         /* Do we post to the FRONT of the queue?         */
        if (msgque[msgq_index].OSMQOut == msgque[msgq_index].OSMQStart) {       /* Yes, Post as LIFO, Wrap OUT pointer if we ... */
            msgque[msgq_index].OSMQOut = msgque[msgq_index].OSMQEnd;            /*      ... are at the 1st queue entry           */
        }
        msgque[msgq_index].OSMQOut--;
        *msgque[msgq_index].OSMQOut = pmsg;                                     /* Insert message into queue                     */
    } else {                                          /* No,  Post as FIFO                             */
        *msgque[msgq_index].OSMQIn++ = pmsg;                                    /* Insert message into queue                     */
        if (msgque[msgq_index].OSMQIn == msgque[msgq_index].OSMQEnd) {          /* Wrap IN ptr if we are at end of queue         */
            msgque[msgq_index].OSMQIn = msgque[msgq_index].OSMQStart;
        }
    }
    msgque[msgq_index].OSMQEntries++;                                           /* Update the nbr of entries in the queue        */
    
    if ((msgque[msgq_index].OSMQPendTbl != 0) && ((opt & OS_POST_OPT_NO_SCHED) == 0u)) { /* See if any task pending on queue               */
        /* Ready highest priority task waiting on event   */
        char i = 0, j = 0;
        u8 highest_prio_id = 0;
        u8 task_sequence = 0;//��ǰ����������ж�����һ����0
        // ���ҵȴ�����Ϣ���������ȼ���߲�������ִ��ʱ��Ͽ������������Ϊ����̬
        for (; i<TASK_SIZE; i++) { //�ҵ����ȼ�������񣬲�������������������е����������������������û������������
            if ((msgque[msgq_index].OSMQPendTbl >> i) & 0x01) {
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
    return (OS_ERR_NONE);
}


#endif
