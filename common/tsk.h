/**
 * @file tsk.h
 * @brief 任务管理模块 -- 线程创建、销毁、休眠
 *
 * @details
 * 提供简化的线程管理接口，支持设置任务名称、调度策略、优先级、堆栈大小和 CPU 亲和性。
 * 底层基于 pthread 实现。
 *
 * @see defs.h（依赖 BOOL, EXTERN_C_BLOCK）
 * @see common.h（被依赖）
 */

#ifndef TSK_H
#define TSK_H

#include <pthread.h>
#include "defs.h"
EXTERN_C_BLOCK

/** @brief TSK 句柄类型 */
typedef pthread_t* TSK_Handle;

/** @brief 任务函数指针类型 */
typedef void * (*TSK_Fxn)(void*);

/**
 * @brief TSK 属性结构体
 */
typedef struct {
    const char* name;           /**< 任务名称 */
    BOOL bFifo;                 /**< 是否使用FIFO调度策略 */
    int priority;               /**< 任务优先级 */
    size_t stackSize;           /**< 堆栈大小 */
    uint32_t cpuAffinityMask;   /**< CPU亲和性掩码（每位代表一个CPU核心） */
} TSK_Attrs;

/** @brief 清零CPU掩码 @param[out] mask 掩码指针 */
#define CPU_ZERO_MASK(mask) (*(mask) = 0U)
/** @brief 设置CPU掩码位 @param[in] cpu CPU编号 @param[in,out] mask 掩码指针 */
#define CPU_SET_MASK(cpu, mask) (*(mask) |= (1U << (cpu)))
/** @brief 清除CPU掩码位 @param[in] cpu CPU编号 @param[in,out] mask 掩码指针 */
#define CPU_CLR_MASK(cpu, mask) (*(mask) &= ~(1U << (cpu)))
/** @brief 检查CPU掩码位 @param[in] cpu CPU编号 @param[in] mask 掩码指针 @return 是否设置 */
#define CPU_ISSET_MASK(cpu, mask) ((*(mask) & (1U << (cpu))) != 0)
/** @brief 统计掩码中置位数量 @param[in] mask 掩码指针 @return 置位数量 */
#define CPU_COUNT_MASK(mask) (__builtin_popcount(*(mask)))

/** @brief 默认TSK属性 */
#define DEFAULT_TSK_ATTR { \
    .name = "DefaultTask", \
    .bFifo = FALSE, \
    .priority = 10, \
    .stackSize = 1024 * 1024, \
    .cpuAffinityMask = 0xFFFFFFFFU /* 默认所有CPU */ \
}

/**
 * @brief 创建任务
 * @param[in] fxn 任务函数指针
 * @param[in] attrs 任务属性，NULL 使用默认属性
 * @param[in] arg 传递给任务函数的参数
 * @return 任务句柄，失败返回 NULL
 */
TSK_Handle TSK_create(TSK_Fxn fxn, const TSK_Attrs * attrs, void * arg);

/**
 * @brief 删除任务（等待线程结束后释放句柄）
 * @param[in] hTsk 任务句柄
 */
void TSK_delete(TSK_Handle hTsk);

/**
 * @brief 任务休眠
 * @param[in] milliseconds 休眠时间（毫秒）
 */
void TSK_sleep(unsigned int milliseconds);

EXTERN_C_BLOCK_END
#endif

