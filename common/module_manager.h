/**
 * @file module_manager.h
 * @brief 模块管理器内部实现 -- 框架上下文、模块链表、消息处理表
 *
 * @details
 * 本文件暴露框架的内部数据结构（module、framework_context），
 * 仅供框架内部 .c 文件使用，用户层不应直接包含此文件。
 *
 * @see framework_def.h（依赖 ModuleInitFunc, ModuleRunFunc, ModuleDestroyFunc, msg_type_t, MessageHandler）
 */

#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include "framework_def.h"

// 内部模块结构体定义（仅在.c文件中可见）
struct module {
    module_id_t id;                   // 模块唯一ID
    void* private_data;               // 模块私有数据
    struct framework_context* framework; // 框架上下文指针
    
    // 生命周期函数
    ModuleInitFunc init;              // 初始化函数
    ModuleRunFunc run;                // 运行函数
    ModuleDestroyFunc destroy;        // 销毁函数
    
    // 消息处理表
    struct message_handler_entry* msg_handlers; // 消息处理表数组
    uint32_t msg_handler_count;       // 处理表项数量
    
    // 链表节点
    struct module* next;              // 指向下一个模块
};

// 消息处理表项结构体（内部使用）
struct message_handler_entry {
    msg_type_t msg_type;              // 消息类型
    MessageHandler handler;           // 处理函数
    bool enabled;                     // 是否启用
};

// 框架上下文结构体（内部使用）
struct framework_context {
    struct module* modules;           // 模块链表头指针
    uint32_t module_count;            // 已注册模块数量
    void* msg_queue;                  // 消息队列句柄（内部使用）
    volatile bool is_running;         // 框架运行状态
    uint32_t next_module_id;          // 下一个可用的模块ID
};

// 内部辅助函数声明
struct module* get_module_by_id(struct framework_context* context, module_id_t id);
void process_framework_message_queue(struct framework_context* context);

// 深拷贝辅助函数
void* deep_copy_data(const void* src, size_t len);

#endif // MODULE_MANAGER_H