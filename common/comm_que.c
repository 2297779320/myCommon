#include "comm_que.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include "log.h"

// 队列结构定义
typedef struct {
    pthread_mutex_t mutex;           // 互斥锁
    pthread_cond_t cond_full;        // 满队列条件变量
    pthread_cond_t cond_empty;       // 空队列条件变量
    
    uint8_t* buffer;                 // 队列缓冲区
    uint32_t maxCount;               // 最大元素数量
    size_t elementSize;              // 每个元素的大小
    
    uint32_t write;                   // 队列写指针
    uint32_t read;                   // 队列读指针
    uint32_t fullCount;              // 当前满元素数量
    uint32_t emptyCount;             // 当前空元素数量
    
    bool isDestroyed;                // 队列是否已销毁标志
    
    // 添加内存释放函数指针
    void (*pFreeFunc)(void* ptr);    // 自定义内存释放函数
} CommQue_t;

// 计算超时时间
static void _calculate_timeout(struct timespec* ts, int32_t timeout_ms) {
    if (timeout_ms == OSAL_TIMEOUT_FOREVER) {
        return;
    }
    
    clock_gettime(CLOCK_REALTIME, ts);
    ts->tv_sec += timeout_ms / 1000;
    ts->tv_nsec += (timeout_ms % 1000) * 1000000;
    
    if (ts->tv_nsec >= 1000000000) {
        ts->tv_sec += 1;
        ts->tv_nsec -= 1000000000;
    }
}

CommQueID CommQue_Create(uint32_t maxCount, size_t elementSize,
                        void* (*pAllocFunc)(size_t size)) {
    // 使用自定义或默认的内存分配函数
    void* (*alloc_func)(size_t) = pAllocFunc ? pAllocFunc : malloc;
    void (*free_func)(void*) = pAllocFunc ? (void(*)(void*))free : free;
    
    // 分配队列控制结构内存
    CommQue_t* pQue = (CommQue_t*)alloc_func(sizeof(CommQue_t));
    if (!pQue) {
        return NULL;
    }
    
    // 初始化互斥锁和条件变量
    if (pthread_mutex_init(&pQue->mutex, NULL) != 0) {
        free_func(pQue);
        return NULL;
    }
    
    if (pthread_cond_init(&pQue->cond_full, NULL) != 0) {
        pthread_mutex_destroy(&pQue->mutex);
        free_func(pQue);
        return NULL;
    }
    
    if (pthread_cond_init(&pQue->cond_empty, NULL) != 0) {
        pthread_cond_destroy(&pQue->cond_full);
        pthread_mutex_destroy(&pQue->mutex);
        free_func(pQue);
        return NULL;
    }
    
    // 分配队列缓冲区内存
    pQue->buffer = (uint8_t*)alloc_func(maxCount * elementSize);
    if (!pQue->buffer) {
        pthread_cond_destroy(&pQue->cond_empty);
        pthread_cond_destroy(&pQue->cond_full);
        pthread_mutex_destroy(&pQue->mutex);
        free_func(pQue);
        return NULL;
    }
    
    memset(pQue->buffer, 0, maxCount * elementSize);

    // 初始化队列参数
    pQue->maxCount = maxCount;
    pQue->elementSize = elementSize;
    pQue->write = 0;
    pQue->read = 0;
    pQue->fullCount = 0;
    pQue->emptyCount = maxCount;
    pQue->isDestroyed = false;
    pQue->pFreeFunc = free_func;
    
    return (CommQueID)pQue;
}

void CommQue_Delete(CommQueID hQue) {
    if (!hQue) return;
    
    CommQue_t* pQue = (CommQue_t*)hQue;
    void (*free_func)(void*) = pQue->pFreeFunc;
    
    pthread_mutex_lock(&pQue->mutex);
    pQue->isDestroyed = true;
    pthread_cond_broadcast(&pQue->cond_full);
    pthread_cond_broadcast(&pQue->cond_empty);
    pthread_mutex_unlock(&pQue->mutex);

    /* 给等待线程时间从 cond_wait 返回并释放锁，避免 destroy 时仍有线程持有锁 */
    struct timespec ts = {0, 5000000}; /* 5ms */
    nanosleep(&ts, NULL);

    pthread_mutex_destroy(&pQue->mutex);
    pthread_cond_destroy(&pQue->cond_full);
    pthread_cond_destroy(&pQue->cond_empty);
    
    free_func(pQue->buffer);
    free_func(pQue);
}

void CommQue_Clear(CommQueID hQue) {
    if (!hQue) return;
    
    CommQue_t* pQue = (CommQue_t*)hQue;
    
    pthread_mutex_lock(&pQue->mutex);
    pQue->write = 0;
    pQue->read = 0;
    pQue->fullCount = 0;
    pQue->emptyCount = pQue->maxCount;
    /* 通知等待空槽的线程队列已清空，有空槽可用 */
    pthread_cond_broadcast(&pQue->cond_empty);
    pthread_mutex_unlock(&pQue->mutex);
}

void* CommQue_GetFull(CommQueID hQue, int32_t timeout) {
    if (!hQue) return NULL;
    
    CommQue_t* pQue = (CommQue_t*)hQue;
    struct timespec ts = {0};
    
    pthread_mutex_lock(&pQue->mutex);

    /* 超时时间点在循环外计算一次，避免虚假唤醒后叠加 */
    if (timeout != OSAL_TIMEOUT_NONE && timeout != OSAL_TIMEOUT_FOREVER) {
        _calculate_timeout(&ts, timeout);
    }

    while (pQue->fullCount == 0 && !pQue->isDestroyed) {
        if (timeout == OSAL_TIMEOUT_NONE) {
            pthread_mutex_unlock(&pQue->mutex);
            return NULL;
        }
        
        if (timeout != OSAL_TIMEOUT_FOREVER) {
            if (pthread_cond_timedwait(&pQue->cond_full, &pQue->mutex, &ts) == ETIMEDOUT) {
                pthread_mutex_unlock(&pQue->mutex);
                return NULL;
            }
        } else {
            pthread_cond_wait(&pQue->cond_full, &pQue->mutex);
        }
    }
    
    if (pQue->isDestroyed) {
        pthread_mutex_unlock(&pQue->mutex);
        return NULL;
    }
    
    void* pElement = pQue->buffer + (pQue->read * pQue->elementSize);
    pQue->read = (pQue->read + 1) % pQue->maxCount;
    pQue->fullCount--;
    /* emptyCount 不在此处加一，cond_empty 也不在此处发送信号。
     * 调用者持有该槽直到 PutEmpty 归还，届时再更新计数和唤醒等待者。*/
    pthread_mutex_unlock(&pQue->mutex);

    return pElement;
}

void* CommQue_PeekFull(CommQueID hQue) {
    if (!hQue) return NULL;
    
    CommQue_t* pQue = (CommQue_t*)hQue;
    pthread_mutex_lock(&pQue->mutex);
    
    if(pQue->fullCount == 0 || pQue->isDestroyed) {
        pthread_mutex_unlock(&pQue->mutex);
        return NULL;
    }
        
    void* pElement = pQue->buffer + (pQue->read * pQue->elementSize);
    pthread_mutex_unlock(&pQue->mutex);
    return pElement;
}

void* CommQue_GetEmpty(CommQueID hQue, int32_t timeout) {
    if (!hQue) return NULL;
    
    CommQue_t* pQue = (CommQue_t*)hQue;
    struct timespec ts = {0};
    
    pthread_mutex_lock(&pQue->mutex);

    /* 超时时间点在循环外计算一次，避免虚假唤醒后叠加 */
    if (timeout != OSAL_TIMEOUT_NONE && timeout != OSAL_TIMEOUT_FOREVER) {
        _calculate_timeout(&ts, timeout);
    }

    while (pQue->emptyCount == 0 && !pQue->isDestroyed) {
        if (timeout == OSAL_TIMEOUT_NONE) {
            pthread_mutex_unlock(&pQue->mutex);
            return NULL;
        }
        
        if (timeout != OSAL_TIMEOUT_FOREVER) {
            if (pthread_cond_timedwait(&pQue->cond_empty, &pQue->mutex, &ts) == ETIMEDOUT) {
                pthread_mutex_unlock(&pQue->mutex);
                return NULL;
            }
        } else {
            pthread_cond_wait(&pQue->cond_empty, &pQue->mutex);
        }
    }
    
    if (pQue->isDestroyed) {
        pthread_mutex_unlock(&pQue->mutex);
        return NULL;
    }
    
    void* pElement = pQue->buffer + (pQue->write * pQue->elementSize);
    pQue->write = (pQue->write + 1) % pQue->maxCount;
    pQue->emptyCount--;

    pthread_mutex_unlock(&pQue->mutex);

    return pElement;
}

int32_t CommQue_PutEmpty(CommQueID hQue, void* pElement) {
    if (!hQue || !pElement) return -1;
    
    CommQue_t* pQue = (CommQue_t*)hQue;
    
    pthread_mutex_lock(&pQue->mutex);
    pQue->emptyCount++;
    pthread_cond_signal(&pQue->cond_empty);
    pthread_mutex_unlock(&pQue->mutex);
    return 0;
}

int32_t CommQue_PutFull(CommQueID hQue, void* pElement) {
    if (!hQue || !pElement) return -1;
    
    CommQue_t* pQue = (CommQue_t*)hQue;
    
    pthread_mutex_lock(&pQue->mutex);
    pQue->fullCount++;
    pthread_cond_signal(&pQue->cond_full);
    pthread_mutex_unlock(&pQue->mutex);

    return 0;
}

int32_t CommQue_PutFullFront(CommQueID hQue, void* pElement) {
    if (!hQue || !pElement) return -1;

    CommQue_t* pQue = (CommQue_t*)hQue;

    pthread_mutex_lock(&pQue->mutex);

    if (pQue->fullCount >= pQue->maxCount || pQue->isDestroyed) {
        pthread_mutex_unlock(&pQue->mutex);
        return -1;
    }

    /* read 回退一格，新槽成为下一次 GetFull 取到的第一个元素 */
    pQue->read = (pQue->read + pQue->maxCount - 1) % pQue->maxCount;
    pQue->fullCount++;

    pthread_cond_signal(&pQue->cond_full);
    pthread_mutex_unlock(&pQue->mutex);

    return 0;
}

int32_t CommQue_PutEmptyFront(CommQueID hQue, void* pElement) {
    if (!hQue || !pElement) return -1;

    CommQue_t* pQue = (CommQue_t*)hQue;

    pthread_mutex_lock(&pQue->mutex);

    if (pQue->emptyCount >= pQue->maxCount || pQue->isDestroyed) {
        pthread_mutex_unlock(&pQue->mutex);
        return -1;
    }

    /* write 回退一格，新槽成为下一次 GetEmpty 取到的第一个空槽 */
    pQue->write = (pQue->write + pQue->maxCount - 1) % pQue->maxCount;
    pQue->emptyCount++;

    pthread_cond_signal(&pQue->cond_empty);
    pthread_mutex_unlock(&pQue->mutex);

    return 0;
}

uint32_t CommQue_GetFullCount(CommQueID hQue) {
    if (!hQue) return 0;
    
    CommQue_t* pQue = (CommQue_t*)hQue;
    uint32_t count;
    
    pthread_mutex_lock(&pQue->mutex);
    count = pQue->fullCount;
    pthread_mutex_unlock(&pQue->mutex);
    
    return count;
}

uint32_t CommQue_GetEmptyCount(CommQueID hQue) {
    if (!hQue) return 0;
    
    CommQue_t* pQue = (CommQue_t*)hQue;
    uint32_t count;
    
    pthread_mutex_lock(&pQue->mutex);
    count = pQue->emptyCount;
    pthread_mutex_unlock(&pQue->mutex);
    
    return count;
}

bool CommQue_IsFull(CommQueID hQue) {
    if (!hQue) return true;
    
    CommQue_t* pQue = (CommQue_t*)hQue;
    bool isFull;
    
    pthread_mutex_lock(&pQue->mutex);
    isFull = (pQue->fullCount == pQue->maxCount);
    pthread_mutex_unlock(&pQue->mutex);
    
    return isFull;
}

bool CommQue_IsEmpty(CommQueID hQue) {
    if (!hQue) return true;
    
    CommQue_t* pQue = (CommQue_t*)hQue;
    bool isEmpty;
    
    pthread_mutex_lock(&pQue->mutex);
    isEmpty = (pQue->fullCount == 0);
    pthread_mutex_unlock(&pQue->mutex);
    
    return isEmpty;
}