/**
 * @file framework_v2.h
 * @brief Framework V2 API -- Topic通配符路由、表驱动分发、Request-Response机制
 *
 * @details
 * Framework V2 在 V1 的基础上增强消息路由能力：
 *   - Topic 通配符匹配（* 匹配任意字段）
 *   - 表驱动的消息分发（T_MsgProcV2 模式）
 *   - Request-Response 机制（strReply 回复地址）
 *   - 延迟响应支持（异步处理）
 *   - 字符串消息标识（strMsgId）
 *
 * @see framework_def.h（V1框架，并存）
 * @see framework_msg_v2.h（V2消息定义）
 * @see module_manager.h（V1模块管理）
 */

#ifndef FRAMEWORK_V2_H
#define FRAMEWORK_V2_H

#include "framework_def.h"
#include "framework_msg_v2.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief V2 框架句柄 */
typedef struct framework_context_v2* FrameworkHandleV2;

/** @brief V2 模块句柄 */
typedef struct module_v2* ModuleHandleV2;

/**
 * @brief V2 模块结构（公开部分字段供宏访问）
 * @note 完整定义在 framework_v2.c 中
 */
struct module_v2 {
    uint32_t id;
    void *private_data;
    struct framework_context_v2 *framework;
    /* 其余字段为内部实现，不公开 */
};

/**
 * @brief V2 框架上下文结构（公开部分字段供宏访问）
 * @note 完整定义在 framework_v2.c 中
 */
struct framework_context_v2 {
    struct module_v2 *modules;
    uint32_t module_count;
    void *msg_queue;
    volatile bool is_running;
    uint32_t next_module_id;
};

/**
 * @brief 创建 V2 框架
 * @param max_msg_count 消息队列最大容量
 * @return 框架句柄，失败返回 NULL
 */
FrameworkHandleV2 framework_v2_create(uint32_t max_msg_count);

/**
 * @brief 销毁 V2 框架
 * @param handle 框架句柄
 */
void framework_v2_destroy(FrameworkHandleV2 handle);

/**
 * @brief 注册 V2 模块
 * @param handle          框架句柄
 * @param init            初始化函数
 * @param run             运行函数
 * @param destroy         销毁函数
 * @param msg_table       V2 消息处理表
 * @param msg_table_len   消息表长度
 * @param config          配置参数（传递给 init）
 * @return 模块ID，失败返回 0
 */
uint32_t framework_v2_register_module(
    FrameworkHandleV2 handle,
    bool (*init)(ModuleHandleV2, void*),
    void (*run)(ModuleHandleV2),
    void (*destroy)(ModuleHandleV2),
    const T_MsgProcEntryV2 *msg_table,
    uint32_t msg_table_len,
    void *config
);

/**
 * @brief 获取模块数量
 * @param handle 框架句柄
 * @return 模块数量
 */
uint32_t framework_v2_get_module_count(FrameworkHandleV2 handle);

/**
 * @brief V2 消息发送
 * @param handle      框架句柄
 * @param sender      发送者模块ID
 * @param receiver    接收者模块ID（0=广播）
 * @param strMsgId    消息标识
 * @param strReply    回复地址（可为NULL）
 * @param call_id     调用ID
 * @param data_len    数据长度
 * @param data        数据指针
 * @param copy_type   拷贝类型（0=浅拷贝, 1=深拷贝）
 * @param timeout     超时时间（毫秒）
 * @return E_StateCode 状态码
 */
E_StateCode framework_v2_send_message(
    FrameworkHandleV2 handle,
    uint32_t sender,
    uint32_t receiver,
    const char *strMsgId,
    const char *strReply,
    uint32_t call_id,
    uint32_t data_len,
    const void *data,
    int copy_type,
    int32_t timeout
);

/**
 * @brief V2 响应消息发送
 * @param handle       框架句柄
 * @param request_msg  请求消息
 * @param strReply     回复地址
 * @param data_len     数据长度
 * @param data         数据指针
 * @return E_StateCode 状态码
 */
E_StateCode framework_v2_send_response(
    FrameworkHandleV2 handle,
    T_FrameworkMsgV2 *request_msg,
    const char *strReply,
    uint32_t data_len,
    const void *data
);

/**
 * @brief 模块便捷消息发送
 * @param module     模块句柄
 * @param receiver   接收者模块ID
 * @param strMsgId   消息标识
 * @param strReply   回复地址
 * @param call_id    调用ID
 * @param data_len   数据长度
 * @param data       数据指针
 * @param copy_type  拷贝类型
 * @param timeout    超时时间
 * @return E_StateCode 状态码
 */
E_StateCode module_v2_send_message(
    ModuleHandleV2 module,
    uint32_t receiver,
    const char *strMsgId,
    const char *strReply,
    uint32_t call_id,
    uint32_t data_len,
    const void *data,
    int copy_type,
    int32_t timeout
);

/**
 * @brief 处理 V2 消息队列（含Topic匹配）
 * @param handle 框架句柄
 */
void framework_v2_process_messages(FrameworkHandleV2 handle);

/**
 * @brief 模块动态添加消息处理器
 * @param module          模块句柄
 * @param strMsgId        消息标识
 * @param handler         处理函数
 * @param use_topic_match 是否启用Topic匹配
 * @return true 成功，false 失败
 */
bool module_v2_add_handler(
    ModuleHandleV2 module,
    const char *strMsgId,
    MsgProcFuncV2 handler,
    bool use_topic_match
);

/**
 * @brief 初始化所有 V2 模块
 * @param handle 框架句柄
 * @return true 成功，false 失败
 */
bool framework_v2_init_all_modules(FrameworkHandleV2 handle);

/**
 * @brief 启动 V2 主循环
 * @param handle      框架句柄
 * @param interval_ms 循环间隔（毫秒）
 * @return true 成功，false 失败
 */
bool framework_v2_start_main_loop(FrameworkHandleV2 handle, uint32_t interval_ms);

/**
 * @brief 停止 V2 主循环
 * @param handle 框架句柄
 */
void framework_v2_stop_main_loop(FrameworkHandleV2 handle);

/***********************************************************
*                    模块操作宏                            *
**********************************************************/

/**
 * @brief 获取模块私有数据
 * @param module 模块句柄
 * @return 私有数据指针
 */
#define MODULE_GET_PRIVATE(module) \
    ((module) ? ((struct module_v2*)(module))->private_data : NULL)

/**
 * @brief 设置模块私有数据
 * @param module 模块句柄
 * @param data   私有数据指针
 */
#define MODULE_SET_PRIVATE(module, data) \
    do { if (module) ((struct module_v2*)(module))->private_data = (data); } while(0)

/**
 * @brief 获取模块ID
 * @param module 模块句柄
 * @return 模块ID
 */
#define MODULE_GET_ID(module) \
    ((module) ? ((struct module_v2*)(module))->id : 0)

/**
 * @brief 获取框架句柄
 * @param module 模块句柄
 * @return 框架句柄
 */
#define MODULE_GET_FRAMEWORK(module) \
    ((module) ? ((struct module_v2*)(module))->framework : NULL)

/**
 * @brief 检查框架是否运行中
 * @param fw 框架句柄
 * @return true/false
 */
#define FRAMEWORK_IS_RUNNING(fw) \
    ((fw) ? ((struct framework_context_v2*)(fw))->is_running : false)

/**
 * @brief 获取模块数量
 * @param fw 框架句柄
 * @return 模块数量
 */
#define FRAMEWORK_GET_MODULE_COUNT(fw) \
    ((fw) ? ((struct framework_context_v2*)(fw))->module_count : 0)

#ifdef __cplusplus
}
#endif

#endif /* FRAMEWORK_V2_H */
