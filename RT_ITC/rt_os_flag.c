#include "rt_os.h"
#include "rt_os_private.h"

#ifdef FLAG_ENABLE

//�¼���Ϣ�ṹ��
typedef struct os_flag {
    u8          OSFlagType;              /* ����¼���Ϣ����ʼ����Ӧ������ΪOS_EVENT_TYPE_FLAG      */
    u8          OSFlagPendTbl;           /* �ȴ��¼��������б�                                      */
    OS_FLAGS    OSFlagFlags;             /* 8, 16 or 32 bit flags                                   */
    OS_FLAGS    OSFlagInterestedFlags;   /* ��Ҫ���й��ĵ��¼�λ                                    */
    interested

    u8          OSFlagWaitType;          /* ָ������Ϣ�¼��жϷ�ʽ��                                */
} OS_FLAG;

static OS_FLAG flag[OS_FLAG_SIZE] = {0};

// ��ʼ��һ���¼���־
// flags��ָʾ��ϣ���ȴ��ĸ�λ������־����λģʽ��
// init_flags�ǳ�ʼֵ��
// ͨ������'flags'�е���Ӧλ��ָ�������λ�� ���� ������Ӧ�ó�����Ҫ�ȴ�0��1λ����ô'flags'������0x03��
// wait_typeָ����Ҫ��������λ����Ҫ�����κ�λ��
// ������ָ�����²�����
// OS_FLAG_WAIT_CLR_ALL�����ȴ�'mask'�е�����λ�����0��
// OS_FLAG_WAIT_SET_ALL�����ȴ�'mask'�е�����λ�����ã�1��
// OS_FLAG_WAIT_CLR_ANY�����ȴ�'mask'�е��κ�λ�����0��
// OS_FLAG_WAIT_SET_ANY�����ȴ�'mask'�е��κ�λ���ã�1��
// ע�⣺�����ϣ���¼���Ǳ������ġ��������OS_FLAG_CONSUME
// ���磬�ȴ����е��κα�־Ȼ�����
// ���ڵı�־����'wait_type'����Ϊ��OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME
// һ�������ֻ�������õı�־λ�������Ҫ��ȫƥ����Ҫ���OS_FLAG_MATCH
u8 os_flag_init (u8 flag_index, OS_FLAGS  init_flags, OS_FLAGS  flags, u8 wait_type)
{
    if (flag_index >= OS_FLAG_SIZE)
        return OS_ERR_FLAG_ID_INVALID;
    if (flag[flag_index].OSFlagType != OS_EVENT_TYPE_UNUSED)
        return OS_ERR_FLAG_USED;

    OS_ENTER_CRITICAL();
    flag[flag_index].OSFlagType     = OS_EVENT_TYPE_USED;   /* ����Ϊ�Ѿ���ʹ��                    */
    flag[flag_index].OSFlagFlags    = init_flags;           /* ���ó�ʼ��־λ                      */
    flag[flag_index].OSFlagInterestedFlags = flags;
    flag[flag_index].OSFlagWaitType = wait_type;            /* ���õȴ��������Ϊ��                */
    flag[flag_index].OSFlagPendTbl  = 0;                    /* ���õȴ��������Ϊ��                */
    OS_EXIT_CRITICAL();

    return (OS_ERR_NONE);
}

// ���һ���¼���־
// Optȷ��ɾ��ѡ�����£�
// opt == OS_DEL_NO_PEND����û�д����������ʱɾ���¼���־��
// opt == OS_DEL_ALWAYS��ʹ�������ڵȴ���Ҳɾ���¼���־�顣 ����������£����������д�������������񷵻�abort��
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
            for (; i<TASK_SIZE; i++) { //�����������������λabort
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
            if (flag[flag_index].OSFlagInterestedFlags == flag[flag_index].OSFlagFlags) { //ȫ��ƥ�����ÿһλ����ͬ
                if (consume == TRUE) {                 /* See if we need to consume the flags      */
                    flag[flag_index].OSFlagFlags = 0;     /* ��ձ�־λ  */
                }
                *err = OS_ERR_NONE;
                flags_rdy = flag[flag_index].OSFlagInterestedFlags;
            } else {
                *err = OS_ERR_FLAG_NOT_RDY;
                flags_rdy = 0;
            }
        } else {
            flags_rdy = flag[flag_index].OSFlagInterestedFlags & flag[flag_index].OSFlagFlags;
            if (flags_rdy == flag[flag_index].OSFlagInterestedFlags) {  //����������λ
                if (consume == TRUE) {                 /* See if we need to consume the flags      */
                    flag[flag_index].OSFlagFlags &= ~flag[flag_index].OSFlagInterestedFlags;     /* ��ձ�־λ  */
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
        if (match == TRUE) {                                                  /* δ����λ����Ϊ0 */
            //�ж�0λ����Ϊ0
            if ((~flag[flag_index].OSFlagInterestedFlags & flag[flag_index].OSFlagFlags) == flag[flag_index].OSFlagInterestedFlags) {
                flags_rdy = flag[flag_index].OSFlagInterestedFlags & flag[flag_index].OSFlagFlags;
                if (flags_rdy != 0) {
                    if (consume == TRUE) {                 /* See if we need to consume the flags      */
                        flag[flag_index].OSFlagFlags &= ~flag[flag_index].OSFlagInterestedFlags;     /* ��ձ�־λ  */
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
                    flag[flag_index].OSFlagFlags &= ~flag[flag_index].OSFlagInterestedFlags;     /* ��ձ�־λ  */
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