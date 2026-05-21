#include "request_que.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include "osal.h"

static AsyncRequest *create_request(void *data,
                                    void *context, RequestCallback callback,
                                    int64_t timeout_ms, request_id_t id,
                                    bool is_sync)
{
    AsyncRequest *req = (AsyncRequest *)malloc(sizeof(AsyncRequest));
    if (!req)
    {
        return NULL;
    }

    req->id = id;
    req->data = data;
    req->context = context;
    req->callback = callback;
    req->status = REQUEST_STATUS_PENDING;
    req->timeout_ms = timeout_ms;
    req->submit_time = OSAL_GetCpuStartInMsec();

    req->is_waiting = false;
    req->is_sync = is_sync;
    req->bNeedFree = false;

    if (pthread_mutex_init(&req->req_mutex, NULL) != 0)
    {
        free(req);
        return NULL;
    }

    if (pthread_cond_init(&req->req_cond, NULL) != 0)
    {
        pthread_mutex_destroy(&req->req_mutex);
        free(req);
        return NULL;
    }

    memset(&req->hh, 0, sizeof(UT_hash_handle));

    return req;
}
// 销毁请求（释放请求和结果数据）
static void destroy_request(AsyncRequest *req)
{
    if (!req)
        return;

    pthread_mutex_destroy(&req->req_mutex);
    pthread_cond_destroy(&req->req_cond);
    free(req);
}
// 创建异步请求队列
AsyncRequestQueue *async_request_queue_create(void)
{
    AsyncRequestQueue *queue = (AsyncRequestQueue *)malloc(sizeof(AsyncRequestQueue));
    if (!queue)
    {
        fprintf(stderr, "Failed to allocate memory for async request queue\n");
        return NULL;
    }

    // 初始化哈希表
    queue->request_table = NULL;

    queue->pending_queue = queue_create();
    if (!queue->pending_queue) {
        fprintf(stderr, "Failed to create pending queue\n");
        free(queue);
        return NULL;
    }


    if (pthread_mutex_init(&queue->id_mutex, NULL) != 0)
    {
        fprintf(stderr, "Failed to initialize id mutex\n");
        queue_destroy(queue->pending_queue);
        free(queue);
        return NULL;
    }

    if (pthread_mutex_init(&queue->table_mutex, NULL) != 0)
    {
        fprintf(stderr, "Failed to initialize table mutex\n");
        pthread_mutex_destroy(&queue->id_mutex);
        queue_destroy(queue->pending_queue);
        free(queue);
        return NULL;
    }

    queue->next_id = 1;
    queue->is_destroyed = false;

    return queue;
}
// 销毁异步请求队列
void async_request_queue_destroy(AsyncRequestQueue *queue)
{
    if (!queue)
        return;

    queue->is_destroyed = true;

    queue_clear(queue->pending_queue);
    queue_destroy(queue->pending_queue);

    // 销毁哈希表中的所有请求
    pthread_mutex_lock(&queue->table_mutex);
    AsyncRequest *req, *tmp;
    HASH_ITER(hh, queue->request_table, req, tmp)
    {
        HASH_DEL(queue->request_table, req);
        destroy_request(req);
    }
    
    pthread_mutex_unlock(&queue->table_mutex);
    pthread_mutex_destroy(&queue->id_mutex);
    pthread_mutex_destroy(&queue->table_mutex);
    free(queue);
}
// 提交异步请求
request_id_t async_request_submit(AsyncRequestQueue *queue,
                                  void *data,
                                  void *context, RequestCallback callback,
                                  int64_t timeout_ms)
{
    if (!queue)
    {
        fprintf(stderr, "Invalid parameters for async_request_submit\n");
        return 0;
    }

    if (queue->is_destroyed)
    {
        fprintf(stderr, "Queue has been destroyed\n");
        return 0;
    }

    // 生成请求ID
    pthread_mutex_lock(&queue->id_mutex);
    request_id_t req_id = queue->next_id++;
    if (queue->next_id == 0)
        queue->next_id = 1; // 防止溢出回绕
    pthread_mutex_unlock(&queue->id_mutex);

    // 创建请求（异步请求）
    AsyncRequest *req = create_request(data, context,
                                       callback, timeout_ms, req_id, false);
    if (!req)
    {
        fprintf(stderr, "Failed to create request\n");
        return 0;
    }

    // 插入哈希表
    pthread_mutex_lock(&queue->table_mutex);
    HASH_ADD(hh, queue->request_table, id, sizeof(request_id_t), req);
    pthread_mutex_unlock(&queue->table_mutex);

    // 入队
    if (queue_enqueue(queue->pending_queue, req, sizeof(AsyncRequest *)) != 0) {
        fprintf(stderr, "Failed to enqueue request\n");
        pthread_mutex_lock(&queue->table_mutex);
        HASH_DEL(queue->request_table, req);
        pthread_mutex_unlock(&queue->table_mutex);
        destroy_request(req);
        return 0;
    }

    return req_id;
}

// 获取待处理请求（供外部处理线程调用）
AsyncRequest *async_request_fetch(AsyncRequestQueue *queue)
{
    if (!queue)
        return NULL;

    if (queue->is_destroyed)
    {
        return NULL;
    }

    // 从待处理队列中取出请求
    size_t req_size;
    AsyncRequest *req = (AsyncRequest *)queue_dequeue(queue->pending_queue, &req_size);

    if (!req)
    {
        return NULL;
    }

    // 更新状态为处理中
    pthread_mutex_lock(&req->req_mutex);
    req->status = REQUEST_STATUS_PROCESSING;
    pthread_mutex_unlock(&req->req_mutex);

    return req;
}
// 标记请求完成（供外部处理线程调用）
void async_request_complete(AsyncRequestQueue *queue, AsyncRequest *req,
                            RequestStatus status)
{
    if (!queue || !req)
    {
        return;
    }

    if (queue->is_destroyed)
    {
        return;
    }

    // 更新请求状态和结果
    pthread_mutex_lock(&req->req_mutex);
    req->status = status;

    // 如果是同步请求且有线程在等待，唤醒等待线程
    if (req->is_waiting)
    {
        pthread_cond_signal(&req->req_cond);
    }
    pthread_mutex_unlock(&req->req_mutex);
}
// 阻塞等待请求完成
int async_request_wait(AsyncRequestQueue *queue, request_id_t req_id,
                       int64_t timeout_ms)
{
    if (!queue)
    {
        return -1;
    }

    if (queue->is_destroyed)
    {
        return -1;
    }

    // 从哈希表中查找请求
    pthread_mutex_lock(&queue->table_mutex);
    AsyncRequest *req = NULL;
    HASH_FIND(hh, queue->request_table, &req_id, sizeof(request_id_t), req);
    pthread_mutex_unlock(&queue->table_mutex);

    if (!req)
    {
        fprintf(stderr, "Request not found: %llu\n", (unsigned long long)req_id);
        return -1;
    }

    // 锁定请求互斥锁
    pthread_mutex_lock(&req->req_mutex);

    // 检查请求是否已经完成
    if (req->status == REQUEST_STATUS_COMPLETED ||
        req->status == REQUEST_STATUS_FAILED ||
        req->status == REQUEST_STATUS_CANCELLED ||
        req->status == REQUEST_STATUS_TIMEOUT)
    {
        req->bNeedFree = true;
        pthread_mutex_unlock(&req->req_mutex);
        return 0;
    }

    // 标记为等待状态
    req->is_waiting = true;

    // 等待请求完成
    int ret = 0;
    if (timeout_ms > 0)
    {
        // 带超时的等待
        struct timespec abs_timeout;
        clock_gettime(CLOCK_REALTIME, &abs_timeout);
        abs_timeout.tv_sec += timeout_ms / 1000;
        abs_timeout.tv_nsec += (timeout_ms % 1000) * 1000000;
        if (abs_timeout.tv_nsec >= 1000000000)
        {
            abs_timeout.tv_sec += 1;
            abs_timeout.tv_nsec -= 1000000000;
        }

        ret = pthread_cond_timedwait(&req->req_cond, &req->req_mutex, &abs_timeout);
        if (ret == ETIMEDOUT)
        {
            req->is_waiting = false;
            pthread_mutex_unlock(&req->req_mutex);
            return -1;
        }
        else if (ret != 0)
        {
            req->is_waiting = false;
            pthread_mutex_unlock(&req->req_mutex);
            return -1;
        }
    }
    else
    {
        // 无限等待
        ret = pthread_cond_wait(&req->req_cond, &req->req_mutex);
        if (ret != 0)
        {
            req->is_waiting = false;
            pthread_mutex_unlock(&req->req_mutex);
            return -1;
        }
    }

    req->is_waiting = false;
    req->bNeedFree = true;
    pthread_mutex_unlock(&req->req_mutex);
    return 0;
}
// 取消请求
bool async_request_cancel(AsyncRequestQueue *queue, request_id_t req_id)
{
    if (!queue)
        return false;

    if (queue->is_destroyed)
    {
        return false;
    }

    // 从哈希表中查找请求
    pthread_mutex_lock(&queue->table_mutex);
    AsyncRequest *req = NULL;
    HASH_FIND(hh, queue->request_table, &req_id, sizeof(request_id_t), req);
    pthread_mutex_unlock(&queue->table_mutex);

    if (!req)
    {
        return false;
    }

    // 检查请求状态
    pthread_mutex_lock(&req->req_mutex);
    if (req->status != REQUEST_STATUS_PENDING)
    {
        pthread_mutex_unlock(&req->req_mutex);
        return false; // 只有待处理的请求可以取消
    }

    req->status = REQUEST_STATUS_CANCELLED;

    // 如果是同步请求且有线程在等待，唤醒等待线程
    if (req->is_waiting)
    {
        pthread_cond_signal(&req->req_cond);
    }

    req->bNeedFree = true;
    pthread_mutex_unlock(&req->req_mutex);
    // 同步请求不放入已完成队列，由等待线程负责清理
    return true;
}
// 获取请求状态
RequestStatus async_request_get_status(AsyncRequestQueue *queue, request_id_t req_id)
{
    if (!queue)
        return REQUEST_STATUS_FAILED;

    // 从哈希表中查找请求
    pthread_mutex_lock(&queue->table_mutex);
    AsyncRequest *req = NULL;
    HASH_FIND(hh, queue->request_table, &req_id, sizeof(request_id_t), req);
    pthread_mutex_unlock(&queue->table_mutex);

    if (!req)
    {
        return REQUEST_STATUS_FAILED;
    }

    RequestStatus status;
    pthread_mutex_lock(&req->req_mutex);
    status = req->status;
    pthread_mutex_unlock(&req->req_mutex);

    return status;
}
// 获取队列统计信息
void async_request_queue_stats(AsyncRequestQueue *queue, QueueStats *stats)
{
    if (!queue || !stats)
    {
        if (stats)
        {
            memset(stats, 0, sizeof(QueueStats));
        }
        return;
    }

    stats->pending_count = queue_size(queue->pending_queue);

    pthread_mutex_lock(&queue->table_mutex);
    stats->total_count = HASH_COUNT(queue->request_table);
    stats->processing_count = stats->total_count > stats->pending_count
                              ? stats->total_count - stats->pending_count : 0;
    pthread_mutex_unlock(&queue->table_mutex);
}
// 清理已完成的请求
void async_request_cleanup_completed(AsyncRequestQueue *queue)
{
    if (!queue)
        return;
    int64_t current_time = OSAL_GetCpuStartInMsec();
    AsyncRequest *req, *tmp;

    /* 第一轮: 在 table_mutex 保护下收集需要处理的请求，
       释放 table_mutex 后再逐个处理，避免 ABBA 死锁 */
    pthread_mutex_lock(&queue->table_mutex);
    HASH_ITER(hh, queue->request_table, req, tmp)
    {
        if (req->bNeedFree)
        {
            HASH_DEL(queue->request_table, req);
            destroy_request(req);
            continue;
        }

        /* 仅检查是否超时，不嵌套锁 */
        if (req->status == REQUEST_STATUS_PENDING && req->timeout_ms > 0)
        {
            if (current_time - req->submit_time >= req->timeout_ms)
            {
                /* 标记超时，单独处理 */
                pthread_mutex_lock(&req->req_mutex);
                req->status = REQUEST_STATUS_TIMEOUT;
                if (req->is_waiting)
                {
                    pthread_cond_signal(&req->req_cond);
                }
                req->bNeedFree = true;
                pthread_mutex_unlock(&req->req_mutex);
            }
        }
    }
    pthread_mutex_unlock(&queue->table_mutex);
}

// 提交同步请求（阻塞等待完成）
request_id_t async_request_submit_sync(AsyncRequestQueue *queue,
                                       void *data,
                                       void *context,
                                       int64_t timeout_ms,
                                       RequestStatus *out_status)
{
    if (!queue || queue->is_destroyed)
        return 0;

    // 生成请求ID
    pthread_mutex_lock(&queue->id_mutex);
    request_id_t req_id = queue->next_id++;
    if (queue->next_id == 0)
        queue->next_id = 1;
    pthread_mutex_unlock(&queue->id_mutex);

    // 创建同步请求
    AsyncRequest *req = create_request(data, context,
                                       NULL, timeout_ms, req_id, true);
    if (!req)
        return 0;

    // 插入哈希表
    pthread_mutex_lock(&queue->table_mutex);
    HASH_ADD(hh, queue->request_table, id, sizeof(request_id_t), req);
    pthread_mutex_unlock(&queue->table_mutex);

    // 入队
    if (queue_enqueue(queue->pending_queue, req, sizeof(req)) != 0) {
        pthread_mutex_lock(&queue->table_mutex);
        HASH_DEL(queue->request_table, req);
        pthread_mutex_unlock(&queue->table_mutex);
        destroy_request(req);
        return 0;
    }

    // 阻塞等待完成
    int ret = async_request_wait(queue, req_id, timeout_ms);
    if (out_status) {
        *out_status = async_request_get_status(queue, req_id);
    }

    return (ret == 0) ? req_id : 0;
}