#include "malloc.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>

// 在Ubuntu系统上，sbrk可能不在unistd.h中，需要手动声明
#ifndef __APPLE__
extern void *sbrk(intptr_t increment);
#endif

static malloc_state_t malloc_state = {0};

#define MIN_BLOCK_SIZE (sizeof(block_header_t) + 8)
#define HEAP_SIZE (1024 * 1024)

void malloc_init(void) {
    if (malloc_state.initialized) {
        return;
    }
    
    // 在Ubuntu系统上，如果sbrk不可用，使用静态内存
    #ifdef __linux__
    // 使用静态内存作为堆
    static char static_heap[HEAP_SIZE];
    malloc_state.heap_start = static_heap;
    #else
    malloc_state.heap_start = sbrk(HEAP_SIZE);
    if (malloc_state.heap_start == (void*)-1) {
        return; 
    }
    #endif
    
    malloc_state.heap_size = HEAP_SIZE;
    malloc_state.free_list = NULL;
    malloc_state.initialized = 1;
    
    block_header_t *initial_block = (block_header_t*)malloc_state.heap_start;
    initial_block->size = HEAP_SIZE;
    initial_block->is_free = 1;
    initial_block->next = NULL;
    initial_block->prev = NULL;
    
    malloc_state.free_list = initial_block;
}


block_header_t *find_free_block(size_t size) {
    block_header_t *current = malloc_state.free_list;
    
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    
    return NULL; // 没有找到合适的块
}

// 分割块
block_header_t *split_block(block_header_t *block, size_t size) {
    if (block->size < size + MIN_BLOCK_SIZE) {
        // 块太小，无法分割
        return block;
    }
    
    // 创建新块
    block_header_t *new_block = (block_header_t*)((char*)block + size);
    new_block->size = block->size - size;
    new_block->is_free = 1;
    new_block->next = block->next;
    new_block->prev = block;
    
    // 更新原块
    block->size = size;
    block->next = new_block;
    
    // 更新链表
    if (new_block->next != NULL) {
        new_block->next->prev = new_block;
    }
    
    return block;
}

// 合并相邻的空闲块
void merge_blocks(block_header_t *block) {
    // 向后合并
    if (block->next != NULL && block->next->is_free) {
        block->size += block->next->size;
        block->next = block->next->next;
        if (block->next != NULL) {
            block->next->prev = block;
        }
    }
    
    // 向前合并
    if (block->prev != NULL && block->prev->is_free) {
        block->prev->size += block->size;
        block->prev->next = block->next;
        if (block->next != NULL) {
            block->next->prev = block->prev;
        }
    }
}

// 添加到空闲链表
void add_to_free_list(block_header_t *block) {
    block->next = malloc_state.free_list;
    block->prev = NULL;
    if (malloc_state.free_list != NULL) {
        malloc_state.free_list->prev = block;
    }
    malloc_state.free_list = block;
}

// 从空闲链表移除
void remove_from_free_list(block_header_t *block) {
    if (block->prev != NULL) {
        block->prev->next = block->next;
    } else {
        malloc_state.free_list = block->next;
    }
    
    if (block->next != NULL) {
        block->next->prev = block->prev;
    }
}

// malloc实现
void *malloc(size_t size) {
    if (!malloc_state.initialized) {
        malloc_init();
    }
    
    if (size == 0) {
        return NULL;
    }
    
    // 计算需要的总大小（包括头部）
    size_t total_size = size + sizeof(block_header_t);
    if (total_size < MIN_BLOCK_SIZE) {
        total_size = MIN_BLOCK_SIZE;
    }
    
    // 对齐到8字节边界
    total_size = (total_size + 7) & ~7;
    
    // 查找合适的空闲块
    block_header_t *block = find_free_block(total_size);
    if (block == NULL) {
        // 没有找到合适的块，返回NULL
        return NULL;
    }
    
    // 分割块（如果需要）
    block = split_block(block, total_size);
    
    // 标记为已使用
    block->is_free = 0;
    
    // 从空闲链表移除
    remove_from_free_list(block);
    
    // 返回用户数据区域
    return (void*)((char*)block + sizeof(block_header_t));
}

// free实现
void free(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    
    if (!malloc_state.initialized) {
        return;
    }
    
    // 获取块头部
    block_header_t *block = (block_header_t*)((char*)ptr - sizeof(block_header_t));
    
    // 检查块是否在有效范围内
    if ((char*)block < (char*)malloc_state.heap_start || 
        (char*)block >= (char*)malloc_state.heap_start + malloc_state.heap_size) {
        return; // 无效指针
    }
    
    // 标记为空闲
    block->is_free = 1;
    
    // 添加到空闲链表
    add_to_free_list(block);
    
    // 尝试合并相邻的空闲块
    merge_blocks(block);
}

// calloc实现
void *calloc(size_t nmemb, size_t size) {
    size_t total_size = nmemb * size;
    void *ptr = malloc(total_size);
    
    if (ptr != NULL) {
        memset(ptr, 0, total_size);
    }
    
    return ptr;
}

// realloc实现
void *realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return malloc(size);
    }
    
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    
    // 获取原块信息
    block_header_t *block = (block_header_t*)((char*)ptr - sizeof(block_header_t));
    size_t old_size = block->size - sizeof(block_header_t);
    
    if (size <= old_size) {
        // 新大小小于等于原大小，直接返回原指针
        return ptr;
    }
    
    // 分配新内存
    void *new_ptr = malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }
    
    // 复制数据
    memcpy(new_ptr, ptr, old_size);
    
    // 释放原内存
    free(ptr);
    
    return new_ptr;
}

// 系统调用接口实现
int sys_malloc(void *args) {
    size_t size = *(size_t*)args;
    void *result = malloc(size);
    
    // 在实际系统中，这里需要将结果返回给用户进程
    // 这里简化处理，直接返回地址
    return (intptr_t)result;
}

int sys_free(void *args) {
    void *ptr = *(void**)args;
    free(ptr);
    return 0;
}

int sys_calloc(void *args) {
    size_t nmemb = ((size_t*)args)[0];
    size_t size = ((size_t*)args)[1];
    void *result = calloc(nmemb, size);
    return (intptr_t)result;
}

int sys_realloc(void *args) {
    if (args == NULL) {
        return 0; // 返回NULL指针
    }
    
    size_t *arg_array = (size_t*)args;
    void *ptr = (void*)arg_array[0];
    size_t size = arg_array[1];
    
    // 验证指针有效性
    if (ptr == NULL) {
        // 如果原指针为NULL，相当于malloc
        void *result = malloc(size);
        return (intptr_t)result;
    }
    
    void *result = realloc(ptr, size);
    return (intptr_t)result;
} 