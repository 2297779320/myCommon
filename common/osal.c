#include "osal.h"
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <errno.h>

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

/* 信号量实现 */

int OSAL_SemaphoreInit(T_SemaphoreObj* sem, int max_count, int initial_count)
{
    if (!sem || max_count <= 0 || initial_count < 0 || initial_count > max_count)
        return -1;
    if (sem_init(&sem->semaphore, 0, (unsigned int)initial_count) != 0)
        return -1;
    sem->max_count = max_count;
    sem->initial_count = initial_count;
    return 0;
}

void OSAL_SemaphoreDestroy(T_SemaphoreObj* sem)
{
    if (!sem) return;
    sem_destroy(&sem->semaphore);
}

int OSAL_SemaphoreWait(T_SemaphoreObj* sem, int timeout_ms)
{
    if (!sem) return -1;
    if (timeout_ms < 0) {
        /* 无限等待 */
        while (sem_wait(&sem->semaphore) != 0) {
            if (errno != EINTR) return -1;
        }
        return 0;
    } else if (timeout_ms == 0) {
        /* 非阻塞 */
        return (sem_trywait(&sem->semaphore) == 0) ? 0 : -1;
    } else {
        /* 带超时等待 */
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000L;
        }
        while (sem_timedwait(&sem->semaphore, &ts) != 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        return 0;
    }
}

int OSAL_SemaphorePost(T_SemaphoreObj* sem)
{
    if (!sem) return -1;
    return (sem_post(&sem->semaphore) == 0) ? 0 : -1;
}

int OSAL_SemaphoreTryWait(T_SemaphoreObj* sem)
{
    if (!sem) return -1;
    return (sem_trywait(&sem->semaphore) == 0) ? 0 : -1;
}

int OSAL_SemaphoreGetValue(T_SemaphoreObj* sem)
{
    if (!sem) return -1;
    int value = 0;
    if (sem_getvalue(&sem->semaphore, &value) != 0)
        return -1;
    return value;
}