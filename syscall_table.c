#include "malloc.h"

// 系统调用表
// 在实际系统中，这个表应该与内核的系统调用处理机制集成

// 系统调用号定义
#define SYS_MALLOC  100
#define SYS_FREE    101
#define SYS_CALLOC  102
#define SYS_REALLOC 103

// 系统调用处理函数指针类型
typedef int (*syscall_handler_t)(void *args);

// 系统调用表结构
typedef struct {
    int syscall_num;
    syscall_handler_t handler;
    const char *name;
} syscall_entry_t;

// 系统调用表
static syscall_entry_t syscall_table[] = {
    {SYS_MALLOC,  sys_malloc,  "malloc"},
    {SYS_FREE,    sys_free,    "free"},
    {SYS_CALLOC,  sys_calloc,  "calloc"},
    {SYS_REALLOC, sys_realloc, "realloc"},
    {0, NULL, NULL} // 结束标记
};

// 根据系统调用号查找处理函数
syscall_handler_t get_syscall_handler(int syscall_num) {
    for (int i = 0; syscall_table[i].handler != NULL; i++) {
        if (syscall_table[i].syscall_num == syscall_num) {
            return syscall_table[i].handler;
        }
    }
    return NULL;
}

// 获取系统调用名称
const char *get_syscall_name(int syscall_num) {
    for (int i = 0; syscall_table[i].handler != NULL; i++) {
        if (syscall_table[i].syscall_num == syscall_num) {
            return syscall_table[i].name;
        }
    }
    return "unknown";
}

// 系统调用分发函数
int dispatch_syscall(int syscall_num, void *args) {
    syscall_handler_t handler = get_syscall_handler(syscall_num);
    if (handler == NULL) {
        // 未知的系统调用
        return -1;
    }
    
    return handler(args);
} 