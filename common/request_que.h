#ifndef REQUEST_QUEUE_H
#define REQUEST_QUEUE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>
#include "thread_safe_queue.h"
#include "uthash.h"
#ifdef __cplusplus
extern "C" {
#endif
// 请求状态枚举
typedef enum {
    REQUEST_STATUS_PENDING = 0,    // 等待处理
    REQUEST_STATUS_PROCESSING,     // 处理中
    REQUEST_STATUS_COMPLETED,      // 已完成
    REQUEST_STATUS_FAILED,         // 失败
    REQUEST_STATUS_CANCELLED,      // 已取消
    REQUEST_STATUS_TIMEOUT         // 超时
} RequestStatus;
// 请求ID类型
typedef uint64_t request_id_t;
// 回调函数类型
typedef void (*RequestCallback)(request_id_t req_id, void *context);
// 异步请求结构体
typedef struct {
    request_id_t id;               // 请求唯一ID（作为uthash的key）
    void *data;                    // 请求数据（由用户管理）
    void *context;                 // 用户上下文（由用户管理，队列不释放）
    RequestCallback callback;      // 完成回调函数
    RequestStatus status;          // 当前状态
    int64_t timeout_ms;            // 超时时间（毫秒，-1表示永不超时）
    int64_t submit_time;           // 提交时间（毫秒时间戳）
    pthread_mutex_t req_mutex;     // 请求互斥锁（保护状态变更）
    pthread_cond_t req_cond;       // 请求条件变量（用于阻塞等待）
    bool is_waiting;               // 是否有线程在等待此请求
    bool is_sync;                  // 是否为同步请求（需要阻塞等待）

    bool bNeedFree;                // 是否需要释放请求数据
    
    UT_hash_handle hh;             // uthash句柄，用于哈希表管理
} AsyncRequest;
// 异步请求队列结构体
typedef struct {
    Queue *pending_queue;         // 待处理队列
    AsyncRequest *request_table;   // 请求哈希表（由uthash管理）
    pthread_mutex_t id_mutex;      // ID生成互斥锁
    request_id_t next_id;          // 下一个请求ID
    bool is_destroyed;             // 是否已销毁
    pthread_mutex_t table_mutex;   // 哈希表访问互斥锁
} AsyncRequestQueue;
// 创建异步请求队列
AsyncRequestQueue *async_request_queue_create(void);
// 销毁异步请求队列
void async_request_queue_destroy(AsyncRequestQueue *queue);
// 提交异步请求
// data: 请求数据（队列会接管并负责释放）
// data_size: 数据大小
// context: 用户上下文（队列不负责释放）
// callback: 完成回调函数（可以为NULL）
// timeout_ms: 超时时间（毫秒，-1表示永不超时）
// 返回: 请求ID，失败返回0
request_id_t async_request_submit(AsyncRequestQueue *queue, 
                                  void *data,
                                  void *context, RequestCallback callback,
                                  int64_t timeout_ms);
// 提交同步请求（阻塞等待完成）
// data: 请求数据（队列会接管并负责释放）
// data_size: 数据大小
// context: 用户上下文（队列不负责释放）
// timeout_ms: 超时时间（毫秒，-1表示永不超时）
// status: 输出参数，返回请求状态
// 返回: 请求ID，失败返回0
request_id_t async_request_submit_sync(AsyncRequestQueue *queue, 
                                       void *data,
                                       void *context,
                                       int64_t timeout_ms,
                                       RequestStatus *status);
// 获取待处理请求（供外部处理线程调用）
// 返回: 请求指针，失败返回NULL
// 注意: 返回的请求由队列管理，调用者不应释放
AsyncRequest *async_request_fetch(AsyncRequestQueue *queue);
// 标记请求完成（供外部处理线程调用）
// req_id: 请求ID
// status: 完成状态
void async_request_complete(AsyncRequestQueue *queue, AsyncRequest *req,
                            RequestStatus status);
// 阻塞等待请求完成
// req_id: 请求ID
// timeout_ms: 等待超时时间（毫秒，-1表示永不超时）
// status: 输出参数，返回请求状态
// 返回: 0表示成功，-1表示失败或超时
int async_request_wait(AsyncRequestQueue *queue, request_id_t req_id,
                       int64_t timeout_ms);
// 取消请求
// 返回: true表示取消成功，false表示失败
bool async_request_cancel(AsyncRequestQueue *queue, request_id_t req_id);
// 获取请求状态
RequestStatus async_request_get_status(AsyncRequestQueue *queue, request_id_t req_id);
// 获取队列统计信息
typedef struct {
    size_t pending_count;      // 待处理数量
    size_t processing_count;   // 处理中数量
    size_t completed_count;    // 已完成数量
    size_t total_count;        // 总请求数量
} QueueStats;
void async_request_queue_stats(AsyncRequestQueue *queue, QueueStats *stats);
// 清理已完成的请求（内部调用，定期清理资源）
void async_request_cleanup_completed(AsyncRequestQueue *queue);
#ifdef __cplusplus
}
#endif
#endif // REQUEST_QUEUE_H
