#include "rt_os.h"
#include "rt_os_private.h"

#ifdef MEM_ENABLE

#define     OS_MEM_STRUCT_SIZE (2)
#define     MEM_USED (0x8000)

static xdata u8 mem[OS_MEM_SIZE] = {0};
static u16 mem_size = 0;

// 初始化内存头部
u8 os_mem_init (void)
{
    u16 *mem_head = (u16 *)mem;
    u16 no_warning = OS_MEM_SIZE;
    if (no_warning <= 2)
        return OS_ERR_MEM_INVALID_SIZE;
    if (no_warning % 2) //2字节对齐，如果未对齐则缩小一字节
        mem_size = no_warning - 1;
    else
        mem_size = no_warning;
    *mem_head = mem_size; //存入可用字节数

    return OS_ERR_NONE;
}

// malloc功能
void *os_malloc (u16 c_size)
{
    u8 *ret_ptr;
    u16 *mem_ptr = (u16 *)mem;
    u16 index = 0;//记录数组脚标
    if (c_size == 0)
        return NULL;
    if (c_size % 2) {
        c_size += 1;//两字节对齐
    }
    OS_ENTER_CRITICAL();
    while((*mem_ptr & ~MEM_USED) < OS_MEM_SIZE) { // 循环查看最后一组之前的可用数组
        //内存块被使用或者所有的内存不够需要的大小
        if ((*mem_ptr & MEM_USED) || ((*mem_ptr - index - OS_MEM_STRUCT_SIZE) < c_size)) {
            u16 mem_index = *mem_ptr & ~MEM_USED; //跳到下一个内存结构处
            index = mem_index;
            mem_ptr = (u16 *)&mem[mem_index];
        } else { //可以使用
            u16 next_addr = *mem_ptr;
            *mem_ptr = c_size + index + OS_MEM_STRUCT_SIZE;
            *mem_ptr |= MEM_USED;
            ret_ptr = (u8 *)mem_ptr + OS_MEM_STRUCT_SIZE;
            mem_ptr = (u16 *)&mem[c_size + index + OS_MEM_STRUCT_SIZE];
            *mem_ptr = next_addr;
            break;
        }
    }
    //处理最后一个内存块
    if ((*mem_ptr & ~MEM_USED) == OS_MEM_SIZE) {
        if (*mem_ptr & MEM_USED) {
            u16 mem_index = *mem_ptr & ~MEM_USED;
            index = mem_index;
            mem_ptr = (u16 *)&mem[*mem_ptr];
            ret_ptr = NULL;
        } else {
            if ((OS_MEM_SIZE - index - OS_MEM_STRUCT_SIZE) < c_size) { //剩余内存不够
                OS_EXIT_CRITICAL();
                return NULL;
            } else if ((OS_MEM_SIZE - index - OS_MEM_STRUCT_SIZE) == c_size) { //内存大小正好
                *mem_ptr |= MEM_USED;
                ret_ptr = (u8 *)mem_ptr + OS_MEM_STRUCT_SIZE;
            } else { //内存够大
                u16 next_addr = *mem_ptr;
                *mem_ptr = c_size + index + OS_MEM_STRUCT_SIZE;
                *mem_ptr |= MEM_USED;
                ret_ptr = (u8 *)mem_ptr + OS_MEM_STRUCT_SIZE;
                mem_ptr = (u16 *)&mem[c_size + index + OS_MEM_STRUCT_SIZE];
                *mem_ptr = next_addr;
            }
        }
    }
    OS_EXIT_CRITICAL();
    return (void *)ret_ptr;
}

//free功能
void os_free (void *free_ptr)
{
    u16 *mem_ptr;
    u8 *mem_head;
    u16 *last_free = NULL;
    if (free_ptr == NULL) {
        return;
    }
    OS_ENTER_CRITICAL();
    mem_head = (u8 *)free_ptr - OS_MEM_STRUCT_SIZE;
    mem_ptr = (u16 *)mem_head;
    *mem_ptr &= ~MEM_USED;
    //遍历内存，合拼空闲内存
    mem_ptr = (u16 *)mem;
    while((*mem_ptr & ~MEM_USED) < OS_MEM_SIZE) { // 循环查看最后一组之前的数组
        if ((*mem_ptr & MEM_USED) == 0) { //这一块是空闲的
            if (last_free) {
                *last_free = *mem_ptr;
                mem_ptr = (u16 *)&mem[*mem_ptr];
            } else {
                //跳到下一个内存结构处
                last_free = mem_ptr;
                mem_ptr = (u16 *)&mem[*mem_ptr];
            }
        } else { //在使用内存
            u16 next_index = (*mem_ptr & ~MEM_USED); //跳到下一个内存结构处
            last_free = NULL;
            mem_ptr = (u16 *)&mem[next_index];
        }
    }
    //处理最后一个内存块
    if ((*mem_ptr & ~MEM_USED) == OS_MEM_SIZE) {
        if ((*mem_ptr & MEM_USED) == 0) { //这一块是空闲的
            if (last_free) {
                *last_free = *mem_ptr;
            }
        } else { //在使用内存
            last_free = NULL;
        }
    }
    OS_EXIT_CRITICAL();
}

//calloc功能
void *os_calloc (u16 nmemb, u16 c_size)
{
    void *ret_ptr = os_malloc (nmemb*c_size);
    OS_ENTER_CRITICAL();
    if (ret_ptr) {
        os_memset(ret_ptr, 0, nmemb*c_size);
    }
    OS_EXIT_CRITICAL();
    return ret_ptr;
}

//realloc
void *os_realloc (void *ptr, u16 c_size)
{
    void *head_ptr, *data_ptr, *ret;
    u16 head;
    if (ptr == NULL) {
        return os_malloc (c_size);
    }
    if (c_size == 0) {
        os_free (ptr);
        return NULL;
    }
    OS_ENTER_CRITICAL();
    data_ptr = ptr;
    head_ptr = (void *)((u8 *)ptr - OS_MEM_STRUCT_SIZE);
    head = *(u16 *)head_ptr;
    os_free (ptr);
    ret = os_malloc(c_size);
    if (ret) {
        os_memcpy(ret, data_ptr, c_size);
    } else {
        *(u16 *)head_ptr = head;
    }
    OS_EXIT_CRITICAL();
    return ret;
}

//memset
void *os_memset(void *s, u8 c, u16 n)
{
    u8 i;
    if (s == NULL) {
        return s;
    }
    for (i=0; i<n; i++) {
        *((u8 *)s+i) = c;
    }

    return s;
}

//memcpy
void *os_memcpy(void *dest, const void *src, u16 n)
{
    u8 i;
    if ((dest == NULL) || (src == NULL)) {
        return dest;
    }
    for (i=0; i<n; i++) {
        *((u8 *)dest+i) = *((u8 *)src+i);
    }
    return dest;
}

#endif
