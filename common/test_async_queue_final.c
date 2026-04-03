#include "async_request_queue_final.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
// 测试上下文
typedef struct {
    int thread_id;
    int completed_count;
    int failed_count;
    int cancelled_count;
} TestContext;

// 回调函数
void test_callback(request_id_t req_id, void *context, 
                   void *result, size_t result_size, 
                   RequestStatus status) {
    TestContext *ctx = (TestContext *)context;
}
// 工作线程函数
void *worker_thread(void *arg) {
    AsyncRequestQueue *queue = (AsyncRequestQueue *)arg;
    
    printf("[Worker] Thread started\n");
    
    while (1) 
    {
        // 获取待处理请求
        AsyncRequest *req = async_request_fetch(queue);
        if (!req) {
            usleep(10000);  // 10ms
            continue;
        }
        
        //req->data 同步请求  send read

        if(req->callback)
        {
            req->callback(req->id, req->context);
        }
        
        // 标记完成
        async_request_complete(queue, req,
                              REQUEST_STATUS_COMPLETED);

        async_request_cleanup_completed(queue);
    }
    
    return NULL;
}

int main(void) {
    printf("=== Async Request Queue with Blocking Wait Test ===\n\n");
    
    // 创建异步请求队列
    AsyncRequestQueue *queue = async_request_queue_create();
    if (!queue) {
        fprintf(stderr, "Failed to create async request queue\n");
        return 1;
    }
    
    // 创建工作线程
    pthread_t worker;
    if (pthread_create(&worker, NULL, worker_thread, queue) != 0) {
        fprintf(stderr, "Failed to create worker thread\n");
        async_request_queue_destroy(queue);
        return 1;
    }
    
    // 测试1: 异步请求
    printf("=== Test 1: Async Requests ===\n");
    TestContext contexts[3];
    request_id_t async_req_ids[3];
    for (int i = 0; i < 3; i++) {
        memset(&contexts[i], 0, sizeof(TestContext));
        contexts[i].thread_id = i;
        
        char *data = malloc(64);
        if (data) {
            snprintf(data, 64, "Async request data %d", i);
        }
        
        async_req_ids[i] = async_request_submit(queue, data, data ? strlen(data) + 1 : 0, 
                                                &contexts[i], test_callback, 
                                                -1);  // 无超时
        
        if (async_req_ids[i] == 0) {
            fprintf(stderr, "Failed to submit async request %d\n", i);
            free(data);
        } else {
            printf("[Main] Submitted async request: %llu\n", (unsigned long long)async_req_ids[i]);
        }
        
        usleep(50000);  // 50ms
    }
    
    // 取消第1个异步请求
    printf("\n[Main] Cancelling async request: %llu\n", (unsigned long long)async_req_ids[1]);
    if (async_request_cancel(queue, async_req_ids[1])) {
        printf("[Main] Async request %llu cancelled successfully\n", (unsigned long long)async_req_ids[1]);
    } else {
        printf("[Main] Failed to cancel async request %llu\n", (unsigned long long)async_req_ids[1]);
    }
    
    printf("\n[Main] All async requests submitted\n\n");
    
    // 等待异步请求完成
    sleep(2);
    
    // 测试2: 同步请求（阻塞等待）
    printf("\n=== Test 2: Sync Requests (Blocking Wait) ===\n");
    for (int i = 0; i < 3; i++) {
        char *data = malloc(64);
        if (data) {
            snprintf(data, 64, "Sync request data %d", i);
        }
        
        void *result = NULL;
        size_t result_size = 0;
        RequestStatus status = REQUEST_STATUS_PENDING;
        
        printf("[Main] Submitting sync request %d and waiting...\n", i);
        request_id_t req_id = async_request_submit_sync(queue, data, data ? strlen(data) + 1 : 0,
                                                         NULL,  // 无上下文
                                                         -1,    // 无超时
                                                         &result, &result_size, &status);
        
        if (req_id == 0) {
            fprintf(stderr, "Failed to submit sync request %d\n", i);
            free(data);
        } else {
            const char *status_str = "UNKNOWN";
            switch (status) {
                case REQUEST_STATUS_PENDING: status_str = "PENDING"; break;
                case REQUEST_STATUS_PROCESSING: status_str = "PROCESSING"; break;
                case REQUEST_STATUS_COMPLETED: status_str = "COMPLETED"; break;
                case REQUEST_STATUS_FAILED: status_str = "FAILED"; break;
                case REQUEST_STATUS_CANCELLED: status_str = "CANCELLED"; break;
                case REQUEST_STATUS_TIMEOUT: status_str = "TIMEOUT"; break;
            }
            
            printf("[Main] Sync request %llu completed with status: %s\n", 
                   (unsigned long long)req_id, status_str);
            
            if (result && result_size > 0) {
                printf("  Result: %.*s\n", (int)result_size, (char *)result);
            }
            
            // 注意：result由队列管理，调用者不应释放
        }
        
        usleep(100000);  // 100ms
    }
    
    // 测试3: 同步请求超时
    printf("\n=== Test 3: Sync Request Timeout ===\n");
    char *timeout_data = malloc(64);
    if (timeout_data) {
        snprintf(timeout_data, 64, "Timeout test request");
    }
    
    void *timeout_result = NULL;
    size_t timeout_result_size = 0;
    RequestStatus timeout_status = REQUEST_STATUS_PENDING;
    
    printf("[Main] Submitting sync request with 100ms timeout...\n");
    request_id_t timeout_req_id = async_request_submit_sync(queue, timeout_data, 
                                                             timeout_data ? strlen(timeout_data) + 1 : 0,
                                                             NULL,  // 无上下文
                                                             100,   // 100ms超时
                                                             &timeout_result, 
                                                             &timeout_result_size, 
                                                             &timeout_status);
    
    if (timeout_req_id == 0) {
        fprintf(stderr, "Failed to submit timeout test request\n");
        free(timeout_data);
    } else {
        const char *status_str = "UNKNOWN";
        switch (timeout_status) {
            case REQUEST_STATUS_PENDING: status_str = "PENDING"; break;
            case REQUEST_STATUS_PROCESSING: status_str = "PROCESSING"; break;
            case REQUEST_STATUS_COMPLETED: status_str = "COMPLETED"; break;
            case REQUEST_STATUS_FAILED: status_str = "FAILED"; break;
            case REQUEST_STATUS_CANCELLED: status_str = "CANCELLED"; break;
            case REQUEST_STATUS_TIMEOUT: status_str = "TIMEOUT"; break;
        }
        
        printf("[Main] Sync request %llu completed with status: %s\n", 
               (unsigned long long)timeout_req_id, status_str);
    }
    
    // 获取统计信息
    QueueStats stats;
    async_request_queue_stats(queue, &stats);
    printf("\n[Main] Queue Statistics:\n");
    printf("  Pending: %zu\n", stats.pending_count);
    printf("  Processing: %zu\n", stats.processing_count);
    printf("  Completed: %zu\n", stats.completed_count);
    printf("  Total: %zu\n", stats.total_count);
    
    // 打印回调统计
    printf("\n[Main] Async Callback Statistics:\n");
    for (int i = 0; i < 3; i++) {
        printf("  Request %d: Total=%d, Completed=%d, Failed=%d, Cancelled=%d\n",
               i, contexts[i].request_count, contexts[i].completed_count,
               contexts[i].failed_count, contexts[i].cancelled_count);
    }
    
    // 清理已完成的请求
    async_request_cleanup_completed(queue);
    
    printf("\n[Main] Cleanup completed\n");
    
    // 再次获取统计信息
    async_request_queue_stats(queue, &stats);
    printf("\n[Main] Queue Statistics after cleanup:\n");
    printf("  Pending: %zu\n", stats.pending_count);
    printf("  Processing: %zu\n", stats.processing_count);
    printf("  Completed: %zu\n", stats.completed_count);
    printf("  Total: %zu\n", stats.total_count);
    
    // 销毁队列
    printf("\n[Main] Destroying queue...\n");
    async_request_queue_destroy(queue);
    
    printf("\n[Main] Test completed successfully!\n");
    
    return 0;
}