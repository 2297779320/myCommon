#include "thread_safe_queue.h"

// 创建队列
Queue *queue_create(void) {
    Queue *queue = (Queue *)malloc(sizeof(Queue));
    if (!queue) {
        fprintf(stderr, "Failed to allocate memory for queue\n");
        return NULL;
    }
    
    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
    queue->is_destroyed = false;
    
    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        fprintf(stderr, "Failed to initialize mutex\n");
        free(queue);
        return NULL;
    }
    
    if (pthread_cond_init(&queue->cond, NULL) != 0) {
        fprintf(stderr, "Failed to initialize condition variable\n");
        pthread_mutex_destroy(&queue->mutex);
        free(queue);
        return NULL;
    }
    
    return queue;
}

// 销毁队列节点(释放节点和数据)
static void destroy_node(QueueNode *node) {
    if (node) {
        free(node->data);  // 释放数据
        free(node);        // 释放节点
    }
}

// 销毁队列(释放所有剩余数据)
void queue_destroy(Queue *queue) {
    if (!queue) return;

    pthread_mutex_lock(&queue->mutex);
    queue->is_destroyed = true;
    pthread_cond_broadcast(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);

    /* 等待所有 cond_wait 中的线程重新获取 mutex 并检查 is_destroyed 后退出 */
    pthread_mutex_lock(&queue->mutex);
    queue_clear(queue);  // 清空并释放所有数据
    pthread_mutex_unlock(&queue->mutex);

    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->cond);
    free(queue);
}

// 入队操作(数据由队列接管)
int queue_enqueue(Queue *queue, void *data, size_t data_size) {
    return queue_enqueue_timeout(queue, data, data_size, -1);
}

// 带超时的入队操作
int queue_enqueue_timeout(Queue *queue, void *data, size_t data_size, int timeout_ms) {
    if (!queue || !data || data_size == 0) {
        fprintf(stderr, "Invalid queue or data\n");
        return -1;
    }
    
    if (pthread_mutex_lock(&queue->mutex) != 0) {
        fprintf(stderr, "Failed to lock mutex\n");
        return -1;
    }
    
    if (queue->is_destroyed) {
        pthread_mutex_unlock(&queue->mutex);
        fprintf(stderr, "Queue has been destroyed\n");
        return -1;
    }
    
    QueueNode *node = (QueueNode *)malloc(sizeof(QueueNode));
    if (!node) {
        fprintf(stderr, "Failed to allocate node\n");
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    
    node->data = data;         // 接管数据所有权
    node->data_size = data_size;
    node->next = NULL;
    
    if (queue->rear) queue->rear->next = node;
    else queue->front = node;
    queue->rear = node;
    queue->size++;
    
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
    
    return 0;
}

// 出队操作(返回数据由用户接管并释放)
void *queue_dequeue(Queue *queue, size_t *data_size) {
    return queue_dequeue_timeout(queue, data_size, -1);
}

// 带超时的出队操作
void *queue_dequeue_timeout(Queue *queue, size_t *data_size, int timeout_ms) {
    if (!queue) {
        if (data_size) *data_size = 0;
        return NULL;
    }
    
    if (pthread_mutex_lock(&queue->mutex) != 0) {
        fprintf(stderr, "Failed to lock mutex\n");
        if (data_size) *data_size = 0;
        return NULL;
    }

    /* 超时时间点在循环外计算一次，避免虚假唤醒后叠加 */
    struct timespec abs_timeout;
    if (timeout_ms > 0) {
        if (clock_gettime(CLOCK_REALTIME, &abs_timeout) != 0) {
            fprintf(stderr, "Failed to get time\n");
            pthread_mutex_unlock(&queue->mutex);
            if (data_size) *data_size = 0;
            return NULL;
        }
        abs_timeout.tv_sec += timeout_ms / 1000;
        abs_timeout.tv_nsec += (timeout_ms % 1000) * 1000000;
        if (abs_timeout.tv_nsec >= 1000000000) {
            abs_timeout.tv_sec += 1;
            abs_timeout.tv_nsec -= 1000000000;
        }
    }

    int ret = 0;
    /* 使用 queue->size == 0 直接判断，避免调用 queue_is_empty() 导致重复加锁死锁 */
    while (queue->size == 0 && !queue->is_destroyed && timeout_ms != 0) {
        if (timeout_ms > 0) {
            ret = pthread_cond_timedwait(&queue->cond, &queue->mutex, &abs_timeout);
            if (ret == ETIMEDOUT) {
                pthread_mutex_unlock(&queue->mutex);
                if (data_size) *data_size = 0;
                return NULL;
            } else if (ret != 0) {
                fprintf(stderr, "Cond wait failed: %d\n", ret);
                pthread_mutex_unlock(&queue->mutex);
                if (data_size) *data_size = 0;
                return NULL;
            }
        } else {
            ret = pthread_cond_wait(&queue->cond, &queue->mutex);
            if (ret != 0) {
                fprintf(stderr, "Cond wait failed: %d\n", ret);
                pthread_mutex_unlock(&queue->mutex);
                if (data_size) *data_size = 0;
                return NULL;
            }
        }
    }
    
    if (queue->is_destroyed || queue->size == 0) {
        pthread_mutex_unlock(&queue->mutex);
        if (data_size) *data_size = 0;
        return NULL;
    }
    
    QueueNode *node = queue->front;
    void *data = node->data;
    if (data_size) *data_size = node->data_size;
    
    queue->front = node->next;
    if (!queue->front) queue->rear = NULL;
    queue->size--;
    free(node);
    
    pthread_mutex_unlock(&queue->mutex);
    return data;
}

// 按通配模式查找元素(返回副本)
void *queue_find_wildcard(Queue *queue, size_t *data_size, MatchFunc match_func, const void *pattern) {
    if (!queue || !match_func || !data_size) return NULL;
    
    *data_size = 0;
    void *found_data = NULL;
    pthread_mutex_lock(&queue->mutex);
    
    QueueNode *current = queue->front;
    while (current && !found_data) {
        if (match_func(current->data, current->data_size, pattern)) {
            found_data = malloc(current->data_size);
            if (found_data) {
                memcpy(found_data, current->data, current->data_size);
                *data_size = current->data_size;
            }
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&queue->mutex);
    return found_data;
}

// 按通配模式删除元素(删除的数据需用户释放)
size_t queue_remove_wildcard(Queue *queue, MatchFunc match_func, const void *pattern, 
                            void ***removed_elements, size_t **element_sizes) {
    if (!queue || !match_func || !removed_elements || !element_sizes) return 0;
    
    *removed_elements = NULL;
    *element_sizes = NULL;
    size_t count = 0;
    pthread_mutex_lock(&queue->mutex);
    
    // 统计匹配数量
    QueueNode *current = queue->front;
    while (current) {
        if (match_func(current->data, current->data_size, pattern)) count++;
        current = current->next;
    }
    
    if (count > 0) {
        *removed_elements = (void **)malloc(sizeof(void *) * count);
        *element_sizes = (size_t *)malloc(sizeof(size_t) * count);
        if (!*removed_elements || !*element_sizes) {
            fprintf(stderr, "Alloc failed for removed elements\n");
            free(*removed_elements);
            free(*element_sizes);
            *removed_elements = NULL;
            *element_sizes = NULL;
            pthread_mutex_unlock(&queue->mutex);
            return 0;
        }
        
        QueueNode **prev = &queue->front;
        current = queue->front;
        size_t index = 0;
        while (current) {
            if (match_func(current->data, current->data_size, pattern)) {
                (*removed_elements)[index] = current->data;  // 用户需释放这些数据
                (*element_sizes)[index] = current->data_size;
                index++;
                
                QueueNode *to_remove = current;
                *prev = current->next;
                if (current == queue->rear) {
                    /* prev 是 QueueNode**，需要取其值得到前一节点指针 */
                    queue->rear = (prev == &queue->front) ? NULL : (QueueNode *)((char *)prev - offsetof(QueueNode, next));
                }
                current = current->next;
                free(to_remove);  // 释放节点，数据由用户释放
                queue->size--;
            } else {
                prev = &current->next;
                current = current->next;
            }
        }
        pthread_cond_signal(&queue->cond);
    }
    
    pthread_mutex_unlock(&queue->mutex);
    return count;
}

// 释放删除元素的指针数组(不释放数据)
void queue_free_removed_elements(void ***elements, size_t **sizes, size_t count) {
    if (!elements || !sizes || !*elements || !*sizes || count == 0) return;
    free(*elements);
    free(*sizes);
    *elements = NULL;
    *sizes = NULL;
}

// 检查队列是否为空（外部调用版本，自行加锁）
bool queue_is_empty(Queue *queue) {
    if (!queue) return true;
    bool empty;
    pthread_mutex_lock(&queue->mutex);
    empty = (queue->size == 0);
    pthread_mutex_unlock(&queue->mutex);
    return empty;
}

// 获取队列大小（外部调用版本，自行加锁）
size_t queue_size(Queue *queue) {
    if (!queue) return 0;
    size_t size;
    pthread_mutex_lock(&queue->mutex);
    size = queue->size;
    pthread_mutex_unlock(&queue->mutex);
    return size;
}

// 清空队列并释放所有数据（调用方必须已持锁）
void queue_clear(Queue *queue) {
    if (!queue || queue->size == 0) return;
    
    QueueNode *current = queue->front;
    QueueNode *next;
    while (current) {
        next = current->next;
        destroy_node(current);
        current = next;
    }
    
    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
}