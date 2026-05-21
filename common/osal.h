/**
 * @file osal.h
 * @brief 操作系统抽象层 (OSAL) -- 时间戳、互斥锁、信号量
 *
 * @details
 * 提供与平台无关的操作系统原语封装：
 *   - 时间戳: OSAL_GetCpuStartInMsec/Usec, get_monotonic_pts_us, get_monotonic_ms
 *   - 互斥锁: T_MutexObj + OSAL_MutexInit/Destroy/Lock/Unlock
 *   - 信号量: T_SemaphoreObj + OSAL_SemaphoreInit/Destroy/Wait/Post/TryWait/GetValue
 *
 * @note 本模块与 ctos.h 中的 g_mutex_t/g_sem_t 是两套独立的 OS 抽象，
 *       类型不可混用。ctos.h 是旧式 API（os_ 前缀），本文件是新式 API（OSAL_ 前缀）。
 *
 * @see defs.h（依赖 UINT32, INT64）
 * @see common.h, uart.h, request_que.c（被依赖）
 */

#ifndef OSAL_H
#define OSAL_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "defs.h"

// 定义一个结构体，用于存储系统时间
typedef struct {
    long long timestamp_ms;
    long long timestamp_us;
    time_t timestamp_s;
} osal_timestamp_t;

// OSAL 获取当前时间戳的接口函数
osal_timestamp_t osal_get_current_timestamp();


/** @brief 获取从系统启动以来的毫秒数 */
UINT32 OSAL_GetCpuStartInMsec(void);

/** @brief 获取从系统启动以来的微秒数 */
UINT32 OSAL_GetCpuStartInUsec(void);

/** @brief 获取单调递增的微秒时间戳 */
INT64 get_monotonic_pts_us(void);

/** @brief 获取单调递增的毫秒时间戳 */
UINT32 get_monotonic_ms(void);

/** @brief 互斥锁对象 */
typedef struct {
	pthread_mutex_t mutex;
	int flag;
} T_MutexObj;

/** @brief 初始化互斥锁 @param[in,out] mutex 互斥锁对象指针 */
void OSAL_MutexInit(T_MutexObj* mutex);

/** @brief 销毁互斥锁 @param[in,out] mutex 互斥锁对象指针 */
void OSAL_MutexDestroy(T_MutexObj* mutex);

/** @brief 加锁 @param[in,out] mutex 互斥锁对象指针 */
void OSAL_MutexLock(T_MutexObj* mutex);

/** @brief 解锁 @param[in,out] mutex 互斥锁对象指针 */
void OSAL_MutexUnlock(T_MutexObj* mutex);


/* 信号量相关类型和函数族 */
typedef struct {
    sem_t semaphore;      // POSIX信号量
    int max_count;        // 最大计数值
    int initial_count;    // 初始计数值
} T_SemaphoreObj;

/**
 * @brief 初始化信号量
 * @param sem 信号量对象指针
 * @param max_count 最大计数值
 * @param initial_count 初始计数值
 * @return 0成功，其他失败
 */
int OSAL_SemaphoreInit(T_SemaphoreObj* sem, int max_count, int initial_count);

/**
 * @brief 销毁信号量
 * @param sem 信号量对象指针
 */
void OSAL_SemaphoreDestroy(T_SemaphoreObj* sem);

/**
 * @brief 等待信号量（P操作）
 * @param sem 信号量对象指针
 * @param timeout_ms 超时时间（毫秒），-1表示无限等待
 * @return 0成功，-1超时，其他错误
 */
int OSAL_SemaphoreWait(T_SemaphoreObj* sem, int timeout_ms);

/**
 * @brief 发布信号量（V操作）
 * @param sem 信号量对象指针
 * @return 0成功，其他失败
 */
int OSAL_SemaphorePost(T_SemaphoreObj* sem);

/**
 * @brief 尝试等待信号量（非阻塞）
 * @param sem 信号量对象指针
 * @return 0成功，-1无信号量可用
 */
int OSAL_SemaphoreTryWait(T_SemaphoreObj* sem);

/**
 * @brief 获取当前信号量计数值
 * @param sem 信号量对象指针
 * @return 当前计数值，-1表示错误
 */
int OSAL_SemaphoreGetValue(T_SemaphoreObj* sem);

#endif // OSAL_H
