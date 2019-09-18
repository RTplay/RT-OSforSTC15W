#include "rt_os.h"
#include "rt_os_private.h"

#ifdef FLAG_ENABLE

//事件消息结构体
typedef struct os_flag {
    u8          OSFlagType;              /* 如果事件消息被初始化过应该设置为OS_EVENT_TYPE_FLAG      */
    u8          OSFlagPendTbl;           /* 等待事件的任务列表                                      */
    OS_FLAGS    OSFlagFlags;             /* 8, 16 or 32 bit flags                                   */
    OS_FLAGS    OSFlagInterestedFlags;   /* 需要进行关心的事件位                                    */
    interested

    u8          OSFlagWaitType;          /* 指定是消息事件判断方式。                                */
} OS_FLAG;

static OS_FLAG flag[OS_FLAG_SIZE] = {0};

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

//
//
//
OS_FLAGS OSFlagAccept (u8 flag_index, u8 *err)
{
    BOOLEAN consume, match;
    u8 result, wait_type;
    OS_FLAGS flags_rdy;
    if (flag_index >= OS_FLAG_SIZE) {
        *err = OS_ERR_FLAG_ID_INVALID;
        return 0;
    }
    if (flag[flag_index].OSFlagType != OS_EVENT_TYPE_UNUSED) {
        *err = OS_ERR_FLAG_USED;
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

    result = (u8)(wait_type & OS_FLAG_MATCH);
    if (result != 0) {                              /* See if we must be match exactly the flags      */
        wait_type &= ~OS_FLAG_MATCH;
        match    = TRUE;
    } else {
        match    = FALSE;
    }

    OS_ENTER_CRITICAL();
    switch (wait_type) {
    case OS_FLAG_WAIT_SET_ALL:                         /* See if all required flags are set        */
        if (match == TRUE) {
            if (flag[flag_index].OSFlagInterestedFlags == flag[flag_index].OSFlagFlags) { //全部匹配必须每一位都相同
                if (consume == TRUE) {                 /* See if we need to consume the flags      */
                    flag[flag_index].OSFlagFlags = 0;     /* 清空标志位  */
                }
                *err = OS_ERR_NONE;
                flags_rdy = flag[flag_index].OSFlagInterestedFlags;
            } else {
                *err = OS_ERR_FLAG_NOT_RDY;
                flags_rdy = 0;
            }
        } else {
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
        }
        OS_EXIT_CRITICAL();
        break;

    case OS_FLAG_WAIT_SET_ANY:
        if (match == TRUE) {                                                  /* 未设置位必须为0 */
            //判断0位必须为0
            if ((~flag[flag_index].OSFlagInterestedFlags & flag[flag_index].OSFlagFlags) == flag[flag_index].OSFlagInterestedFlags) {
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
            } else {
                *err = OS_ERR_FLAG_NOT_RDY;
                flags_rdy = 0;
            }
        } else {
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
        }
        OS_EXIT_CRITICAL();
        break;

    case OS_FLAG_WAIT_CLR_ALL:                        /* See if all required flags are cleared    */
        flags_rdy = (OS_FLAGS)~pgrp->OSFlagFlags & flags;    /* Extract only the bits we want     */
        if (flags_rdy == flags) {                     /* Must match ALL the bits that we want     */
            if (consume == OS_TRUE) {                 /* See if we need to consume the flags      */
                pgrp->OSFlagFlags |= flags_rdy;       /* Set ONLY the flags that we wanted        */
            }
        } else {
            *perr = OS_ERR_FLAG_NOT_RDY;
        }
        OS_EXIT_CRITICAL();
        break;

    case OS_FLAG_WAIT_CLR_ANY:
        flags_rdy = (OS_FLAGS)~pgrp->OSFlagFlags & flags;   /* Extract only the bits we want      */
        if (flags_rdy != (OS_FLAGS)0) {               /* See if any flag cleared                  */
            if (consume == OS_TRUE) {                 /* See if we need to consume the flags      */
                pgrp->OSFlagFlags |= flags_rdy;       /* Set ONLY the flags that we got           */
            }
        } else {
            *perr = OS_ERR_FLAG_NOT_RDY;
        }
        OS_EXIT_CRITICAL();
        break;
#endif

    default:
        OS_EXIT_CRITICAL();
        flags_rdy = 0;
        *err   = OS_ERR_FLAG_WAIT_TYPE;
        break;
    }
    return (acc_flag);
}




























#endif