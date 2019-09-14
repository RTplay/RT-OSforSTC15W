#include "rt_os.h"
#include "rt_os_private.h"

OS_SS sys_stat[TASK_SIZE] = {0};


#ifdef SYSTEM_DETECT_MODE
/*************************************��ջͳ����ش���*******************************************/
u8 get_stack_used(u8 *pstack, u8 stack_size)
{
    u8 i = stack_size-1;
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


















#endif