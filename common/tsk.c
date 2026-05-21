#define _GNU_SOURCE
#define __USE_GNU
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sched.h>  
#include "tsk.h"

// 设置线程CPU亲和性（使用uint32_t掩码）
int set_thread_affinity_mask(pthread_t *thread, uint32_t mask) {
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    for (int i = 0; i < CPU_SETSIZE && i < 32; i++) {
        if (mask & (1U << i)) {
            CPU_SET(i, &cpuset);
        }
    }

    return pthread_setaffinity_np(*thread, sizeof(cpu_set_t), &cpuset);
#else
    (void)thread;
    (void)mask;
    return 0;
#endif
}

// 获取当前线程运行的CPU核心
int get_current_cpu() {
#ifdef __linux__
    return sched_getcpu();
#else
    return 0; // 返回默认CPU
#endif
}

// 创建任务
TSK_Handle TSK_create(TSK_Fxn fxn, const TSK_Attrs* attrs, void* arg) {
    if (fxn == NULL) {
        return NULL;
    }
    
    // 分配线程句柄内存
    pthread_t* thread = (pthread_t*)malloc(sizeof(pthread_t));
    if (thread == NULL) {
        return NULL;
    }
    
    // 设置线程属性
    pthread_attr_t threadAttr;
    pthread_attr_init(&threadAttr);
    
    // 设置调度策略
    if (attrs != NULL && attrs->bFifo) {
        struct sched_param schedParam;
        pthread_attr_setschedpolicy(&threadAttr, SCHED_FIFO);
        schedParam.sched_priority = attrs->priority;
        pthread_attr_setschedparam(&threadAttr, &schedParam);
        pthread_attr_setinheritsched(&threadAttr, PTHREAD_EXPLICIT_SCHED);
    }
    
    // 设置堆栈大小
    if (attrs != NULL && attrs->stackSize > 0) {
        pthread_attr_setstacksize(&threadAttr, attrs->stackSize);
    }
    
    // 创建线程
    int result = pthread_create(thread, &threadAttr, fxn, arg);
    pthread_attr_destroy(&threadAttr);
    
    if (result != 0) {
        free(thread);
        return NULL;
    }
    
    // 设置CPU亲和性
    if (attrs != NULL && attrs->cpuAffinityMask != 0xFFFFFFFFU) { // 如果不是所有CPU
        result = set_thread_affinity_mask(thread, attrs->cpuAffinityMask);
        if (result != 0) {
            printf("Warning: Failed to set CPU affinity: %s\n", strerror(result));
        }
    }

    // 设置线程名称（如果支持）
    if (attrs != NULL && attrs->name != NULL) {
#ifdef __APPLE__
        /* macOS pthread_setname_np 只能设置当前线程名，无法直接设置新线程。
           在新线程入口处自行调用 pthread_setname_np(attrs->name) 才是正确方式。
           此处仅在支持该扩展的平台上设置。 */
        (void)0; /* macOS 不支持从外部设置其他线程名称 */
#elif defined(__linux__)
        pthread_setname_np(*thread, attrs->name);
#endif
    }
    
    return thread;
}

// 删除任务
void TSK_delete(TSK_Handle hTsk) {
    if (hTsk == NULL) {
        return;
    }
    
    // // 取消线程
    // pthread_cancel(*hTsk);
    
    // 等待线程结束
    pthread_join(*hTsk, NULL);
    
    // 释放句柄内存
    free(hTsk);
}

// 任务休眠（毫秒）
void TSK_sleep(unsigned int milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}


