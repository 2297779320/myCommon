/**
 * @file msgqueue.h
 * @brief 消息队列封装 -- 基于 CommQue 的 T_Msg 消息队列
 *
 * @details
 * 在 CommQue 之上封装的高级消息队列，专门用于 T_Msg 消息的收发。
 * 提供创建、销毁、发送、接收、归还消息等操作。
 *
 * @see comm_que.h（依赖 CommQueID）
 * @see defs.h（依赖 E_StateCode）
 * @see media.h（依赖 T_Msg）
 */

#ifndef MSGQUEUE_H
#define MSGQUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "comm_que.h" 
#include "defs.h"    
#include "media.h"  
// 消息队列句柄
typedef struct _MsgQueueHandle
{
    CommQueID que_id; // 唯一的一个 CommQue 队列ID
} MsgQueueHandle;

/**
 * @brief  创建并初始化一个消息队列。
 * @param  max_msg_count: 消息队列能容纳的最大消息数（即消息池大小）。
 * @return 成功返回消息队列句柄，失败返回 NULL。
 */
MsgQueueHandle* msg_queue_create(uint32_t max_msg_count);

/**
 * @brief  销毁一个消息队列。
 * @param  handle: 消息队列句柄。
 */
void msg_queue_destroy(MsgQueueHandle* handle);

/**
 * @brief  发送一个消息到队列。
 * @param  handle: 消息队列句柄。
 * @param  pcMsg: 消息名称/类型。
 * @param  pcBody: 消息体。
 * @param  uiCallId: 调用ID。
 * @param  timeout: 等待获取空消息的超时时间 (毫秒)。
 * @return 成功返回 0，失败返回 -1。
 */
E_StateCode msg_queue_send(MsgQueueHandle* handle, T_Msg* msg, int32_t timeout);

/**
 * @brief  从队列接收一个消息。
 * @param  handle: 消息队列句柄。
 * @param  timeout: 等待消息的超时时间 (毫秒)。
 * @return 成功返回指向 T_Msg 的指针，失败或超时返回 NULL。
 * @note   调用者在处理完消息后，必须调用 msg_queue_release_msg 将消息归还。
 */
T_Msg* msg_queue_receive(MsgQueueHandle* handle, int32_t timeout);

/**
 * @brief  将处理完毕的消息归还给消息队列。
 * @param  handle: 消息队列句柄。
 * @param  msg: 要归还的消息指针。
 * @return 成功返回 0，失败返回 -1。
 */
E_StateCode msg_queue_release_msg(MsgQueueHandle* handle, T_Msg* msg);

/**
 * @brief  获取当前队列中等待处理的消息数量。
 * @param  handle: 消息队列句柄。
 * @return 消息数量。
 */
uint32_t msg_queue_get_pending_count(MsgQueueHandle* handle);

#endif // MSGQUEUE_H