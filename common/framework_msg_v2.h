/**
 * @file framework_msg_v2.h
 * @brief Framework V2 消息结构定义 -- Topic通配符路由、Request-Response机制
 *
 * @details
 * V2 消息结构兼容 T_MsgV2，支持：
 *   - Topic 通配符匹配（* 匹配任意字段）
 *   - Request-Response 机制（strReply 回复地址）
 *   - 延迟响应（异步处理）
 *   - 字符串消息标识（strMsgId）
 *
 * @see defs.h（依赖 E_StateCode）
 * @see framework_v2.h（被依赖）
 */

#ifndef FRAMEWORK_MSG_V2_H
#define FRAMEWORK_MSG_V2_H

#include "defs.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief V2 消息结构体（兼容 T_MsgV2）
 */
typedef struct _T_FrameworkMsgV2 {
    uint32_t    sender;                    /**< 发送者模块ID */
    uint32_t    receiver;                  /**< 接收者模块ID (0=广播) */
    char        strMsgId[128];             /**< 消息标识（支持通配符） */
    char        strReply[256];             /**< 回复地址 */
    void       *pcBody;                    /**< 消息体数据 */
    uint32_t    uiBodySize;                /**< 数据长度 */
    uint32_t    uiCallId;                  /**< 调用ID */
    uint64_t    u64PrivateData;            /**< 私有数据 */
    char        strDescription[128];       /**< 描述 */
    int         copy_type;                 /**< 0=浅拷贝, 1=深拷贝 */
} T_FrameworkMsgV2;

/**
 * @brief 消息处理函数签名（V2 版本）
 *
 * @param pPrivate      模块私有数据
 * @param ptMsg         消息
 * @param pcResMsg      响应消息缓冲区
 * @param ppcResData    响应数据指针
 * @param puiDataSize   响应数据长度
 * @param pbDelayRes    延迟响应标志
 * @return E_StateCode  状态码
 */
typedef E_StateCode (*MsgProcFuncV2)(
    void *pPrivate,
    T_FrameworkMsgV2 *ptMsg,
    char *pcResMsg,
    char **ppcResData,
    uint32_t *puiDataSize,
    bool *pbDelayRes
);

/**
 * @brief 消息处理表项
 */
typedef struct {
    const char     *strMsgId;          /**< 消息ID（支持通配符*） */
    MsgProcFuncV2   pfProc;            /**< 处理函数 */
    void           *pPrivate;          /**< 私有数据 */
    bool            enabled;           /**< 是否启用 */
    bool            use_topic_match;   /**< 是否启用Topic匹配 */
} T_MsgProcEntryV2;

/**
 * @brief Topic 匹配工具函数
 *
 * @param pattern   模式字符串（支持 * 通配符）
 * @param topic     实际 Topic
 * @return true     匹配成功
 * @return false    匹配失败
 *
 * @details
 * 匹配规则：
 *   - 按 '.' 分割为多个字段
 *   - '*' 匹配任意单个字段
 *   - 字段数量必须相同
 *   - 其他字段必须精确匹配
 *
 * @example
 *   IsTopicEqualV2("$request.*.*.*.sample.*.on", "$request.set.0.1.2.sample.v1.on") => true
 *   IsTopicEqualV2("$request.get.*", "$request.set.0") => false
 */
bool IsTopicEqualV2(const char *pattern, const char *topic);

/**
 * @brief 从 Topic 中提取 ID（最后一段）
 *
 * @param topic   Topic 字符串
 * @return const char*  ID 字符串（指向 topic 内部）
 */
const char* GetTopicIdV2(const char *topic);

/**
 * @brief 从 Topic 中提取客户端 ID（第三段）
 *
 * @param topic   Topic 字符串
 * @return const char*  客户端 ID 字符串（指向 topic 内部）
 */
const char* GetClientIdV2(const char *topic);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEWORK_MSG_V2_H */
