/**
 * @file comm_que.h
 * @brief 通用消息队列 -- 生产者/消费者模式的线程安全队列
 *
 * @details
 * 提供基于空/满缓冲区的队列实现，适用于生产者-消费者场景。
 * 内部使用 CommQue 模型：GetEmpty 获取空缓冲 → 填充数据 → PutFull 提交；
 * GetFull 获取满缓冲 → 处理数据 → PutEmpty 归还。
 *
 * @see defs.h（依赖 EXTERN_C_BLOCK）
 * @see msgqueue.h, common.h（被依赖）
 */

#ifndef COMM_QUE_H
#define COMM_QUE_H
#include "defs.h"
#include <stdint.h>
#include <stdbool.h>
EXTERN_C_BLOCK

// 队列ID类型
typedef void* CommQueID;

// 超时时间定义
#define OSAL_TIMEOUT_NONE     0       // 不等待
#define OSAL_TIMEOUT_FOREVER  -1      // 永久等待

// 创建队列
// Count: 元素数量
// elementSize: 每个元素的大小
// pAllocFunc: 自定义内存分配函数, NULL则使用默认的malloc
CommQueID CommQue_Create(uint32_t maxCount, size_t elementSize,
                        void* (*pAllocFunc)(size_t size));

// 销毁队列
void CommQue_Delete(CommQueID hQue);

// 清空队列
void CommQue_Clear(CommQueID hQue);

// 获取一个满包
// timeout: 超时时间(毫秒), OSAL_TIMEOUT_NONE表示不等待, OSAL_TIMEOUT_FOREVER表示永久等待
// 返回值: 成功返回元素指针, 失败返回NULL
void* CommQue_GetFull(CommQueID hQue, int32_t timeout);

void *CommQue_PeekFull(CommQueID hQue);

// 获取一个空包
// timeout: 超时时间(毫秒), OSAL_TIMEOUT_NONE表示不等待, OSAL_TIMEOUT_FOREVER表示永久等待
// 返回值: 成功返回元素指针, 失败返回NULL
void* CommQue_GetEmpty(CommQueID hQue, int32_t timeout);

// 将包放回空队列
// pElement: 要放回的元素指针
// 返回值: 成功返回0, 失败返回-1
int32_t CommQue_PutEmpty(CommQueID hQue, void* pElement);

// 将包放入满队列
// pElement: 要放入的元素指针
// 返回值: 成功返回0, 失败返回-1
int32_t CommQue_PutFull(CommQueID hQue, void* pElement);

// 获取队列当前满包数量
uint32_t CommQue_GetFullCount(CommQueID hQue);

// 获取队列当前空包数量
uint32_t CommQue_GetEmptyCount(CommQueID hQue);

bool CommQue_IsFull(CommQueID hQue);

bool CommQue_IsEmpty(CommQueID hQue);

// 将包放入满队列头
// pElement: 要放入的元素指针
// 返回值: 成功返回0, 失败返回-1
int32_t CommQue_PutFullFront(CommQueID hQue, void* pElement);


// 将包放入空队列头
// pElement: 要放入的元素指针
// 返回值: 成功返回0, 失败返回-1
int32_t CommQue_PutEmptyFront(CommQueID hQue, void* pElement);

EXTERN_C_BLOCK_END
#endif // COMM_QUE_H
