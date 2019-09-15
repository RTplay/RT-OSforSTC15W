#include "rt_os.h"
#include "rt_os_private.h"

//消息队列结构体
typedef struct os_msgque {
    void          **OSMQStart;          /* 消息队列起始地址                        */
    void          **OSMQEnd;            /* 指向队列数据结尾的指针                  */
    void          **OSMQIn;             /* 指向下一条消息将插入队列的位置          */
    void          **OSMQOut;            /* 指向从队列中提取下一条消息的位置的指针  */
    u8              OSMQSize;           /* 队列大小（最大条目数）                  */
    u8              OSMQEntries;        /* 队列中当前的条目数                      */
    u8              OSMQPendTbl;        /* 等待信号量的任务列表                    */
} OS_MQ;

#ifdef MSGQ_ENABLE
static OS_MQ msgque[OS_MSGQ_SIZE];

// 初始化消息队列
// OS_ERR_PDATA_NULL用户指针为null
// OS_ERR_MSGQ_ID_INVALID无效的消息队列id
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

// 清空消息队列
// OS_ERR_PDATA_NULL用户指针为null
// OS_ERR_MSGQ_ID_INVALID无效的消息队列id
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

// 从消息队列读取一个消息
// OS_ERR_PDATA_NULL用户指针为null
// OS_ERR_MSGQ_ID_INVALID无效的消息队列id
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
    // 没有消息可读取
    msgque[msgq_index].OSMQPendTbl |= 0x01<<os_task_running_ID;         /* 加入消息队列的任务等待表                           */
    os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_MSGQ;
    os_tcb[os_task_running_ID].OSTCBDly = ticks;
    if (ticks) {
        os_tcb[os_task_running_ID].OSTCBStatPend = OS_STAT_PEND_TO;
    }
    else {
        os_tcb[os_task_running_ID].OSTCBStatPend = OS_STAT_PEND_OK;
    }
    OS_EXIT_CRITICAL();
    OS_TASK_SW(); //从新调度
    
    // 当再次进入时，根据OSTCBDly值判断是否是超时导致的。
    if((os_tcb[os_task_running_ID].OSTCBDly == 0) && (os_tcb[os_task_running_ID].OSTCBStatPend == OS_STAT_PEND_TO)) {
        msgque[msgq_index].OSMQPendTbl &= ~(0x01<<os_task_running_ID);  //将自己从任务等待表中清除
        *err = OS_ERR_TIMEOUT;
        return NULL;
    } else { // 如果没有超时，需要将OSTCBDly置0，以免引发不必要的调度
        OS_ENTER_CRITICAL();
        os_tcb[os_task_running_ID].OSTCBDly = 0;
        msgque[msgq_index].OSMQPendTbl &= ~(0x01<<os_task_running_ID);  //将自己从任务等待表中清除
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

// 发送消息到消息队列
// OS_ERR_PDATA_NULL用户指针为null
// OS_ERR_MSGQ_ID_INVALID无效的消息队列id
// OS_ERR_MSGQ_FULL队列满
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
        u8 task_sequence = 0;//当前任务的已运行队列中一定是0
        // 查找等待该消息中任务优先级最高并且任务被执行时间较靠后的任务，设置为就绪态
        for (; i<TASK_SIZE; i++) { //找到优先级最高任务，并且是在已运行任务队列的最后，如果已运行任务队列中没有则优先运行
            if ((msgque[msgq_index].OSMQPendTbl >> i) & 0x01) {
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
        // 将最高优先级的任务状态置位rdy，并进行任务调度
        os_tcb[highest_prio_id].OSTCBStatus = OS_STAT_RDY;
        os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_RDY; // 将自己设置为rdy
        OS_EXIT_CRITICAL();
        OS_TASK_SW();
        return OS_ERR_NONE;
    }
    OS_EXIT_CRITICAL();
    return (OS_ERR_NONE);
}

// 此函数将消息发送到队列，但与os_msgq_post不同，消息将发布在前端而不是队列的末尾。 使用os_msgq_post_front可以发送“高优先级”消息。
// OS_ERR_PDATA_NULL用户指针为null
// OS_ERR_MSGQ_ID_INVALID无效的消息队列id
// OS_ERR_MSGQ_FULL队列满
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
        u8 task_sequence = 0;//当前任务的已运行队列中一定是0
        // 查找等待该消息中任务优先级最高并且任务被执行时间较靠后的任务，设置为就绪态
        for (; i<TASK_SIZE; i++) { //找到优先级最高任务，并且是在已运行任务队列的最后，如果已运行任务队列中没有则优先运行
            if ((msgque[msgq_index].OSMQPendTbl >> i) & 0x01) {
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
        // 将最高优先级的任务状态置位rdy，并进行任务调度
        os_tcb[highest_prio_id].OSTCBStatus = OS_STAT_RDY;
        os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_RDY; // 将自己设置为rdy
        OS_EXIT_CRITICAL();
        OS_TASK_SW();
        return OS_ERR_NONE;
    }
    
    OS_EXIT_CRITICAL();
    return (OS_ERR_NONE);
}

// 此函数检查队列以查看消息是否可用。 与os_msgq_pend不同，如果消息不可用，os_msgq_accept不会挂起调用任务。
// OS_ERR_PDATA_NULL用户指针为null
// OS_ERR_MSGQ_ID_INVALID无效的消息队列id
// OS_ERR_MSGQ_EMPTY消息队列为空
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
    else {                                                              // 没有消息可读取
        *err = OS_ERR_MSGQ_EMPTY;
    }
    OS_EXIT_CRITICAL();
    return (pmsg);                                                       /* Return received message                            */
}

// 此函数获取消息队列的数据数量。
// OS_ERR_MSGQ_ID_INVALID无效的消息队列id
u8 os_msgq_query (u8 msgq_index, u8 *err)
{
    if (msgq_index >= OS_MSGQ_SIZE) {
        *err = OS_ERR_MSGQ_ID_INVALID;
        return 0;
    }
    *err = OS_ERR_NONE;
    return (msgque[msgq_index].OSMQEntries);
}

// 此函数将消息发送到队列。 已添加此调用以减少代码大小，因为它可以替换os_msgq_post和os_msgq_post_front。
// opt确定执行的POST类型：
// OS_POST_OPT_NONE POST到单个等待任务（与os_msgq_post相同）
// OS_POST_OPT_FRONT POST为LIFO（模拟os_msgq_post_front）
// OS_POST_OPT_NO_SCHED表示不会调用调度程序
// 返回值
// OS_ERR_PDATA_NULL用户指针为null
// OS_ERR_MSGQ_ID_INVALID无效的消息队列id
// OS_ERR_MSGQ_FULL队列满
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
        u8 task_sequence = 0;//当前任务的已运行队列中一定是0
        // 查找等待该消息中任务优先级最高并且任务被执行时间较靠后的任务，设置为就绪态
        for (; i<TASK_SIZE; i++) { //找到优先级最高任务，并且是在已运行任务队列的最后，如果已运行任务队列中没有则优先运行
            if ((msgque[msgq_index].OSMQPendTbl >> i) & 0x01) {
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
        // 将最高优先级的任务状态置位rdy，并进行任务调度
        os_tcb[highest_prio_id].OSTCBStatus = OS_STAT_RDY;
        os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_RDY; // 将自己设置为rdy
        OS_EXIT_CRITICAL();
        OS_TASK_SW();
        return OS_ERR_NONE;
    }

    OS_EXIT_CRITICAL();
    return (OS_ERR_NONE);
}


#endif
