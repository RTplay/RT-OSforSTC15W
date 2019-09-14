#include "rt_os.h"
#include "rt_os_private.h"

//互斥锁结构体
typedef struct os_mutex {
    u8              OSMutexState;           /* 互斥锁状态，0：被占用，1：可用                          */
    u8              OSMutexPendTbl;         /* 等待互斥锁任务列表                                      */
    u8              OSMutexOwnerTaskID;     /* 互斥锁拥有者任务ID                                      */
    u8              OSMutexOwnerPrio;       /* 互斥锁拥有者的优先级，用于优先级恢复                    */
    u8              OSMutexCnt;             /* 互斥锁嵌套层数，允许拥有者嵌套使用                      */
} OS_MUTEX;

#ifdef MUTEX_ENABLE
static OS_MUTEX mutex[OS_MUTEX_SIZE];

// 初始化互斥锁
// OS_ERR_MUTXE_ID_INVALID无效的锁id
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

// 请求使用一个互斥锁
// ticks为超时设置，如果为0则永久等待
// OS_ERR_NONE调用成功，您的任务拥有互斥锁
// OS_ERR_TIMEOUT指定的“超时时间”内的互斥锁不可用。
// OS_ERR_MUTXE_ID_INVALID无效的锁id
u8 os_mutex_pend(u8 mutex_index, u16 ticks)
{
    if (mutex_index >= OS_MUTEX_SIZE)
        return OS_ERR_MUTXE_ID_INVALID;

    OS_ENTER_CRITICAL();
    if (mutex[mutex_index].OSMutexOwnerTaskID == os_task_running_ID) { //自身进行嵌套锁
        mutex[mutex_index].OSMutexCnt++;
        OS_EXIT_CRITICAL();
    } else if (mutex[mutex_index].OSMutexState == 1) { //互斥锁有效
        mutex[mutex_index].OSMutexState = 0;      //互斥锁被占，不可用
        mutex[mutex_index].OSMutexOwnerTaskID = os_task_running_ID;
        mutex[mutex_index].OSMutexOwnerPrio = os_tcb[os_task_running_ID].OSTCBPrio;
        mutex[mutex_index].OSMutexCnt++;
        OS_EXIT_CRITICAL();
    } else { // 无法获取到锁
        mutex[mutex_index].OSMutexPendTbl |= 0x01<<os_task_running_ID;  //加入互斥锁的任务等待表
        os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_MUTEX;         //将自己的状态改为互斥锁等待
        os_tcb[os_task_running_ID].OSTCBDly = ticks; //如延时为 0，刚无限等待
        if (os_tcb[os_task_running_ID].OSTCBPrio > mutex[mutex_index].OSMutexOwnerPrio) { //如果拥有者的优先级低
            os_tcb[(mutex[mutex_index].OSMutexOwnerTaskID)].OSTCBPrio = os_tcb[os_task_running_ID].OSTCBPrio; //重新提升拥有者优先级，并进行任务调度
        }
        OS_EXIT_CRITICAL();
        OS_TASK_SW(); //从新调度

        // 当再次进入时，根据OSTCBDly值判断是否是超时导致的。
        if(os_tcb[os_task_running_ID].OSTCBDly == 0) {
            mutex[mutex_index].OSMutexPendTbl &= ~(0x01<<os_task_running_ID);  //将自己从任务等待表中清除
            return OS_ERR_TIMEOUT;
        } else { // 如果没有超时，需要将OSTCBDly置0，以免引发不必要的调度
            OS_ENTER_CRITICAL();
            os_tcb[os_task_running_ID].OSTCBDly = 0;
            mutex[mutex_index].OSMutexState = 0;      //互斥锁被占，不可用
            mutex[mutex_index].OSMutexOwnerTaskID = os_task_running_ID;
            mutex[mutex_index].OSMutexOwnerPrio = os_tcb[os_task_running_ID].OSTCBPrio;
            mutex[mutex_index].OSMutexPendTbl &= ~(0x01<<os_task_running_ID);  //将自己从任务等待表中清除
            mutex[mutex_index].OSMutexCnt++;
            OS_EXIT_CRITICAL();
        }
    }

    return OS_ERR_NONE;
}

// 释放互斥锁
// OS_ERR_NONE调用成功并发出互斥信号。
// OS_ERR_NOT_MUTEX_OWNER释放互斥锁的任务不是MUTEX的所有者。
// OS_ERR_NOT_MUTEX未上锁
// OS_ERR_MUTXE_ID_INVALID无效的锁id
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
    if (mutex[mutex_index].OSMutexCnt == 0) { // 嵌套锁到底
        if (mutex[mutex_index].OSMutexPendTbl == 0) { //无任务等待互斥锁
            mutex[mutex_index].OSMutexState = 1;      //互斥锁可用
            mutex[mutex_index].OSMutexOwnerTaskID = 0xFF;
            mutex[mutex_index].OSMutexOwnerPrio = 0;
        } 
        else { // 有任务等待互斥锁
            char i = 0, j = 0;
            u8 highest_prio_id = 0;
            u8 task_sequence = 0;//当前任务的已运行队列中一定是0
            mutex[mutex_index].OSMutexState = 1;      //互斥锁可用
            mutex[mutex_index].OSMutexOwnerTaskID = 0xFF;
            // 查找等待该互斥锁中任务优先级最高并且任务被执行时间较靠后的任务，设置为就绪态
            for (; i<TASK_SIZE; i++) { //找到优先级最高任务，并且是在已运行任务队列的最后，如果已运行任务队列中没有则优先运行
                if ((mutex[mutex_index].OSMutexPendTbl >> i) & 0x01) {
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
            os_tcb[os_task_running_ID].OSTCBPrio = mutex[mutex_index].OSMutexOwnerPrio; // 修改自己的优先级，并且进行调度
            os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_RDY;
            OS_EXIT_CRITICAL();
            OS_TASK_SW();
            return OS_ERR_NONE;
        }
    }

    OS_EXIT_CRITICAL();
    return OS_ERR_NONE;
}

// 无等待请求互斥锁
// OS_ERR_NONE调用成功，您的任务拥有互斥锁
// OS_ERR_PEND_LOCKED锁已经被占用
// OS_ERR_MUTXE_ID_INVALID无效的锁id
u8 os_mutex_accept(u8 mutex_index)
{
    if (mutex_index >= OS_MUTEX_SIZE)
        return OS_ERR_MUTXE_ID_INVALID;
    OS_ENTER_CRITICAL();
    if (mutex[mutex_index].OSMutexOwnerTaskID == os_task_running_ID) { //自身进行嵌套锁
        mutex[mutex_index].OSMutexCnt++;
        OS_EXIT_CRITICAL();
    } else if (mutex[mutex_index].OSMutexState == 1) { //互斥锁有效
        mutex[mutex_index].OSMutexState = 0;      //互斥锁被占，不可用
        mutex[mutex_index].OSMutexOwnerTaskID = os_task_running_ID;
        mutex[mutex_index].OSMutexOwnerPrio = os_tcb[os_task_running_ID].OSTCBPrio;
        mutex[mutex_index].OSMutexCnt++;
        OS_EXIT_CRITICAL();
    } else { // 无法获取到锁
        OS_EXIT_CRITICAL();
        return OS_ERR_PEND_LOCKED;
    }

    return OS_ERR_NONE;
}

// 查询锁的状态
// OS_ERR_PEND_UNLOCK锁未被占用
// OS_ERR_PEND_LOCKED锁已经被占用
// OS_ERR_MUTXE_ID_INVALID无效的锁id
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