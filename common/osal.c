#include "osal.h"
#include <sys/time.h>
#include <time.h>
#include <stdint.h>

osal_timestamp_t osal_get_current_timestamp()
{
    osal_timestamp_t ts;
    struct timeval tv;
    gettimeofday(&tv, NULL);

    ts.timestamp_s = tv.tv_sec;
    ts.timestamp_ms = (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
    ts.timestamp_us = (long long)tv.tv_sec * 1000000 + tv.tv_usec;

    return ts;
}

// 返回从系统启动以来的毫秒数（使用单调时钟，避免 Epoch 溢出）
// 注意：UINT32 约 49 天后自然回绕，调用方应使用差值比较
UINT32 OSAL_GetCpuStartInMsec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (UINT32)((uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

// 返回从系统启动以来的微秒数（使用单调时钟）
// 注意：UINT32 约 71 分钟后自然回绕，调用方应使用差值比较
UINT32 OSAL_GetCpuStartInUsec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (UINT32)((uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
}

void OSAL_MutexInit(T_MutexObj *mutex)
{
    if (mutex == NULL)
    {
        return;
    }
    pthread_mutex_init(&mutex->mutex, NULL);
    mutex->flag = 1;
}

void OSAL_MutexDestroy(T_MutexObj *mutex)
{
    if (mutex == NULL)
    {
        return;
    }
    pthread_mutex_destroy(&mutex->mutex);
    mutex->flag = 0;
}

void OSAL_MutexLock(T_MutexObj *mutex)
{
    if (mutex == NULL)
    {
        return;
    }
    pthread_mutex_lock(&mutex->mutex);
}

void OSAL_MutexUnlock(T_MutexObj *mutex)
{
    if (mutex == NULL)
    {
        return;
    }
    pthread_mutex_unlock(&mutex->mutex);
}

INT64 get_monotonic_pts_us(void)
{
    struct timespec ts;
    // 读取单调时钟（纳秒级）
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
        perror("clock_gettime failed");
        return -1;
    }
    // 转换为微秒（1秒=1e6微秒，1纳秒=1e-3微秒）
    return (INT64)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

UINT32 get_monotonic_ms(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
        perror("clock_gettime failed");
        return 0;
    }
    // 只取低32位，允许自然回绕
    return (UINT32)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}