#include "rt_os.h"
#include "rt_os_private.h"

#ifdef FLAG_ENABLE

//�¼���Ϣ�ṹ��
typedef struct os_flag {
    u8          OSFlagType;              /* ����¼���Ϣ����ʼ����Ӧ������ΪOS_EVENT_TYPE_FLAG      */
    u8          OSFlagPendTbl;           /* �ȴ��¼��������б�                                      */
    OS_FLAGS    OSFlagFlags;             /* 8, 16 or 32 bit flags                                   */
    OS_FLAGS    OSFlagInterestedFlags;   /* ��Ҫ���й��ĵ��¼�λ                                    */
    u8          OSFlagWaitType;          /* ָ������Ϣ�¼��жϷ�ʽ��                                */
} OS_FLAG;

static OS_FLAG flag[OS_FLAG_SIZE] = {0};

static void os_flag_task_rdy(u8 flag_index);

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

// ����־λ�����û�����㲻���������
// �¼���־���еı�־ʹ����������������������ʱ�������Ϊ0��
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
        if (flags_rdy == flag[flag_index].OSFlagInterestedFlags) {  //����������λ
            if (consume == TRUE) {                 /* See if we need to consume the flags      */
                flag[flag_index].OSFlagFlags &= ~flag[flag_index].OSFlagInterestedFlags;     /* ��ձ�־λ  */
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
                flag[flag_index].OSFlagFlags &= ~flag[flag_index].OSFlagInterestedFlags;     /* ��ձ�־λ  */
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
                flag[flag_index].OSFlagFlags |= flag[flag_index].OSFlagInterestedFlags;     /* �����־λ  */
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
                flag[flag_index].OSFlagFlags |= flag[flag_index].OSFlagInterestedFlags;     /* �����־λ  */
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

// ���ô˺����Եȴ����¼���־�������õ�λ��ϡ�
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
        if (flags_rdy == flag[flag_index].OSFlagInterestedFlags) {  //����������λ
            if (consume == TRUE) {                     /* See if we need to consume the flags      */
                flag[flag_index].OSFlagFlags &= ~flag[flag_index].OSFlagInterestedFlags;     /* ��ձ�־λ  */
            }
            *err = OS_ERR_NONE;
            OS_EXIT_CRITICAL();
            return flags_rdy;
        } else {                                                           /* ���������㣬�������� */
            flag[flag_index].OSFlagPendTbl |= 0x01<<os_task_running_ID;    //��������ȴ���
            os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_FLAG;         //���Լ���״̬��Ϊ�¼���ʶ�ȴ�
            os_tcb[os_task_running_ID].OSTCBDly = ticks;                   //����ʱΪ 0�������޵ȴ�
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
                flag[flag_index].OSFlagFlags &= ~flag[flag_index].OSFlagInterestedFlags;     /* ��ձ�־λ  */
            }
            *err = OS_ERR_NONE;
            OS_EXIT_CRITICAL();
            return flags_rdy;
        } else {
            flag[flag_index].OSFlagPendTbl |= 0x01<<os_task_running_ID;    //��������ȴ���
            os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_FLAG;         //���Լ���״̬��Ϊ�¼���ʶ�ȴ�
            os_tcb[os_task_running_ID].OSTCBDly = ticks;                   //����ʱΪ 0�������޵ȴ�
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
                flag[flag_index].OSFlagFlags |= flag[flag_index].OSFlagInterestedFlags;     /* �����־λ  */
            }
            *err = OS_ERR_NONE;
            OS_EXIT_CRITICAL();
            return flags_rdy;
        } else {
            flag[flag_index].OSFlagPendTbl |= 0x01<<os_task_running_ID;    //��������ȴ���
            os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_FLAG;         //���Լ���״̬��Ϊ�¼���ʶ�ȴ�
            os_tcb[os_task_running_ID].OSTCBDly = ticks;                   //����ʱΪ 0�������޵ȴ�
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
                flag[flag_index].OSFlagFlags |= flag[flag_index].OSFlagInterestedFlags;     /* �����־λ  */
            }
            *err = OS_ERR_NONE;
            OS_EXIT_CRITICAL();
            return flags_rdy;
        } else {
            flag[flag_index].OSFlagPendTbl |= 0x01<<os_task_running_ID;    //��������ȴ���
            os_tcb[os_task_running_ID].OSTCBStatus = OS_STAT_FLAG;         //���Լ���״̬��Ϊ�¼���ʶ�ȴ�
            os_tcb[os_task_running_ID].OSTCBDly = ticks;                   //����ʱΪ 0�������޵ȴ�
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

    OS_TASK_SW(); //���µ���
    // ���ٴν���ʱ
    OS_ENTER_CRITICAL();
    switch (os_tcb[os_task_running_ID].OSTCBStatPend) {
    case OS_STAT_PEND_ABORT:
        *err = OS_ERR_PEND_ABORT;                               /* flag��ɾ��������ֹ                      */
        flags_rdy = 0;
        break;
    case OS_STAT_PEND_OK:
        switch (wait_type) {
        case OS_FLAG_WAIT_SET_ALL:                              /* See if all required flags are set        */
            flags_rdy = flag[flag_index].OSFlagInterestedFlags;
            if (consume == TRUE) {                              /* See if we need to consume the flags      */
                flag[flag_index].OSFlagFlags &= ~flag[flag_index].OSFlagInterestedFlags;     /* ��ձ�־λ  */
            }
            break;

        case OS_FLAG_WAIT_SET_ANY:
            flags_rdy = flag[flag_index].OSFlagInterestedFlags & flag[flag_index].OSFlagFlags;
            if (consume == TRUE) {                              /* See if we need to consume the flags      */
                flag[flag_index].OSFlagFlags &= ~flag[flag_index].OSFlagInterestedFlags;     /* ��ձ�־λ  */
            }
            break;

        case OS_FLAG_WAIT_CLR_ALL:                              /* See if all required flags are cleared    */
            flags_rdy = flag[flag_index].OSFlagInterestedFlags;
            if (consume == TRUE) {                              /* See if we need to consume the flags      */
                flag[flag_index].OSFlagFlags |= flag[flag_index].OSFlagInterestedFlags;      /* �����־λ  */
            }
            break;

        case OS_FLAG_WAIT_CLR_ANY:
            flags_rdy = flag[flag_index].OSFlagInterestedFlags & ~flag[flag_index].OSFlagFlags;
            if (consume == TRUE) {                              /* See if we need to consume the flags      */
                flag[flag_index].OSFlagFlags |= flag[flag_index].OSFlagInterestedFlags;      /* �����־λ  */
            }
            break;
        }
        *err = OS_ERR_NONE;
        break;
    case OS_STAT_PEND_TO:
        if (os_tcb[os_task_running_ID].OSTCBDly == 0) {
            flag[flag_index].OSFlagPendTbl &= ~(0x01<<os_task_running_ID);  //���Լ�������ȴ��������
            *err = OS_ERR_TIMEOUT;                                 /* Indicate that we timed-out waiting       */
            flags_rdy = 0;
        } else {
            os_tcb[os_task_running_ID].OSTCBDly = 0;
            flag[flag_index].OSFlagPendTbl &= ~(0x01<<os_task_running_ID);  //���Լ�������ȴ��������
            switch (wait_type) {
            case OS_FLAG_WAIT_SET_ALL:                              /* See if all required flags are set        */
                flags_rdy = flag[flag_index].OSFlagInterestedFlags;
                if (consume == TRUE) {                              /* See if we need to consume the flags      */
                    flag[flag_index].OSFlagFlags &= ~flag[flag_index].OSFlagInterestedFlags;     /* ��ձ�־λ  */
                }
                break;

            case OS_FLAG_WAIT_SET_ANY:
                flags_rdy = flag[flag_index].OSFlagInterestedFlags & flag[flag_index].OSFlagFlags;
                if (consume == TRUE) {                              /* See if we need to consume the flags      */
                    flag[flag_index].OSFlagFlags &= ~flag[flag_index].OSFlagInterestedFlags;     /* ��ձ�־λ  */
                }
                break;

            case OS_FLAG_WAIT_CLR_ALL:                              /* See if all required flags are cleared    */
                flags_rdy = flag[flag_index].OSFlagInterestedFlags;
                if (consume == TRUE) {                              /* See if we need to consume the flags      */
                    flag[flag_index].OSFlagFlags |= flag[flag_index].OSFlagInterestedFlags;      /* �����־λ  */
                }
                break;

            case OS_FLAG_WAIT_CLR_ANY:
                flags_rdy = flag[flag_index].OSFlagInterestedFlags & ~flag[flag_index].OSFlagFlags;
                if (consume == TRUE) {                              /* See if we need to consume the flags      */
                    flag[flag_index].OSFlagFlags |= flag[flag_index].OSFlagInterestedFlags;      /* �����־λ  */
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

// ���ô˺��������û�����¼���־���е�ĳЩλ�� Ҫ���û������λ�ɡ�bit mask��ָ����
// flags���'opt'�������ģ���OS_FLAG_SET������'flags'�����õ�ÿ��λ�������¼���־���е���Ӧλ��
// ���� Ҫ����λ0,4��5�������Խ�'flags'����Ϊ��
// 0x31��ע�⣬λ0�������Чλ��
// ���'opt'�������ģ���OS_FLAG_CLR������'flags'�����õ�ÿ��λ������¼���־���е���Ӧλ�� ���� Ҫ���λ0,4��5�������Խ�'flags'ָ��Ϊ��
// 0x31��ע�⣬λ0�������Чλ��
// opt��ʾ��־�Ƿ�Ϊ�����ã�OS_FLAG_SET���� �����OS_FLAG_CLR��
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
        switch (flag[flag_index].OSFlagWaitType & ~OS_FLAG_CONSUME) { //ȥ�����ı�־�����Ĵ�����pend��
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

// ��������ȴ��������ȼ���ߵ���������λ����̬
static void os_flag_task_rdy(u8 flag_index)
{
    char i = 0, j = 0;
    u8 highest_prio_id = 0;
    u8 task_sequence = 0;//��ǰ����������ж�����һ����0
    // ���ҵȴ����ź������������ȼ���߲�������ִ��ʱ��Ͽ������������Ϊ����̬
    for (; i<TASK_SIZE; i++) { //�ҵ����ȼ�������񣬲�������������������е����������������������û������������
        if ((flag[flag_index].OSFlagPendTbl >> i) & 0x01) {
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
}










#endif