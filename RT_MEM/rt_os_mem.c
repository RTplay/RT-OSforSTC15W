#include "rt_os.h"
#include "rt_os_private.h"

#ifdef MEM_ENABLE

#define     OS_MEM_STRUCT_SIZE (2)
#define     MEM_USED (0x8000)

static datax u8 mem[OS_MEM_SIZE] = {0};
static u16 mem_size = 0;

// ��ʼ���ڴ�ͷ��
u8 os_mem_init (void)
{
    u16 *mem_head = (u16 *)mem;
    if (OS_MEM_SIZE <= 2)
        return OS_ERR_MEM_INVALID_SIZE;
    if (OS_MEM_SIZE % 2) //2�ֽڶ��룬���δ��������Сһ�ֽ�
        mem_size = OS_MEM_SIZE - 1;
    else
        mem_size = OS_MEM_SIZE;
    *mem_head = mem_size - 2; //��������ֽ���

    return OS_ERR_NONE;
}

// malloc����
void *os_malloc (u16 c_size)
{
    u8 *ret_ptr;
    u16 *mem_ptr = (u16 *)mem;
    u16 index = 0;//��¼����ű�
    if (c_size == 0)
        return NULL;
    if (c_size % 2) {
        c_size += 1;//���ֽڶ���
    }

    while((*mem_ptr & ~MEM_USED) < OS_MEM_SIZE) { // ѭ���鿴���һ��֮ǰ�Ŀ�������
        //�ڴ�鱻ʹ�û������е��ڴ治����Ҫ�Ĵ�С
        if ((*mem_ptr & MEM_USED) || ((*mem_ptr - index - OS_MEM_STRUCT_SIZE) < c_size)) {
            u16 mem_index = *mem_ptr & ~MEM_USED; //������һ���ڴ�ṹ��
            index = mem_index;
            mem_ptr = (u16 *)&mem[mem_index];
        } else { //����ʹ��
            u16 next_addr = *mem_ptr;
            *mem_ptr = c_size + index + OS_MEM_STRUCT_SIZE;
            *mem_ptr |= MEM_USED;
            ret_ptr = (u8 *)mem_ptr + OS_MEM_STRUCT_SIZE;
            mem_ptr = (u16 *)&mem[c_size + index + OS_MEM_STRUCT_SIZE];
            *mem_ptr = next_addr;
            break;
        }
    }
    //�������һ���ڴ��
    if ((*mem_ptr & ~MEM_USED) == OS_MEM_SIZE) {
        if (*mem_ptr & MEM_USED) {
            u16 mem_index = *mem_ptr & ~MEM_USED;
            index = mem_index;
            mem_ptr = (u16 *)&mem[*mem_ptr];
            ret_ptr = NULL;
        } else {
            if ((OS_MEM_SIZE - index - OS_MEM_STRUCT_SIZE) < c_size) { //ʣ���ڴ治��
                return NULL;
            } else if ((OS_MEM_SIZE - index - OS_MEM_STRUCT_SIZE) == c_size) { //�ڴ��С����
                *mem_ptr |= MEM_USED;
                ret_ptr = (u8 *)mem_ptr + OS_MEM_STRUCT_SIZE;
            } else { //�ڴ湻��
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

//free����
void os_free (void *free_ptr)
{
    u16 *mem_ptr;
    if (free_ptr == NULL) {
        return;
    }
    u8 *mem_head = (u8 *)free_ptr - OS_MEM_STRUCT_SIZE;
    mem_ptr = (u16 *)mem_head;
    *mem_ptr &= ~MEM_USED;
    //�����ڴ棬��ƴ�����ڴ�
    mem_ptr = (u16 *)mem;
    u16 *last_free = NULL;
    while((*mem_ptr & ~MEM_USED) < OS_MEM_SIZE) { // ѭ���鿴���һ��֮ǰ������
        if ((*mem_ptr & MEM_USED) == 0) { //��һ���ǿ��е�
            if (last_free) {
                *last_free = *mem_ptr;
                mem_ptr = (u16 *)&mem[*mem_ptr];
            } else {
                //������һ���ڴ�ṹ��
                last_free = mem_ptr;
                mem_ptr = (u16 *)&mem[*mem_ptr];
            }
        } else { //��ʹ���ڴ�
            u16 next_index = (*mem_ptr & ~MEM_USED); //������һ���ڴ�ṹ��
            last_free = NULL;
            mem_ptr = (u16 *)&mem[next_index];
        }
    }
    //�������һ���ڴ��
    if ((*mem_ptr & ~MEM_USED) == OS_MEM_SIZE) {
        if ((*mem_ptr & MEM_USED) == 0) { //��һ���ǿ��е�
            if (last_free) {
                *last_free = *mem_ptr;
            }
        } else { //��ʹ���ڴ�
            last_free = NULL;
        }
    }
}


#endif
