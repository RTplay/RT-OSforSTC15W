#include "rt_os.h"
#include "rt_os_private.h"

#ifdef SYSTEM_DETECT_MODE
OS_SS sys_stat[TASK_SIZE] = {0};

/*************************************堆栈统计相关代码*******************************************/
u8 get_stack_used(u8 *pstack, u8 stack_size)
{
    u8 i = stack_size - 1;
    u8 unused = 0;
    while (STACK_MAGIC == pstack[i]) {
        unused++;
        if (0 == i)
            break;
        else
            i--;
    }
    return stack_size - unused;
}

OS_SS *get_sys_statistics(void)
{
    return sys_stat;
}

#endif