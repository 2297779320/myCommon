#ifndef TSK_H
#define TSK_H

#include <pthread.h>
#include "defs.h"
EXTERN_C_BLOCK

// TSK 句柄类型
typedef pthread_t* TSK_Handle;

// 任务函数指针类型
typedef void * (*TSK_Fxn)(void*);

// TSK 属性结构体
typedef struct {
    const char* name;    // 任务名称
    BOOL bFifo;         // 是否使用FIFO调度策略
    int priority;       // 任务优先级
    size_t stackSize;   // 堆栈大小
    uint32_t cpuAffinityMask; // CPU亲和性掩码（每位代表一个CPU核心）
    // 可以添加其他属性字段
} TSK_Attrs;

// CPU掩码操作宏
#define CPU_ZERO_MASK(mask) (*(mask) = 0U)
#define CPU_SET_MASK(cpu, mask) (*(mask) |= (1U << (cpu)))
#define CPU_CLR_MASK(cpu, mask) (*(mask) &= ~(1U << (cpu)))
#define CPU_ISSET_MASK(cpu, mask) ((*(mask) & (1U << (cpu))) != 0)
#define CPU_COUNT_MASK(mask) (__builtin_popcount(*(mask)))

// 默认TSK属性
#define DEFAULT_TSK_ATTR { \
    .name = "DefaultTask", \
    .bFifo = FALSE, \
    .priority = 10, \
    .stackSize = 1024 * 1024, \
    .cpuAffinityMask = 0xFFFFFFFFU /* 默认所有CPU */ \
}

TSK_Handle TSK_create(TSK_Fxn fxn, const TSK_Attrs * attrs, void * arg);

void TSK_delete(TSK_Handle hTsk);

void TSK_sleep(unsigned int milliseconds);

EXTERN_C_BLOCK_END
#endif

