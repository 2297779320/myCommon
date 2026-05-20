/**
 * @file TlcMgeLoop.h
 * @brief 事件循环管理器接口 -- 为StbpClient提供事件循环支持
 *
 * @details
 * 提供事件循环的创建、启动、停止和销毁功能。
 * 基于 pthread 实现异步事件处理。
 */

#ifndef TLC_MGE_LOOP_H
#define TLC_MGE_LOOP_H

#include "typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建事件循环
 * @param phLoop 输出：事件循环句柄
 * @return 0成功，其他失败
 */
INT32 TlcMgeLoopCreate(tlHdl_t* phLoop);

/**
 * @brief 启动事件循环
 * @param hLoop 事件循环句柄
 * @param strName 循环名称
 * @return 0成功，其他失败
 */
INT32 TlcMgeLoopStart(tlHdl_t hLoop, const INT8* strName);

/**
 * @brief 停止事件循环
 * @param hLoop 事件循环句柄
 * @return 0成功，其他失败
 */
INT32 TlcMgeLoopStop(tlHdl_t hLoop);

/**
 * @brief 销毁事件循环
 * @param hLoop 事件循环句柄
 * @return 0成功，其他失败
 */
INT32 TlcMgeLoopDestroy(tlHdl_t hLoop);

#ifdef __cplusplus
}
#endif

#endif /* TLC_MGE_LOOP_H */
