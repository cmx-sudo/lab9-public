#ifndef MALLOC_H
#define MALLOC_H

#include <stddef.h>
#include <stdint.h>

// 内存块头部结构
typedef struct block_header {
    size_t size;                    // 块大小（包括头部）
    int is_free;                    // 是否空闲
    struct block_header *next;      // 下一个块
    struct block_header *prev;      // 上一个块
} block_header_t;

// 内存管理器的全局状态
typedef struct {
    void *heap_start;               // 堆起始地址
    size_t heap_size;               // 堆大小
    block_header_t *free_list;      // 空闲块链表
    int initialized;                // 是否已初始化
} malloc_state_t;

// 函数声明
void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);

// 内部函数
void malloc_init(void);
block_header_t *find_free_block(size_t size);
block_header_t *split_block(block_header_t *block, size_t size);
void merge_blocks(block_header_t *block);
void add_to_free_list(block_header_t *block);
void remove_from_free_list(block_header_t *block);

// 系统调用接口
int sys_malloc(void *args);
int sys_free(void *args);
int sys_calloc(void *args);
int sys_realloc(void *args);

#endif // MALLOC_H 