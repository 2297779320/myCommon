/**
 * @file thread_safe_queue.h
 * @brief 线程安全通用队列 -- 基于链表的FIFO队列
 *
 * @details
 * 提供线程安全的通用队列实现，支持超时入队/出队、通配查找和批量删除。
 * 内部使用 pthread 互斥锁 + 条件变量实现线程同步。
 *
 * @see request_que.h（被依赖）
 */

#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// 队列节点结构
typedef struct QueueNode {
    void *data;               // 存储元素数据指针
    size_t data_size;         // 数据大小
    struct QueueNode *next;   // 指向下一节点
} QueueNode;

// 队列结构
typedef struct {
    QueueNode *front;         // 队头指针
    QueueNode *rear;          // 队尾指针
    size_t size;              // 当前队列大小
    pthread_mutex_t mutex;    // 互斥锁，保证线程安全
    pthread_cond_t cond;      // 条件变量，用于线程间同步
    bool is_destroyed;        // 队列是否已销毁
} Queue;

// 通配匹配函数指针类型
typedef bool (*MatchFunc)(const void *element, size_t element_size, const void *pattern);

// 基础队列操作函数声明
Queue *queue_create(void);
void queue_destroy(Queue *queue);  // 销毁队列并释放所有剩余数据
int queue_enqueue(Queue *queue, void *data, size_t data_size);  // 入队后数据由队列管理
void *queue_dequeue(Queue *queue, size_t *data_size);  // 出队后数据由用户接管并释放
bool queue_is_empty(const Queue *queue);
size_t queue_size(Queue *queue);
void queue_clear(Queue *queue);  // 清空队列并释放所有数据

// 线程安全相关函数声明
int queue_enqueue_timeout(Queue *queue, void *data, size_t data_size, int timeout_ms);
void *queue_dequeue_timeout(Queue *queue, size_t *data_size, int timeout_ms);

// 元素通配相关函数声明
void *queue_find_wildcard(Queue *queue, size_t *data_size, MatchFunc match_func, const void *pattern);
size_t queue_remove_wildcard(Queue *queue, MatchFunc match_func, const void *pattern, 
                            void ***removed_elements, size_t **element_sizes);  // 删除的元素需用户释放
void queue_free_removed_elements(void ***elements, size_t **sizes, size_t count);  // 释放指针数组

#endif // THREAD_SAFE_QUEUE_H