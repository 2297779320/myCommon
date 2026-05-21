/**
 * @file share_mem_queue.h
 * @brief 共享内存队列 -- 跨进程的生产者/消费者队列
 *
 * @details
 * 基于 System V 共享内存（shmget/shmat）实现的跨进程队列。
 * 通过字符串 ID 标识队列，支持多个进程同时读写。
 *
 * @see defs.h（依赖 HANDLE, E_StateCode, UINT32）
 */

#ifndef SHARE_MEM_QUEUE_H
#define SHARE_MEM_QUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include "defs.h"

EXTERN_C_BLOCK

/**
 * 创建或打开共享内存队列
 * 
 * @param attr 队列属性
 * @return 成功返回队列句柄，失败返回NULL
 */
HANDLE ShareMemQue_Create(UINT32 uiFrameCount, UINT32 uiFrameSize, const char* strId);

/**
 * 删除共享内存队列
 * 
 * @param handle 队列句柄
 * @return 成功返回SMQ_SUCCESS，失败返回错误码
 */
E_StateCode ShareMemQue_Delete(HANDLE handle);

/**
 * 获取写入指针
 * 
 * @param handle 队列句柄
 * @return 成功返回指向空闲元素的指针，失败返回NULL
 */
void* ShareMemQue_GetWritePtr(HANDLE handle);

/**
 * 提交写入指针(元素入队)
 * 
 * @param handle 队列句柄
 * @param pData 之前 GetWritePtr 返回的指针，用于验证
 * @return 成功返回SMQ_SUCCESS，失败返回错误码
 */
E_StateCode ShareMemQue_PutWritePtr(HANDLE handle, void* pData);

/**
 * 获取读取指针
 * 
 * @param handle 队列句柄
 * @return 成功返回指向队首元素的指针，失败返回NULL
 */
void* ShareMemQue_GetReadPtr(HANDLE handle);

/**
 * 提交读取指针(元素出队)
 * 
 * @param handle 队列句柄
 * @param pData 之前 GetReadPtr 返回的指针，用于验证
 * @return 成功返回SMQ_SUCCESS，失败返回错误码
 */
E_StateCode ShareMemQue_PutReadPtr(HANDLE handle, void* pData);

/**
 * 获取队列当前元素数量
 * 
 * @param handle 队列句柄
 * @return 成功返回元素数量，失败返回-1
 */
int ShareMemQue_GetCount(HANDLE handle);

EXTERN_C_BLOCK_END

#endif // SHARE_MEM_QUEUE_H