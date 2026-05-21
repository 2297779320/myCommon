#include "module_manager.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include "comm_que.h"
#include "common.h"

// 获取当前时间戳
static uint32_t get_current_timestamp()
{
    return (uint32_t)time(NULL);
}

// 深拷贝数据
void *deep_copy_data(const void *src, size_t len)
{
    if (!src || len == 0)
    {
        return NULL;
    }

    void *dest = malloc(len);
    if (!dest)
    {
        return NULL;
    }

    memcpy(dest, src, len);
    return dest;
}

// 创建模块实例
static struct module *create_module_instance(module_id_t id,
                                             ModuleInitFunc init,
                                             ModuleRunFunc run,
                                             ModuleDestroyFunc destroy,
                                             struct framework_context *framework)
{
    struct module *module = (struct module *)malloc(sizeof(struct module));
    if (!module)
    {
        return NULL;
    }

    memset(module, 0, sizeof(struct module));
    module->id = id;
    module->init = init;
    module->run = run;
    module->destroy = destroy;
    module->framework = framework;
    module->msg_handlers = NULL;
    module->msg_handler_count = 0;
    module->next = NULL;

    return module;
}

// 根据ID查找模块
struct module *get_module_by_id(struct framework_context *context, module_id_t id)
{
    if (!context || id == 0)
    {
        return NULL;
    }

    struct module *current = context->modules;
    while (current)
    {
        if (current->id == id)
        {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

// 处理消息队列中的所有待处理消息
void process_framework_message_queue(struct framework_context *context)
{
    if (!context || !context->msg_queue)
    {
        return;
    }

    // 循环处理所有可用消息，超时设为0表示不阻塞
    while (context->is_running)
    {
        T_ModuleMsg *msg = (T_ModuleMsg *)CommQue_GetFull((CommQueID)context->msg_queue, OSAL_TIMEOUT_NONE);
        if (!msg)
        {
            break; // 没有更多消息或接收失败
        }

        // 查找消息处理函数并调用
        if (msg->receiver == 0)
        {
            // 广播消息 - 发送给所有模块
            struct module *current = context->modules;
            while (current)
            {
                // 查找模块中对应的消息处理器
                for (uint32_t i = 0; i < current->msg_handler_count; i++)
                {
                    struct message_handler_entry *entry = &current->msg_handlers[i];
                    if (entry->msg_type == msg->type && entry->enabled && entry->handler)
                    {
                        entry->handler((ModuleHandle)current, msg);
                    }
                }
                current = current->next;
            }
        }
        else
        {
            // 定向消息 - 发送给指定模块
            struct module *target_module = get_module_by_id(context, msg->receiver);
            if (target_module)
            {
                for (uint32_t i = 0; i < target_module->msg_handler_count; i++)
                {
                    struct message_handler_entry *entry = &target_module->msg_handlers[i];
                    if (entry->msg_type == msg->type && entry->enabled && entry->handler)
                    {
                        entry->handler((ModuleHandle)target_module, msg);
                    }
                }
            }
        }

        // 释放消息
        CommQue_PutEmpty((CommQueID)context->msg_queue, msg);
    }
}

// 框架创建与销毁
FrameworkHandle framework_create(uint32_t max_msg_count)
{
    struct framework_context *context = (struct framework_context *)malloc(sizeof(struct framework_context));
    if (!context)
    {
        return NULL;
    }

    memset(context, 0, sizeof(struct framework_context));
    context->modules = NULL;
    context->module_count = 0;
    context->is_running = false;
    context->next_module_id = 1; // 从1开始，0保留给广播

    context->msg_queue = (void *)CommQue_Create(max_msg_count, sizeof(T_ModuleMsg), NULL);
    if (!context->msg_queue)
    {
        free(context);
        return NULL;
    }

    syslog("框架创建成功，消息队列容量: %u\n", max_msg_count);
    return (FrameworkHandle)context;
}

void framework_destroy(FrameworkHandle handle)
{
    if (!handle)
    {
        return;
    }

    struct framework_context *context = (struct framework_context *)handle;

    // 停止主循环
    framework_stop_main_loop(handle);

    // 销毁所有模块
    struct module *current = context->modules;
    while (current)
    {
        struct module *next = current->next;

        if (current->destroy)
        {
            current->destroy((ModuleHandle)current);
        }

        if (current->msg_handlers)
        {
            free(current->msg_handlers);
        }

        free(current);
        current = next;
    }

    // 销毁消息队列
    if (context->msg_queue)
    {
        CommQue_Delete((CommQueID)context->msg_queue);
    }

    free(context);
    syslog("框架已销毁\n");
}

// 模块注册与管理
module_id_t framework_register_module(FrameworkHandle handle,
                                      ModuleInitFunc init,
                                      ModuleRunFunc run,
                                      ModuleDestroyFunc destroy,
                                      void *config)
{
    if (!handle || !init || !run || !destroy)
    {
        syslog("注册模块失败: 参数不完整\n");
        return 0;
    }

    struct framework_context *context = (struct framework_context *)handle;

    // 创建新模块（先用当前 next_module_id，init 失败不递增，避免 ID 空洞）
    struct module *module = create_module_instance(context->next_module_id,
                                                   init, run, destroy, context);
    if (!module)
    {
        syslog("注册模块失败: 内存分配失败\n");
        return 0;
    }

    // 初始化模块（失败时 next_module_id 不递增）
    if (!module->init((ModuleHandle)module, config))
    {
        syslog("注册模块失败: 模块ID %u 初始化失败\n", context->next_module_id);
        free(module);
        return 0;
    }

    // 初始化成功，将模块添加到链表并递增 ID
    module->next = context->modules;
    context->modules = module;
    context->module_count++;
    module_id_t assigned_id = context->next_module_id++;
    syslog("模块注册成功，ID: %u\n", assigned_id);
    return assigned_id;
}

uint32_t framework_get_module_count(FrameworkHandle handle)
{
    if (!handle)
    {
        return 0;
    }
    return ((struct framework_context *)handle)->module_count;
}

bool framework_module_exists(FrameworkHandle handle, module_id_t id)
{
    if (!handle)
    {
        return false;
    }
    return get_module_by_id((struct framework_context *)handle, id) != NULL;
}

// 框架运行控制
bool framework_init_all_modules(FrameworkHandle handle)
{
    if (!handle)
    {
        return false;
    }

    struct framework_context *context = (struct framework_context *)handle;

    syslog("初始化所有模块...\n");
    struct module *current = context->modules;
    while (current)
    {
        syslog("初始化模块: ID %u\n", current->id);
        current = current->next;
    }

    return true;
}

bool framework_start_main_loop(FrameworkHandle handle, uint32_t interval_ms)
{
    if (!handle)
    {
        return false;
    }

    struct framework_context *context = (struct framework_context *)handle;
    if (context->is_running)
    {
        return false;
    }

    syslog("启动框架主循环，模块数量: %u，间隔: %u ms\n",
           context->module_count, interval_ms);

    context->is_running = true;

    // 主循环
    while (context->is_running)
    {
        // 1. 处理消息队列
        process_framework_message_queue(context);

        // 2. 调用所有模块的run函数
        struct module *current = context->modules;
        while (current && context->is_running)
        {
            current->run((ModuleHandle)current);
            current = current->next;
        }

        // 3. 按照指定间隔休眠
        if (interval_ms > 0 && context->is_running)
        {
            usleep(interval_ms * 1000); // 转换为微秒
        }
    }

    syslog("框架主循环已停止\n");
    return true;
}

void framework_stop_main_loop(FrameworkHandle handle)
{
    if (handle)
    {
        ((struct framework_context *)handle)->is_running = false;
    }
}

// 消息发送接口
E_StateCode framework_send_message(FrameworkHandle handle,
                                   module_id_t sender,
                                   module_id_t receiver,
                                   msg_type_t type,
                                   uint32_t call_id,
                                   size_t data_len,
                                   const void *data,
                                   MsgCopyType copy_type,
                                   int32_t timeout)
{
    if (!handle || sender == 0)
    {
        return STATE_CODE_INVALID_PARAM;
    }

    struct framework_context *context = (struct framework_context *)handle;
    if (!context->is_running || !context->msg_queue)
    {
        return STATE_CODE_INVALID_HANDLE;
    }

    // 检查发送者是否存在
    if (!get_module_by_id(context, sender))
    {
        syslog("发送消息失败: 发送者模块ID %u 不存在\n", sender);
        return STATE_CODE_INVALID_PARAM;
    }

    // 创建消息
    T_ModuleMsg msg;
    memset(&msg, 0, sizeof(T_ModuleMsg));
    msg.sender = sender;
    msg.receiver = receiver;
    msg.type = type;
    msg.call_id = call_id;
    msg.timestamp = get_current_timestamp();
    msg.data_len = data_len;
    msg.copy_type = copy_type;

    // 根据拷贝类型处理数据
    if (copy_type == MSG_COPY_DEEP)
    {
        // 深拷贝：分配内存并复制数据
        msg.data = deep_copy_data(data, data_len);
        if (!msg.data && data_len > 0)
        {
            syslog("发送消息失败: 深拷贝内存分配失败\n");
            return STATE_CODE_ALLOCATION_FAILURE;
        }
    }
    else
    {
        // 浅拷贝：仅复制指针
        msg.data = (void *)data;
    }

    // 发送消息：获取空槽 → 复制消息 → 提交到满队列
    T_ModuleMsg *pSlot = (T_ModuleMsg *)CommQue_GetEmpty((CommQueID)context->msg_queue, timeout);
    if (!pSlot)
    {
        // 如果发送失败且是深拷贝，需要释放分配的内存
        if (copy_type == MSG_COPY_DEEP && msg.data)
        {
            free(msg.data);
        }
        syslog("消息发送失败: 获取空槽超时, 发送者 %u -> 接收者 %u\n", sender, receiver);
        return STATE_CODE_TIMEOUT;
    }

    *pSlot = msg;
    CommQue_PutFull((CommQueID)context->msg_queue, pSlot);

    syslog("消息发送成功: 发送者 %u -> 接收者 %u, 类型 %u, 拷贝类型: %s\n",
           sender, receiver, type, copy_type == MSG_COPY_DEEP ? "深拷贝" : "浅拷贝");
    return STATE_CODE_NO_ERROR;
}

E_StateCode module_send_message(ModuleHandle module,
                                module_id_t receiver,
                                msg_type_t type,
                                uint32_t call_id,
                                size_t data_len,
                                const void *data,
                                MsgCopyType copy_type,
                                int32_t timeout)
{
    if (!module)
    {
        return STATE_CODE_INVALID_PARAM;
    }

    struct module *mod = (struct module *)module;
    return framework_send_message((FrameworkHandle)mod->framework,
                                  mod->id, receiver, type, call_id,
                                  data_len, data, copy_type, timeout);
}

// 消息处理器管理接口
bool module_register_handler(ModuleHandle module, msg_type_t msg_type, MessageHandler handler)
{
    if (!module || !handler)
    {
        return false;
    }

    struct module *mod = (struct module *)module;

    // 检查是否已存在相同类型的处理器
    for (uint32_t i = 0; i < mod->msg_handler_count; i++)
    {
        if (mod->msg_handlers[i].msg_type == msg_type)
        {
            mod->msg_handlers[i].handler = handler;
            mod->msg_handlers[i].enabled = true; // 默认为启用状态
            return true;
        }
    }

    // 扩展消息处理表
    uint32_t new_count = mod->msg_handler_count + 1;
    struct message_handler_entry *new_handlers = (struct message_handler_entry *)realloc(
        mod->msg_handlers, new_count * sizeof(struct message_handler_entry));

    if (!new_handlers)
    {
        return false;
    }

    // 添加新的处理器
    new_handlers[new_count - 1].msg_type = msg_type;
    new_handlers[new_count - 1].handler = handler;
    new_handlers[new_count - 1].enabled = true; // 默认为启用状态

    mod->msg_handlers = new_handlers;
    mod->msg_handler_count = new_count;

    syslog("模块 %u 注册消息处理器: 类型 %u\n", mod->id, msg_type);
    return true;
}

E_StateCode module_enable_handler(ModuleHandle module, msg_type_t msg_type, bool enable)
{
    if (!module)
    {
        return STATE_CODE_INVALID_HANDLE;
    }

    struct module *mod = (struct module *)module;

    // 查找消息处理器
    for (uint32_t i = 0; i < mod->msg_handler_count; i++)
    {
        if (mod->msg_handlers[i].msg_type == msg_type)
        {
            mod->msg_handlers[i].enabled = enable;
            syslog("模块 %u %s 消息处理器: 类型 %u\n",
                   mod->id, enable ? "启用" : "禁用", msg_type);
            return STATE_CODE_NO_ERROR;
        }
    }

    return STATE_CODE_INVALID_PARAM;
}

bool module_is_handler_enabled(ModuleHandle module, msg_type_t msg_type)
{
    if (!module)
    {
        return false;
    }

    struct module *mod = (struct module *)module;

    for (uint32_t i = 0; i < mod->msg_handler_count; i++)
    {
        if (mod->msg_handlers[i].msg_type == msg_type)
        {
            return mod->msg_handlers[i].enabled;
        }
    }

    return false;
}

// 消息数据释放接口
void msg_free_data(const T_ModuleMsg *msg)
{
    if (!msg)
    {
        return;
    }

    // 只有深拷贝的消息才需要释放数据
    if (msg->copy_type == MSG_COPY_DEEP && msg->data && msg->data_len > 0)
    {
        free(msg->data);
        syslog("释放深拷贝消息数据\n");
    }
}