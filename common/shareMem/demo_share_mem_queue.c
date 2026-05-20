/**
 * @file demo_share_mem_queue.c
 * @brief 共享内存队列使用示例 -- 跨进程生产者/消费者演示
 *
 * 两个进程使用同一个 QUEUE_ID，第一个调用 Create 的进程会创建队列，
 * 第二个调用 Create 的进程会自动打开已有的队列（EEXIST 路径）。
 *
 * 编译: gcc -o demo demo_share_mem_queue.c share_mem_queue.c ../log/log.c ../log/debugtrace.c -lpthread -lrt
 * 用法:
 *   进程A: ./demo cons   (消费者，先启动则创建队列)
 *   进程B: ./demo prod   (生产者，后启动则打开已有队列)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "share_mem_queue.h"

#define QUEUE_ID    "/tmp/smq_demo_queue"
#define FRAME_COUNT 16
#define FRAME_SIZE  256

static volatile int g_running = 1;

static void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
}

/* 消息结构 */
typedef struct {
    int  seq;
    char text[FRAME_SIZE - sizeof(int)];
} DemoMsg;

/* ========== 生产者 ========== */
static void producer_run(void) {
    HANDLE h = ShareMemQue_Create(FRAME_COUNT, FRAME_SIZE, QUEUE_ID);
    if (!h) {
        fprintf(stderr, "[Producer] Failed to open queue\n");
        return;
    }

    printf("[Producer] Queue opened, sending %d messages...\n", FRAME_COUNT);

    for (int i = 0; i < FRAME_COUNT && g_running; i++) {
        DemoMsg *msg = (DemoMsg *)ShareMemQue_GetWritePtr(h);
        if (!msg) {
            fprintf(stderr, "[Producer] GetWritePtr failed\n");
            break;
        }

        msg->seq = i + 1;
        snprintf(msg->text, sizeof(msg->text), "Hello from producer #%d", i + 1);

        ShareMemQue_PutWritePtr(h, msg);
        printf("[Producer] Sent msg #%d: \"%s\"\n", msg->seq, msg->text);

        usleep(200000); /* 200ms */
    }

    printf("[Producer] Done\n");
}

/* ========== 消费者 ========== */
static void consumer_run(void) {
    HANDLE h = ShareMemQue_Create(FRAME_COUNT, FRAME_SIZE, QUEUE_ID);
    if (!h) {
        fprintf(stderr, "[Consumer] Failed to create queue\n");
        return;
    }

    printf("[Consumer] Queue created, waiting for messages...\n");

    int received = 0;
    while (g_running) {
        DemoMsg *msg = (DemoMsg *)ShareMemQue_GetReadPtr(h);
        if (!msg) {
            fprintf(stderr, "[Consumer] GetReadPtr failed\n");
            break;
        }

        printf("[Consumer] Received #%d: \"%s\" (count=%d)\n",
               msg->seq, msg->text, ShareMemQue_GetCount(h));

        ShareMemQue_PutReadPtr(h, msg);
        received++;

        if (received >= FRAME_COUNT) {
            break;
        }
    }

    printf("[Consumer] Received %d messages, destroying queue\n", received);
    ShareMemQue_Delete(h);
}

/* ========== main ========== */
int main(int argc, char *argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <prod|cons>\n", argv[0]);
        fprintf(stderr, "  cons  - Consumer (creates the queue)\n");
        fprintf(stderr, "  prod  - Producer (opens existing queue)\n");
        return 1;
    }

    if (strcmp(argv[1], "prod") == 0) {
        producer_run();
    } else if (strcmp(argv[1], "cons") == 0) {
        consumer_run();
    } else {
        fprintf(stderr, "Unknown role: %s\n", argv[1]);
        return 1;
    }

    return 0;
}
