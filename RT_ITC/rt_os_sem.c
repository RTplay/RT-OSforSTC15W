#include "rt_os.h"
#include "rt_os_private.h"

//互斥锁结构体
typedef struct os_sem {
    u8              OSSemPendTbl;         /* 等待信号量的任务列表                                      */
    u8              OSSemCnt;             /* 信号量计数                                                */
} OS_SEM;

#ifdef SEM_ENABLE
static OS_SEM sem[OS_SEM_SIZE];

// 初始化信号量
// OS_ERR_SEM_ID_INVALID无效的信号量id
u8 os_sem_init(u8 sem_index, u8 initial_value)
{
    if (sem_index >= OS_SEM_SIZE)
        return OS_ERR_SEM_ID_INVALID;
    sem[sem_index].OSSemPendTbl = 0;
    sem[sem_index].OSSemCnt = initial_value;
    return OS_ERR_NONE;
}

// 等待一个信号量
// ticks为超时设置，如果为0则永久等待
// OS_ERR_NONE调用成功，您的任务已经消费一个信号量
// OS_ERR_TIMEOUT指定的“超时时间”内的信号量不可用。
// OS_ERR_SEM_ID_INVALID无效的信号量id
u8 os_sem_pend(u8 sem_index, u16 ticks)
{
    if (sem_index >= OS_SEM_SIZE)
        return OS_ERR_SEM_ID_INVALID;

    OS_ENTER_CRITICAL();
    if (sem[sem_index].OSSemCnt > 0) { //有可用信号量
        sem[sem_index].OSSemCnt--;     //消费
        OS_EXIT_CRITICAL();
    } else { // 无法获取到信号量
        sem[sem_index].OSSemPendTbl |= 0x01<<os_task_running_ID;  //加入信号量的任务等待表
        os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_SEM;         //将自己的状态改为信号量等待
        os_tcb[os_task_running_ID].OSTCBDly = ticks; //如延时为 0，刚无限等待
        OS_EXIT_CRITICAL();
        OS_TASK_SW(); //从新调度

        // 当再次进入时，根据OSTCBDly值判断是否是超时导致的。
        if(os_tcb[os_task_running_ID].OSTCBDly == 0) {
            sem[sem_index].OSSemPendTbl &= ~(0x01<<os_task_running_ID);  //将自己从任务等待表中清除
            return OS_ERR_TIMEOUT;
        } else { // 如果没有超时，需要将OSTCBDly置0，以免引发不必要的调度
            OS_ENTER_CRITICAL();
            os_tcb[os_task_running_ID].OSTCBDly = 0;
            sem[sem_index].OSSemPendTbl &= ~(0x01<<os_task_running_ID);  //将自己从任务等待表中清除
            sem[sem_index].OSSemCnt--;     //消费
            OS_EXIT_CRITICAL();
        }
    }

    return OS_ERR_NONE;
}

// 释放一个信号量
// OS_ERR_NONE调用成功并发出一个信号量。
// OS_ERR_SEM_ID_INVALID无效的信号量id
u8 os_sem_post(u8 sem_index)
{
    if (sem_index >= OS_SEM_SIZE)
        return OS_ERR_SEM_ID_INVALID;

    OS_ENTER_CRITICAL();
    sem[sem_index].OSSemCnt++;

    if (sem[sem_index].OSSemPendTbl != 0) { //有任务等待信号量
        char i = 0, j = 0;
        u8 highest_prio_id = 0;
        u8 task_sequence = 0;//当前任务的已运行队列中一定是0
        // 查找等待该信号量中任务优先级最高并且任务被执行时间较靠后的任务，设置为就绪态
        for (; i<TASK_SIZE; i++) { //找到优先级最高任务，并且是在已运行任务队列的最后，如果已运行任务队列中没有则优先运行
            if ((sem[sem_index].OSSemPendTbl >> i) & 0x01) {
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
    return OS_ERR_NONE;
}

// 无等待请求信号量
// OS_ERR_NONE调用成功，您的任务已经消费一个信号量
// OS_ERR_SEM_UNAVAILABLE没有可用的信号量
// OS_ERR_SEM_ID_INVALID无效的信号量id
u8 os_sem_accept(u8 sem_index)
{
    if (sem_index >= OS_SEM_SIZE)
        return OS_ERR_SEM_ID_INVALID;
    OS_ENTER_CRITICAL();
    if (sem[sem_index].OSSemCnt > 0) { //有可用信号量
        sem[sem_index].OSSemCnt--;     //消费
        OS_EXIT_CRITICAL();
    } else { // 无法获取到信号量
        OS_EXIT_CRITICAL();
        return OS_ERR_SEM_UNAVAILABLE;
    }
    return OS_ERR_NONE;
}

// 查询信号量的个数
// OS_ERR_NONE调用成功
// OS_ERR_PDATA_NULL用户传入null指针
// OS_ERR_SEM_ID_INVALID无效的信号量id
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

// 设置信号量数量
// OS_ERR_NONE调用成功
// OS_ERR_SEM_ID_INVALID无效的信号量id
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






