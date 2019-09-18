#include "rt_os.h"
#include "rt_os_private.h"

#ifdef FLAG_ENABLE

//事件消息结构体
typedef struct os_flag {
    u8          OSFlagType;              /* 如果事件消息被初始化过应该设置为OS_EVENT_TYPE_FLAG      */
    u8          OSFlagPendTbl;           /* 等待事件的任务列表                                      */
    OS_FLAGS    OSFlagFlags;             /* 8, 16 or 32 bit flags                                   */
    OS_FLAGS    OSFlagInterestedFlags;   /* 需要进行关心的事件位                                    */
    u8          OSFlagWaitType;          /* 指定是消息事件判断方式。                                */
} OS_FLAG;

static OS_FLAG flag[OS_FLAG_SIZE] = {0};

static void os_flag_task_rdy(u8 flag_index);

// 初始化一个事件标志
// flags是指示您希望等待哪个位（即标志）的位模式。
// init_flags是初始值。
// 通过设置'flags'中的相应位来指定所需的位。 例如 如果你的应用程序想要等待0和1位，那么'flags'将包含0x03。
// wait_type指定是要设置所有位还是要设置任何位。
// 您可以指定以下参数：
// OS_FLAG_WAIT_CLR_ALL您将等待'mask'中的所有位清除（0）
// OS_FLAG_WAIT_SET_ALL您将等待'mask'中的所有位被设置（1）
// OS_FLAG_WAIT_CLR_ANY您将等待'mask'中的任何位清除（0）
// OS_FLAG_WAIT_SET_ANY您将等待'mask'中的任何位设置（1）
// 注意：如果您希望事件标记被“消耗”，请添加OS_FLAG_CONSUME
// 例如，等待组中的任何标志然后清除
// 存在的标志，将'wait_type'设置为：OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME
// 一般情况下只关心设置的标志位，如果需要完全匹配需要添加OS_FLAG_MATCH
u8 os_flag_init (u8 flag_index, OS_FLAGS  init_flags, OS_FLAGS  flags, u8 wait_type)
{
    if (flag_index >= OS_FLAG_SIZE)
        return OS_ERR_FLAG_ID_INVALID;
    if (flag[flag_index].OSFlagType != OS_EVENT_TYPE_UNUSED)
        return OS_ERR_FLAG_USED;

    OS_ENTER_CRITICAL();
    flag[flag_index].OSFlagType     = OS_EVENT_TYPE_USED;   /* 设置为已经被使用                    */
    flag[flag_index].OSFlagFlags    = init_flags;           /* 设置初始标志位                      */
    flag[flag_index].OSFlagInterestedFlags = flags;
    flag[flag_index].OSFlagWaitType = wait_type;            /* 设置等待任务队列为空                */
    flag[flag_index].OSFlagPendTbl  = 0;                    /* 设置等待任务队列为空                */
    OS_EXIT_CRITICAL();

    return (OS_ERR_NONE);
}

// 清除一个事件标志
// Opt确定删除选项如下：
// opt == OS_DEL_NO_PEND仅在没有待处理的任务时删除事件标志组
// opt == OS_DEL_ALWAYS即使任务正在等待，也删除事件标志组。 在这种情况下，将就绪所有待处理的任务，任务返回abort。
u8 os_flag_release (u8 flag_index, u8 opt)
{
    BOOLEAN tasks_waiting;
    u8 rel_return;
    if (flag_index >= OS_FLAG_SIZE)
        return OS_ERR_FLAG_ID_INVALID;
    if (flag[flag_index].OSFlagType != OS_EVENT_TYPE_USED)
        return OS_ERR_FLAG_UNUSED;

    OS_ENTER_CRITICAL();
    if (flag[flag_index].OSFlagPendTbl != 0) {             /* See if any tasks waiting on event flags  */
        tasks_waiting = TRUE;                              /* Yes                                      */
    } else {
        tasks_waiting = FALSE;                             /* No                                       */
    }
    switch (opt) {
    case OS_DEL_NO_PEND:                               /* Delete group if no task waiting          */
        if (tasks_waiting == FALSE) {
            flag[flag_index].OSFlagType     = OS_EVENT_TYPE_UNUSED;
            flag[flag_index].OSFlagFlags    = (OS_FLAGS)0;
            flag[flag_index].OSFlagInterestedFlags = 0;
            OS_EXIT_CRITICAL();
            rel_return = OS_ERR_NONE;
        } else {
            OS_EXIT_CRITICAL();
            rel_return = OS_ERR_TASK_WAITING;
        }
        break;

    case OS_DEL_ALWAYS:                                /* Always delete the event flag group       */
        if (tasks_waiting == FALSE) {
            flag[flag_index].OSFlagType     = OS_EVENT_TYPE_UNUSED;
            flag[flag_index].OSFlagFlags    = (OS_FLAGS)0;
            flag[flag_index].OSFlagInterestedFlags = 0;
            OS_EXIT_CRITICAL();

        } else {
            u8 i = 0;
            for (; i<TASK_SIZE; i++) { //将所有任务就绪，置位abort
                if ((flag[flag_index].OSFlagPendTbl >> i) & 0x01) {
                    os_tcb[i].OSTCBStatPend = OS_STAT_PEND_ABORT;
                    os_tcb[i].OSTCBStatus = OS_STAT_RDY;
                }
            }
            flag[flag_index].OSFlagType     = OS_EVENT_TYPE_UNUSED;
            flag[flag_index].OSFlagFlags    = (OS_FLAGS)0;
            flag[flag_index].OSFlagInterestedFlags = 0;
            OS_EXIT_CRITICAL();
            OS_TASK_SW();
        }
        rel_return = OS_ERR_NONE;
        break;

    default:
        OS_EXIT_CRITICAL();
        rel_return = OS_ERR_INVALID_OPT;
        break;
    }
    return (rel_return);
}

// 检查标志位，如果没有满足不会挂起任务
// 事件标志组中的标志使任务就绪，或者如果发生超时或错误则为0。
OS_FLAGS os_flag_accept (u8 flag_index, u8 *err)
{
    BOOLEAN consume;
    u8 result, wait_type;
    OS_FLAGS flags_rdy;
    if (flag_index >= OS_FLAG_SIZE) {
        *err = OS_ERR_FLAG_ID_INVALID;
        return 0;
    }
    if (flag[flag_index].OSFlagType != OS_EVENT_TYPE_USED) {
        *err = OS_ERR_FLAG_UNUSED;
        return 0;
    }

    wait_type = flag[flag_index].OSFlagWaitType;
    result = (u8)(wait_type & OS_FLAG_CONSUME);
    if (result != 0) {                                   /* See if we need to consume the flags      */
        wait_type &= ~OS_FLAG_CONSUME;
        consume    = TRUE;
    } else {
        consume    = FALSE;
    }

    OS_ENTER_CRITICAL();
    switch (wait_type) {
    case OS_FLAG_WAIT_SET_ALL:                         /* See if all required flags are set        */
        flags_rdy = flag[flag_index].OSFlagInterestedFlags & flag[flag_index].OSFlagFlags;
        if (flags_rdy == flag[flag_index].OSFlagInterestedFlags) {  //不关心其他位
            if (consume == TRUE) {                 /* See if we need to consume the flags      */
                flag[flag_index].OSFlagFlags &= ~flag[flag_index].OSFlagInterestedFlags;     /* 清空标志位  */
            }
            *err = OS_ERR_NONE;
        } else {
            *err = OS_ERR_FLAG_NOT_RDY;
            flags_rdy = 0;
        }
        OS_EXIT_CRITICAL();
        break;

    case OS_FLAG_WAIT_SET_ANY:
        flags_rdy = flag[flag_index].OSFlagInterestedFlags & flag[flag_index].OSFlagFlags;
        if (flags_rdy != 0) {
            if (consume == TRUE) {                 /* See if we need to consume the flags      */
                flag[flag_index].OSFlagFlags &= ~flag[flag_index].OSFlagInterestedFlags;     /* 清空标志位  */
            }
            *err = OS_ERR_NONE;
        } else {
            *err = OS_ERR_FLAG_NOT_RDY;
            flags_rdy = 0;
        }
        OS_EXIT_CRITICAL();
        break;

    case OS_FLAG_WAIT_CLR_ALL:                        /* See if all required flags are cleared    */
        if (0 == flag[flag_index].OSFlagInterestedFlags & flag[flag_index].OSFlagFlags) {
            if (consume == TRUE) {                 /* See if we need to consume the flags      */
                flag[flag_index].OSFlagFlags |= flag[flag_index].OSFlagInterestedFlags;     /* 置起标志位  */
            }
            *err = OS_ERR_NONE;
            flags_rdy = flag[flag_index].OSFlagInterestedFlags;
        } else {
            *err = OS_ERR_FLAG_NOT_RDY;
            flags_rdy = 0;
        }
        OS_EXIT_CRITICAL();
        break;

    case OS_FLAG_WAIT_CLR_ANY:
        flags_rdy = flag[flag_index].OSFlagInterestedFlags & ~flag[flag_index].OSFlagFlags;
        if (flags_rdy != (OS_FLAGS)0) {               /* See if any flag cleared                  */
            if (consume == TRUE) {                    /* See if we need to consume the flags      */
                flag[flag_index].OSFlagFlags |= flag[flag_index].OSFlagInterestedFlags;     /* 置起标志位  */
            }
            *err = OS_ERR_NONE;
        } else {
            *err = OS_ERR_FLAG_NOT_RDY;
            flags_rdy = 0;
        }
        OS_EXIT_CRITICAL();
        break;

    default:
        OS_EXIT_CRITICAL();
        flags_rdy = 0;
        *err   = OS_ERR_FLAG_WAIT_TYPE;
        break;
    }
    return (flags_rdy);
}

// 调用此函数以等待在事件标志组中设置的位组合。
OS_FLAGS  os_flag_pend (u8 flag_index, u16 ticks, u8 *err)
{
    BOOLEAN consume;
    u8 result, wait_type;
    OS_FLAGS flags_rdy;
    if (flag_index >= OS_FLAG_SIZE) {
        *err = OS_ERR_FLAG_ID_INVALID;
        return 0;
    }
    if (flag[flag_index].OSFlagType != OS_EVENT_TYPE_USED) {
        *err = OS_ERR_FLAG_UNUSED;
        return 0;
    }

    wait_type = flag[flag_index].OSFlagWaitType;
    result = (u8)(wait_type & OS_FLAG_CONSUME);
    if (result != 0) {                                   /* See if we need to consume the flags      */
        wait_type &= ~OS_FLAG_CONSUME;
        consume    = TRUE;
    } else {
        consume    = FALSE;
    }

    OS_ENTER_CRITICAL();
    switch (wait_type) {
    case OS_FLAG_WAIT_SET_ALL:                         /* See if all required flags are set        */
        flags_rdy = (OS_FLAGS)(flag[flag_index].OSFlagFlags & flag[flag_index].OSFlagInterestedFlags)
        if (flags_rdy == flag[flag_index].OSFlagInterestedFlags) {  //不关心其他位
            if (consume == TRUE) {                     /* See if we need to consume the flags      */
                flag[flag_index].OSFlagFlags &= ~flag[flag_index].OSFlagInterestedFlags;     /* 清空标志位  */
            }
            *err = OS_ERR_NONE;
            OS_EXIT_CRITICAL();
            return flags_rdy;
        } else {                                                           /* 条件不满足，挂起任务 */
            flag[flag_index].OSFlagPendTbl |= 0x01<<os_task_running_ID;    //加入任务等待表
            os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_FLAG;         //将自己的状态改为事件标识等待
            os_tcb[os_task_running_ID].OSTCBDly = ticks;                   //如延时为 0，则无限等待
            if (ticks) {
                os_tcb[os_task_running_ID].OSTCBStatPend = OS_STAT_PEND_TO;
            } else {
                os_tcb[os_task_running_ID].OSTCBStatPend = OS_STAT_PEND_OK;
            }
            OS_EXIT_CRITICAL();
        }
        break;

    case OS_FLAG_WAIT_SET_ANY:
        flags_rdy = (OS_FLAGS)(flag[flag_index].OSFlagFlags & flag[flag_index].OSFlagInterestedFlags);
        if (flags_rdy != (OS_FLAGS)0) {
            if (consume == TRUE) {                 /* See if we need to consume the flags      */
                flag[flag_index].OSFlagFlags &= ~flag[flag_index].OSFlagInterestedFlags;     /* 清空标志位  */
            }
            *err = OS_ERR_NONE;
            OS_EXIT_CRITICAL();
            return flags_rdy;
        } else {
            flag[flag_index].OSFlagPendTbl |= 0x01<<os_task_running_ID;    //加入任务等待表
            os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_FLAG;         //将自己的状态改为事件标识等待
            os_tcb[os_task_running_ID].OSTCBDly = ticks;                   //如延时为 0，则无限等待
            if (ticks) {
                os_tcb[os_task_running_ID].OSTCBStatPend = OS_STAT_PEND_TO;
            } else {
                os_tcb[os_task_running_ID].OSTCBStatPend = OS_STAT_PEND_OK;
            }
            OS_EXIT_CRITICAL();
        }

        break;

    case OS_FLAG_WAIT_CLR_ALL:                        /* See if all required flags are cleared    */
        flags_rdy = (OS_FLAGS)(~flag[flag_index].OSFlagFlags & flag[flag_index].OSFlagInterestedFlags);
        if (flags_rdy == flag[flag_index].OSFlagInterestedFlags) {
            if (consume == TRUE) {                 /* See if we need to consume the flags      */
                flag[flag_index].OSFlagFlags |= flag[flag_index].OSFlagInterestedFlags;     /* 置起标志位  */
            }
            *err = OS_ERR_NONE;
            OS_EXIT_CRITICAL();
            return flags_rdy;
        } else {
            flag[flag_index].OSFlagPendTbl |= 0x01<<os_task_running_ID;    //加入任务等待表
            os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_FLAG;         //将自己的状态改为事件标识等待
            os_tcb[os_task_running_ID].OSTCBDly = ticks;                   //如延时为 0，则无限等待
            if (ticks) {
                os_tcb[os_task_running_ID].OSTCBStatPend = OS_STAT_PEND_TO;
            } else {
                os_tcb[os_task_running_ID].OSTCBStatPend = OS_STAT_PEND_OK;
            }
            OS_EXIT_CRITICAL();
        }
        break;

    case OS_FLAG_WAIT_CLR_ANY:
        flags_rdy = (OS_FLAGS)(OS_FLAGS)(~flag[flag_index].OSFlagFlags & flag[flag_index].OSFlagInterestedFlags);
        if (flags_rdy != (OS_FLAGS)0) {               /* See if any flag cleared                  */
            if (consume == TRUE) {                    /* See if we need to consume the flags      */
                flag[flag_index].OSFlagFlags |= flag[flag_index].OSFlagInterestedFlags;     /* 置起标志位  */
            }
            *err = OS_ERR_NONE;
            OS_EXIT_CRITICAL();
            return flags_rdy;
        } else {
            flag[flag_index].OSFlagPendTbl |= 0x01<<os_task_running_ID;    //加入任务等待表
            os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_FLAG;         //将自己的状态改为事件标识等待
            os_tcb[os_task_running_ID].OSTCBDly = ticks;                   //如延时为 0，则无限等待
            if (ticks) {
                os_tcb[os_task_running_ID].OSTCBStatPend = OS_STAT_PEND_TO;
            } else {
                os_tcb[os_task_running_ID].OSTCBStatPend = OS_STAT_PEND_OK;
            }
            OS_EXIT_CRITICAL();
        }
        break;

    default:
        OS_EXIT_CRITICAL();
        flags_rdy = 0;
        *err   = OS_ERR_FLAG_WAIT_TYPE;
        return flags_rdy;
        break;
    }

    OS_TASK_SW(); //从新调度
    // 当再次进入时
    OS_ENTER_CRITICAL();
    switch (os_tcb[os_task_running_ID].OSTCBStatPend) {
    case OS_STAT_PEND_ABORT:
        *err = OS_ERR_PEND_ABORT;                               /* flag被删除导致终止                      */
        flags_rdy = 0;
        break;
    case OS_STAT_PEND_OK:
        switch (wait_type) {
        case OS_FLAG_WAIT_SET_ALL:                              /* See if all required flags are set        */
            flags_rdy = flag[flag_index].OSFlagInterestedFlags;
            if (consume == TRUE) {                              /* See if we need to consume the flags      */
                flag[flag_index].OSFlagFlags &= ~flag[flag_index].OSFlagInterestedFlags;     /* 清空标志位  */
            }
            break;

        case OS_FLAG_WAIT_SET_ANY:
            flags_rdy = flag[flag_index].OSFlagInterestedFlags & flag[flag_index].OSFlagFlags;
            if (consume == TRUE) {                              /* See if we need to consume the flags      */
                flag[flag_index].OSFlagFlags &= ~flag[flag_index].OSFlagInterestedFlags;     /* 清空标志位  */
            }
            break;

        case OS_FLAG_WAIT_CLR_ALL:                              /* See if all required flags are cleared    */
            flags_rdy = flag[flag_index].OSFlagInterestedFlags;
            if (consume == TRUE) {                              /* See if we need to consume the flags      */
                flag[flag_index].OSFlagFlags |= flag[flag_index].OSFlagInterestedFlags;      /* 置起标志位  */
            }
            break;

        case OS_FLAG_WAIT_CLR_ANY:
            flags_rdy = flag[flag_index].OSFlagInterestedFlags & ~flag[flag_index].OSFlagFlags;
            if (consume == TRUE) {                              /* See if we need to consume the flags      */
                flag[flag_index].OSFlagFlags |= flag[flag_index].OSFlagInterestedFlags;      /* 置起标志位  */
            }
            break;
        }
        *err = OS_ERR_NONE;
        break;
    case OS_STAT_PEND_TO:
        if (os_tcb[os_task_running_ID].OSTCBDly == 0) {
            flag[flag_index].OSFlagPendTbl &= ~(0x01<<os_task_running_ID);  //将自己从任务等待表中清除
            *err = OS_ERR_TIMEOUT;                                 /* Indicate that we timed-out waiting       */
            flags_rdy = 0;
        } else {
            os_tcb[os_task_running_ID].OSTCBDly = 0;
            flag[flag_index].OSFlagPendTbl &= ~(0x01<<os_task_running_ID);  //将自己从任务等待表中清除
            switch (wait_type) {
            case OS_FLAG_WAIT_SET_ALL:                              /* See if all required flags are set        */
                flags_rdy = flag[flag_index].OSFlagInterestedFlags;
                if (consume == TRUE) {                              /* See if we need to consume the flags      */
                    flag[flag_index].OSFlagFlags &= ~flag[flag_index].OSFlagInterestedFlags;     /* 清空标志位  */
                }
                break;

            case OS_FLAG_WAIT_SET_ANY:
                flags_rdy = flag[flag_index].OSFlagInterestedFlags & flag[flag_index].OSFlagFlags;
                if (consume == TRUE) {                              /* See if we need to consume the flags      */
                    flag[flag_index].OSFlagFlags &= ~flag[flag_index].OSFlagInterestedFlags;     /* 清空标志位  */
                }
                break;

            case OS_FLAG_WAIT_CLR_ALL:                              /* See if all required flags are cleared    */
                flags_rdy = flag[flag_index].OSFlagInterestedFlags;
                if (consume == TRUE) {                              /* See if we need to consume the flags      */
                    flag[flag_index].OSFlagFlags |= flag[flag_index].OSFlagInterestedFlags;      /* 置起标志位  */
                }
                break;

            case OS_FLAG_WAIT_CLR_ANY:
                flags_rdy = flag[flag_index].OSFlagInterestedFlags & ~flag[flag_index].OSFlagFlags;
                if (consume == TRUE) {                              /* See if we need to consume the flags      */
                    flag[flag_index].OSFlagFlags |= flag[flag_index].OSFlagInterestedFlags;      /* 置起标志位  */
                }
                break;
            }
            *err = OS_ERR_NONE;
        }
        break;
    default:
        *err = OS_ERR_FLAG_CORE;                                    /* Indicate that we timed-out waiting       */
        flags_rdy = 0;
        break;
    }
    OS_EXIT_CRITICAL();
    return (flags_rdy);
}

// 调用此函数以设置或清除事件标志组中的某些位。 要设置或清除的位由“bit mask”指定。
// flags如果'opt'（见下文）是OS_FLAG_SET，则在'flags'中设置的每个位将设置事件标志组中的相应位。
// 例如 要设置位0,4和5，您可以将'flags'设置为：
// 0x31（注意，位0是最低有效位）
// 如果'opt'（见下文）是OS_FLAG_CLR，则在'flags'中设置的每个位将清除事件标志组中的相应位。 例如 要清除位0,4和5，您可以将'flags'指定为：
// 0x31（注意，位0是最低有效位）
// opt表示标志是否为：设置（OS_FLAG_SET）或 清除（OS_FLAG_CLR）
OS_FLAGS os_flag_post (u8 flag_index, u8 opt, u8 *err)
{
    BOOLEAN sched;

    OS_FLAGS flags_rdy;
    if (flag_index >= OS_FLAG_SIZE) {
        *err = OS_ERR_FLAG_ID_INVALID;
        return 0;
    }
    if (flag[flag_index].OSFlagType != OS_EVENT_TYPE_USED) {
        *err = OS_ERR_FLAG_UNUSED;
        return 0;
    }

    OS_ENTER_CRITICAL();
    switch (opt) {
    case OS_FLAG_CLR:
        flag[flag_index].OSFlagFlags &= (OS_FLAGS)~flags;  /* Clear the flags specified in the group         */
        break;

    case OS_FLAG_SET:
        flag[flag_index].OSFlagFlags |=  flags;            /* Set   the flags specified in the group         */
        break;

    default:
        OS_EXIT_CRITICAL();                     /* INVALID option                                 */
        *err = OS_ERR_FLAG_INVALID_OPT;
        return ((OS_FLAGS)0);
    }
    sched = FALSE;                                /* Indicate that we don't need rescheduling       */
    if (flag[flag_index].OSFlagPendTbl != 0) {             /* Has tasks waiting on event flag(s)  */
        switch (flag[flag_index].OSFlagWaitType & ~OS_FLAG_CONSUME) { //去掉消耗标志，消耗处理在pend中
        case OS_FLAG_WAIT_SET_ALL:               /* See if all req. flags are set for current node */
            flags_rdy = (OS_FLAGS)(flag[flag_index].OSFlagFlags & flag[flag_index].OSFlagInterestedFlags);
            if (flags_rdy == flag[flag_index].OSFlagInterestedFlags) {
                os_flag_task_rdy(flag_index);  /* Make task to RDY          */
                sched = TRUE;                     /* When done we will reschedule          */
            }
            break;

        case OS_FLAG_WAIT_SET_ANY:               /* See if any flag set                            */
            flags_rdy = (OS_FLAGS)(flag[flag_index].OSFlagFlags & flag[flag_index].OSFlagInterestedFlags);
            if (flags_rdy != (OS_FLAGS)0) {
                os_flag_task_rdy(flag_index);  /* Make task to RDY          */
                sched = TRUE;                     /* When done we will reschedule          */
            }
            break;

        case OS_FLAG_WAIT_CLR_ALL:               /* See if all req. flags are set for current node */
            flags_rdy = (OS_FLAGS)(~flag[flag_index].OSFlagFlags & flag[flag_index].OSFlagInterestedFlags);
            if (flags_rdy == flag[flag_index].OSFlagInterestedFlags) {
                os_flag_task_rdy(flag_index);  /* Make task to RDY          */
                sched = TRUE;                     /* When done we will reschedule          */
            }
            break;

        case OS_FLAG_WAIT_CLR_ANY:               /* See if any flag set                            */
            flags_rdy = (OS_FLAGS)(OS_FLAGS)(~flag[flag_index].OSFlagFlags & flag[flag_index].OSFlagInterestedFlags);
            if (flags_rdy != (OS_FLAGS)0) {
                os_flag_task_rdy(flag_index);  /* Make task to RDY          */
                sched = TRUE;                     /* When done we will reschedule          */
            }
            break;

        default:
            OS_EXIT_CRITICAL();
            *err = OS_ERR_FLAG_WAIT_TYPE;
            return ((OS_FLAGS)0);
        }
    }
    OS_EXIT_CRITICAL();
    if (sched == TRUE) {
        OS_TASK_SW();
    }
    *err     = OS_ERR_NONE;
    return (flags_rdy);
}

// 查找任务等待表中优先级最高的任务，设置位就绪态
static void os_flag_task_rdy(u8 flag_index)
{
    char i = 0, j = 0;
    u8 highest_prio_id = 0;
    u8 task_sequence = 0;//当前任务的已运行队列中一定是0
    // 查找等待该信号量中任务优先级最高并且任务被执行时间较靠后的任务，设置为就绪态
    for (; i<TASK_SIZE; i++) { //找到优先级最高任务，并且是在已运行任务队列的最后，如果已运行任务队列中没有则优先运行
        if ((flag[flag_index].OSFlagPendTbl >> i) & 0x01) {
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
}










#endif