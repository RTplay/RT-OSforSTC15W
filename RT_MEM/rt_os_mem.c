#include "rt_os.h"
#include "rt_os_private.h"

#ifdef MEM_ENABLE

#define     OS_MEM_STRUCT_SIZE (2)
#define     MEM_USED (0x8000)

static datax u8 mem[OS_MEM_SIZE] = {0};
static u16 mem_size = 0;

// 初始化内存头部
u8 os_mem_init (void)
{
    u16 *mem_head = (u16 *)mem;
    if (OS_MEM_SIZE <= 2)
        return OS_ERR_MEM_INVALID_SIZE;
    if (OS_MEM_SIZE % 2) //2字节对齐，如果未对齐则缩小一字节
        mem_size = OS_MEM_SIZE - 1;
    else
        mem_size = OS_MEM_SIZE;
    *mem_head = mem_size - 2; //存入可用字节数

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
    return (void *)ret_ptr;
}

//free功能
void os_free (void *free_ptr)
{
    u16 *mem_ptr;
    if (free_ptr == NULL) {
        return;
    }
    u8 *mem_head = (u8 *)free_ptr - OS_MEM_STRUCT_SIZE;
    mem_ptr = (u16 *)mem_head;
    *mem_ptr &= ~MEM_USED;
    //遍历内存，合拼空闲内存
    mem_ptr = (u16 *)mem;
    u16 *last_free = NULL;
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
}


#endif
