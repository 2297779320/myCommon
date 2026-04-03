#ifndef FRAMEWORK_DEF_H
#define FRAMEWORK_DEF_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"

// 前向声明 - 隐藏内部实现细节
typedef struct framework_context* FrameworkHandle;
typedef struct module* ModuleHandle;

// 模块ID类型定义
typedef uint32_t module_id_t;

// 消息类型定义
typedef uint32_t msg_type_t;

// 深拷贝标志枚举
typedef enum {
    MSG_COPY_SHALLOW = 0,    // 浅拷贝：仅复制指针，数据共享
    MSG_COPY_DEEP = 1        // 深拷贝：复制整个数据，接收方拥有所有权
} MsgCopyType;

// 消息结构体定义
typedef struct _T_ModuleMsg
{
    module_id_t sender;       // 发送者模块ID
    module_id_t receiver;     // 接收者模块ID (0表示广播)
    msg_type_t type;          // 消息类型
    uint32_t timestamp;       // 时间戳
    uint32_t call_id;         // 调用ID
    size_t data_len;          // 数据长度
    void* data;               // 消息数据指针
    MsgCopyType copy_type;    // 拷贝类型标志
} T_ModuleMsg;

/**
 * 消息处理函数原型
 */
typedef void (*MessageHandler)(ModuleHandle module, const T_ModuleMsg* msg);

/**
 * 模块生命周期函数原型
 */
typedef bool (*ModuleInitFunc)(ModuleHandle module, void* config);
typedef void (*ModuleRunFunc)(ModuleHandle module);
typedef void (*ModuleDestroyFunc)(ModuleHandle module);

/**
 * 框架创建与销毁
 */
FrameworkHandle framework_create(uint32_t max_msg_count);
void framework_destroy(FrameworkHandle handle);

/**
 * 模块注册与管理
 */
module_id_t framework_register_module(FrameworkHandle handle, 
                                     ModuleInitFunc init,
                                     ModuleRunFunc run,
                                     ModuleDestroyFunc destroy,
                                     void* config);
                                     
uint32_t framework_get_module_count(FrameworkHandle handle);
bool framework_module_exists(FrameworkHandle handle, module_id_t id);

/**
 * 框架运行控制
 */
bool framework_init_all_modules(FrameworkHandle handle);
bool framework_start_main_loop(FrameworkHandle handle, uint32_t interval_ms);
void framework_stop_main_loop(FrameworkHandle handle);

/**
 * 消息发送接口
 */
E_StateCode framework_send_message(FrameworkHandle handle, 
                                  module_id_t sender, 
                                  module_id_t receiver, 
                                  msg_type_t type, 
                                  uint32_t call_id, 
                                  size_t data_len, 
                                  const void* data, 
                                  MsgCopyType copy_type,
                                  int32_t timeout);
                                  
E_StateCode module_send_message(ModuleHandle module, 
                               module_id_t receiver, 
                               msg_type_t type, 
                               uint32_t call_id, 
                               size_t data_len, 
                               const void* data, 
                               MsgCopyType copy_type,
                               int32_t timeout);

/**
 * 消息处理器管理接口
 */
bool module_register_handler(ModuleHandle module, msg_type_t msg_type, MessageHandler handler);
E_StateCode module_enable_handler(ModuleHandle module, msg_type_t msg_type, bool enable);
bool module_is_handler_enabled(ModuleHandle module, msg_type_t msg_type);

/**
 * 消息数据释放接口（用于深拷贝消息）
 */
void msg_free_data(const T_ModuleMsg* msg);

#endif // FRAMEWORK_DEF_H