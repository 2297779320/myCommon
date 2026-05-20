/**
 * @file TlcMgeStbpc.h
 * @brief STBP客户端底层通信函数接口
 *
 * @details
 * 提供 STBP 客户端的底层网络通信功能：
 *   - 连接管理（Create/Connect/Disconnect/Destroy）
 *   - 消息收发（Request/Publish/Subscribe）
 *   - 异步操作（RequestAsync）
 *   - 消息管理（FreeMsg）
 */

#ifndef TLC_MGE_STBPC_H
#define TLC_MGE_STBPC_H

#include "typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建STBP客户端连接
 * @param hLoop   事件循环句柄
 * @param pCfg    配置参数
 * @param phConn  输出：连接句柄
 * @return 0成功，其他失败
 */
INT32 TlcMgeStbpcCreate(void* hLoop, void* pCfg, void** phConn);

/**
 * @brief 连接到STBP服务器
 * @param hConn   连接句柄
 * @param iFlags  连接标志
 * @return 0成功，其他失败
 */
INT32 TlcMgeStbpcConnect(void* hConn, INT32 iFlags);

/**
 * @brief 断开连接
 * @param hConn       连接句柄
 * @param iTimeout    超时时间（毫秒）
 * @return 0成功，其他失败
 */
INT32 TlcMgeStbpcDisconnect(void* hConn, INT32 iTimeout);

/**
 * @brief 销毁连接
 * @param hConn   连接句柄
 * @return 0成功，其他失败
 */
INT32 TlcMgeStbpcDestroy(void* hConn);

/**
 * @brief 同步请求（等待响应）
 * @param hConn       连接句柄
 * @param strTopic    Topic
 * @param pPayload    载荷数据
 * @param uiLen       载荷长度
 * @param pResponse   输出：响应数据
 * @param pRespLen    输出：响应长度
 * @param iTimeout    超时时间（毫秒）
 * @return 0成功，其他失败
 */
INT32 TlcMgeStbpcRequest(void* hConn, const INT8* strTopic,
                          const void* pPayload, UINT32 uiLen,
                          void** pResponse, UINT32* pRespLen,
                          INT32 iTimeout);

/**
 * @brief 异步请求
 * @param hConn       连接句柄
 * @param strTopic    Topic
 * @param pPayload    载荷数据
 * @param uiLen       载荷长度
 * @param pfCb        回调函数
 * @param pCbCtx      回调上下文
 * @param iTimeout    超时时间（毫秒）
 * @return 0成功，其他失败
 */
INT32 TlcMgeStbpcRequestAsync(void* hConn, const INT8* strTopic,
                               const void* pPayload, UINT32 uiLen,
                               void* pfCb, void* pCbCtx,
                               INT32 iTimeout);

/**
 * @brief 发布消息（无响应）
 * @param hConn       连接句柄
 * @param strTopic    Topic
 * @param pPayload    载荷数据
 * @param uiLen       载荷长度
 * @param iFlags      标志
 * @return 0成功，其他失败
 */
INT32 TlcMgeStbpcPublish(void* hConn, const INT8* strTopic,
                          const void* pPayload, UINT32 uiLen,
                          INT32 iFlags);

/**
 * @brief 订阅Topic
 * @param hConn       连接句柄
 * @param strTopic    Topic（支持通配符）
 * @param iFlags      标志
 * @return 0成功，其他失败
 */
INT32 TlcMgeStbpcSubscribe(void* hConn, const INT8* strTopic, INT32 iFlags);

/**
 * @brief 取消订阅
 * @param hConn       连接句柄
 * @param strTopic    Topic
 * @param iFlags      标志
 * @return 0成功，其他失败
 */
INT32 TlcMgeStbpcUnsubscribe(void* hConn, const INT8* strTopic, INT32 iFlags);

/**
 * @brief 释放消息
 * @param pMsg    消息指针
 * @return 0成功，其他失败
 */
INT32 TlcMgeStbpcFreeMsg(void* pMsg);

/**
 * @brief 获取发送和接收长度
 * @param hConn       连接句柄
 * @param pSendLen    输出：发送长度
 * @param pRecvLen    输出：接收长度
 * @return 0成功，其他失败
 */
INT32 TlcMgeStbpcReturnSendLenAndRecvLenInside(void* hConn,
                                                UINT32* pSendLen,
                                                UINT32* pRecvLen);

#ifdef __cplusplus
}
#endif

#endif /* TLC_MGE_STBPC_H */
