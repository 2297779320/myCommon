/**
 * @file framework_v2.c
 * @brief Framework V2 实现 -- Topic通配符路由、表驱动分发、Request-Response机制
 *
 * @details
 * 实现 V2 框架的核心功能：
 *   - Topic 通配符匹配算法
 *   - 表驱动消息分发
 *   - Request-Response 机制
 *   - 延迟响应支持
 *   - 模块消息队列管理
 */

#include "framework_v2.h"
#include "comm_que.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/***********************************************************
*                    内部数据结构                          *
**********************************************************/

/**
 * @brief V2 模块内部结构
 */
struct module_v2 {
    uint32_t id;                                   /**< 模块ID */
    void *private_data;                            /**< 模块私有数据 */
    struct framework_context_v2 *framework;        /**< 框架上下文指针 */
    
    /** 生命周期函数 */
    bool (*init)(ModuleHandleV2, void*);
    void (*run)(ModuleHandleV2);
    void (*destroy)(ModuleHandleV2);
    
    /** V2 消息处理表 */
    T_MsgProcEntryV2 *msg_table;
    uint32_t msg_table_len;
    uint32_t msg_table_capacity;
    
    /** 链表节点 */
    struct module_v2 *next;
};

/**
 * @brief V2 框架内部上下文
 */
struct framework_context_v2 {
    struct module_v2 *modules;                     /**< 模块链表头 */
    uint32_t module_count;                         /**< 模块数量 */
    void *msg_queue;                               /**< CommQue句柄 */
    volatile bool is_running;                      /**< 运行标志 */
    uint32_t next_module_id;                       /**< 下一个可用模块ID */
};

/***********************************************************
*                    内部辅助函数                          *
**********************************************************/

/**
 * @brief 根据ID查找模块
 */
static struct module_v2* get_module_by_id_v2(struct framework_context_v2 *context, uint32_t id)
{
    if (!context || id == 0) return NULL;
    
    struct module_v2 *current = context->modules;
    while (current) {
        if (current->id == id) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/**
 * @brief 获取当前时间戳
 */
static uint32_t get_current_timestamp_v2(void)
{
    return (uint32_t)time(NULL);
}

/***********************************************************
*                    Topic 匹配算法                        *
**********************************************************/

/**
 * @brief Topic 通配符匹配
 * 
 * 规则：
 *   - 按 '.' 分割为多个字段
 *   - '*' 匹配任意单个字段
 *   - 字段数量必须相同
 *   - 其他字段必须精确匹配
 */
bool IsTopicEqualV2(const char *pattern, const char *topic)
{
    if (!pattern || !topic) return false;
    
    const char *p = pattern;
    const char *t = topic;
    
    while (*p && *t) {
        /* 提取当前字段 */
        const char *p_end = strchr(p, '.');
        const char *t_end = strchr(t, '.');
        
        /* 计算字段长度 */
        size_t p_len = p_end ? (size_t)(p_end - p) : strlen(p);
        size_t t_len = t_end ? (size_t)(t_end - t) : strlen(t);
        
        /* 检查通配符 */
        if (p_len == 1 && *p == '*') {
            /* '*' 匹配任意字段，跳过 */
        } else if (p_len != t_len || strncmp(p, t, p_len) != 0) {
            return false;  /* 字段不匹配 */
        }
        
        /* 移动到下一个字段 */
        p = p_end ? p_end + 1 : p + p_len;
        t = t_end ? t_end + 1 : t + t_len;
    }
    
    /* 两个字符串都必须结束 */
    return *p == '\0' && *t == '\0';
}

/**
 * @brief 从 Topic 中提取 ID（最后一段）
 */
const char* GetTopicIdV2(const char *topic)
{
    if (!topic) return NULL;
    
    const char *last_dot = strrchr(topic, '.');
    if (last_dot) {
        return last_dot + 1;
    }
    return topic;
}

/**
 * @brief 从 Topic 中提取客户端 ID（第三段）
 */
const char* GetClientIdV2(const char *topic)
{
    if (!topic) return NULL;
    
    int dot_count = 0;
    const char *start = topic;
    const char *end = topic;
    
    while (*end && dot_count < 2) {
        if (*end == '.') {
            dot_count++;
            if (dot_count == 2) {
                start = end + 1;
            }
        }
        end++;
    }
    
    if (dot_count < 2) return NULL;
    
    const char *next_dot = strchr(start, '.');
    if (next_dot) {
        static char client_id[32];
        size_t len = (size_t)(next_dot - start);
        if (len >= sizeof(client_id)) len = sizeof(client_id) - 1;
        memcpy(client_id, start, len);
        client_id[len] = '\0';
        return client_id;
    }
    
    return start;
}

/***********************************************************
*                    框架创建与销毁                        *
**********************************************************/

FrameworkHandleV2 framework_v2_create(uint32_t max_msg_count)
{
    struct framework_context_v2 *context = 
        (struct framework_context_v2 *)malloc(sizeof(struct framework_context_v2));
    if (!context) {
        syslog("Framework V2: 创建失败，内存分配失败\n");
        return NULL;
    }
    
    memset(context, 0, sizeof(struct framework_context_v2));
    context->is_running = false;
    context->next_module_id = 1;  /* 从1开始，0保留给广播 */
    
    /* 创建消息队列 */
    context->msg_queue = (void *)CommQue_Create(max_msg_count, sizeof(T_FrameworkMsgV2), NULL);
    if (!context->msg_queue) {
        syslog("Framework V2: 创建失败，消息队列创建失败\n");
        free(context);
        return NULL;
    }
    
    syslog("Framework V2: 创建成功，消息队列容量: %u\n", max_msg_count);
    return (FrameworkHandleV2)context;
}

void framework_v2_destroy(FrameworkHandleV2 handle)
{
    if (!handle) return;
    
    struct framework_context_v2 *context = (struct framework_context_v2 *)handle;
    
    /* 停止主循环 */
    framework_v2_stop_main_loop(handle);
    
    /* 销毁所有模块 */
    struct module_v2 *current = context->modules;
    while (current) {
        struct module_v2 *next = current->next;
        
        if (current->destroy) {
            current->destroy((ModuleHandleV2)current);
        }
        
        if (current->msg_table) {
            free(current->msg_table);
        }
        
        free(current);
        current = next;
    }
    
    /* 销毁消息队列 */
    if (context->msg_queue) {
        CommQue_Delete((CommQueID)context->msg_queue);
    }
    
    free(context);
    syslog("Framework V2: 已销毁\n");
}

/***********************************************************
*                    模块注册与管理                        *
**********************************************************/

uint32_t framework_v2_register_module(
    FrameworkHandleV2 handle,
    bool (*init)(ModuleHandleV2, void*),
    void (*run)(ModuleHandleV2),
    void (*destroy)(ModuleHandleV2),
    const T_MsgProcEntryV2 *msg_table,
    uint32_t msg_table_len,
    void *config)
{
    if (!handle || !init || !run || !destroy) {
        syslog("Framework V2: 注册模块失败，参数不完整\n");
        return 0;
    }
    
    struct framework_context_v2 *context = (struct framework_context_v2 *)handle;
    
    /* 创建模块实例 */
    struct module_v2 *mod = (struct module_v2 *)malloc(sizeof(struct module_v2));
    if (!mod) {
        syslog("Framework V2: 注册模块失败，内存分配失败\n");
        return 0;
    }
    
    memset(mod, 0, sizeof(struct module_v2));
    mod->id = context->next_module_id;
    mod->init = init;
    mod->run = run;
    mod->destroy = destroy;
    mod->framework = context;
    
    /* 复制消息表 */
    if (msg_table && msg_table_len > 0) {
        mod->msg_table = (T_MsgProcEntryV2 *)malloc(msg_table_len * sizeof(T_MsgProcEntryV2));
        if (!mod->msg_table) {
            syslog("Framework V2: 注册模块失败，消息表内存分配失败\n");
            free(mod);
            return 0;
        }
        memcpy(mod->msg_table, msg_table, msg_table_len * sizeof(T_MsgProcEntryV2));
        mod->msg_table_len = msg_table_len;
        mod->msg_table_capacity = msg_table_len;
    }
    
    /* 调用模块初始化 */
    if (!mod->init((ModuleHandleV2)mod, config)) {
        syslog("Framework V2: 注册模块失败，模块ID %u 初始化失败\n", context->next_module_id);
        if (mod->msg_table) free(mod->msg_table);
        free(mod);
        return 0;
    }
    
    /* 添加到链表（头插法） */
    mod->next = context->modules;
    context->modules = mod;
    context->module_count++;
    
    uint32_t assigned_id = context->next_module_id++;
    syslog("Framework V2: 模块注册成功，ID: %u\n", assigned_id);
    
    return assigned_id;
}

uint32_t framework_v2_get_module_count(FrameworkHandleV2 handle)
{
    if (!handle) return 0;
    return ((struct framework_context_v2 *)handle)->module_count;
}

/***********************************************************
*                    消息发送                              *
**********************************************************/

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
    int32_t timeout)
{
    if (!handle || !strMsgId) {
        return STATE_CODE_INVALID_PARAM;
    }
    
    struct framework_context_v2 *ctx = (struct framework_context_v2 *)handle;
    if (!ctx->msg_queue) {
        return STATE_CODE_INVALID_HANDLE;
    }
    
    /* 创建消息 */
    T_FrameworkMsgV2 msg;
    memset(&msg, 0, sizeof(msg));
    msg.sender = sender;
    msg.receiver = receiver;
    strncpy(msg.strMsgId, strMsgId, sizeof(msg.strMsgId) - 1);
    if (strReply) {
        strncpy(msg.strReply, strReply, sizeof(msg.strReply) - 1);
    }
    msg.uiCallId = call_id;
    msg.uiBodySize = data_len;
    msg.copy_type = copy_type;
    msg.timestamp = get_current_timestamp_v2();
    
    /* 处理数据拷贝 */
    if (copy_type == 1 && data_len > 0) {  /* 深拷贝 */
        msg.pcBody = malloc(data_len);
        if (!msg.pcBody) {
            syslog("Framework V2: 发送消息失败，深拷贝内存分配失败\n");
            return STATE_CODE_ALLOCATION_FAILURE;
        }
        memcpy(msg.pcBody, data, data_len);
    } else {
        msg.pcBody = (void*)data;  /* 浅拷贝 */
    }
    
    /* 发送到队列 */
    T_FrameworkMsgV2 *slot = (T_FrameworkMsgV2 *)CommQue_GetEmpty(
        (CommQueID)ctx->msg_queue, timeout);
    if (!slot) {
        if (copy_type == 1 && msg.pcBody) free(msg.pcBody);
        syslog("Framework V2: 发送消息失败，获取空槽超时\n");
        return STATE_CODE_TIMEOUT;
    }
    
    *slot = msg;
    CommQue_PutFull((CommQueID)ctx->msg_queue, slot);
    
    return STATE_CODE_NO_ERROR;
}

E_StateCode framework_v2_send_response(
    FrameworkHandleV2 handle,
    T_FrameworkMsgV2 *request_msg,
    const char *strReply,
    uint32_t data_len,
    const void *data)
{
    if (!handle || !request_msg) {
        return STATE_CODE_INVALID_PARAM;
    }
    
    return framework_v2_send_message(
        handle,
        request_msg->receiver,  /* 原来的接收者现在变成发送者 */
        request_msg->sender,    /* 回复给原来的发送者 */
        strReply ? strReply : request_msg->strMsgId,
        request_msg->strReply,
        request_msg->uiCallId,
        data_len,
        data,
        1,  /* 深拷贝 */
        OSAL_TIMEOUT_NONE
    );
}

E_StateCode module_v2_send_message(
    ModuleHandleV2 module,
    uint32_t receiver,
    const char *strMsgId,
    const char *strReply,
    uint32_t call_id,
    uint32_t data_len,
    const void *data,
    int copy_type,
    int32_t timeout)
{
    if (!module) {
        return STATE_CODE_INVALID_PARAM;
    }
    
    struct module_v2 *mod = (struct module_v2 *)module;
    return framework_v2_send_message(
        (FrameworkHandleV2)mod->framework,
        mod->id,
        receiver,
        strMsgId,
        strReply,
        call_id,
        data_len,
        data,
        copy_type,
        timeout
    );
}

/***********************************************************
*                    消息处理                              *
**********************************************************/

/**
 * @brief 处理消息队列中的所有待处理消息
 */
static void process_framework_message_queue_v2(struct framework_context_v2 *context)
{
    if (!context || !context->msg_queue) return;
    
    /* 循环处理所有消息（不阻塞） */
    while (context->is_running) {
        T_FrameworkMsgV2 *msg = (T_FrameworkMsgV2 *)CommQue_GetFull(
            (CommQueID)context->msg_queue, OSAL_TIMEOUT_NONE);
        if (!msg) break;  /* 无更多消息 */
        
        bool delay_res = false;
        char res_msg[256] = {0};
        char *res_data = NULL;
        uint32_t res_data_size = 0;
        
        if (msg->receiver == 0) {
            /* 广播：遍历所有模块 */
            struct module_v2 *mod = context->modules;
            while (mod) {
                /* 遍历模块的消息表 */
                for (uint32_t i = 0; i < mod->msg_table_len; i++) {
                    T_MsgProcEntryV2 *entry = &mod->msg_table[i];
                    if (!entry->enabled || !entry->pfProc) continue;
                    
                    bool matched = false;
                    if (entry->use_topic_match) {
                        /* Topic 通配符匹配 */
                        matched = IsTopicEqualV2(entry->strMsgId, msg->strMsgId);
                    } else {
                        /* 精确字符串匹配 */
                        matched = (strcmp(entry->strMsgId, msg->strMsgId) == 0);
                    }
                    
                    if (matched) {
                        /* 调用处理函数 */
                        E_StateCode ret = entry->pfProc(
                            entry->pPrivate ? entry->pPrivate : mod->private_data,
                            msg, res_msg, &res_data, &res_data_size, &delay_res);
                        
                        if (ret == STATE_CODE_NO_ERROR && !delay_res && res_data) {
                            /* 同步响应：发送回复 */
                            framework_v2_send_response(
                                context, msg, msg->strReply,
                                res_data_size, res_data);
                        }
                        break;  /* 找到匹配项，停止 */
                    }
                }
                mod = mod->next;
            }
        } else {
            /* 定向发送：查找目标模块 */
            struct module_v2 *target = get_module_by_id_v2(context, msg->receiver);
            if (target) {
                /* 同样遍历消息表匹配 */
                for (uint32_t i = 0; i < target->msg_table_len; i++) {
                    T_MsgProcEntryV2 *entry = &target->msg_table[i];
                    if (!entry->enabled || !entry->pfProc) continue;
                    
                    bool matched = entry->use_topic_match
                        ? IsTopicEqualV2(entry->strMsgId, msg->strMsgId)
                        : (strcmp(entry->strMsgId, msg->strMsgId) == 0);
                    
                    if (matched) {
                        entry->pfProc(
                            entry->pPrivate ? entry->pPrivate : target->private_data,
                            msg, res_msg, &res_data, &res_data_size, &delay_res);
                        break;
                    }
                }
            }
        }
        
        /* 释放深拷贝数据 */
        if (msg->copy_type == 1 && msg->pcBody) {
            free(msg->pcBody);
            msg->pcBody = NULL;
        }
        
        /* 归还消息到队列池 */
        CommQue_PutEmpty((CommQueID)context->msg_queue, msg);
    }
}

void framework_v2_process_messages(FrameworkHandleV2 handle)
{
    if (!handle) return;
    process_framework_message_queue_v2((struct framework_context_v2 *)handle);
}

/***********************************************************
*                    动态消息处理器注册                    *
**********************************************************/

bool module_v2_add_handler(
    ModuleHandleV2 module,
    const char *strMsgId,
    MsgProcFuncV2 handler,
    bool use_topic_match)
{
    if (!module || !strMsgId || !handler) {
        return false;
    }
    
    struct module_v2 *mod = (struct module_v2 *)module;
    
    /* 检查是否已存在 */
    for (uint32_t i = 0; i < mod->msg_table_len; i++) {
        if (strcmp(mod->msg_table[i].strMsgId, strMsgId) == 0) {
            mod->msg_table[i].pfProc = handler;
            mod->msg_table[i].enabled = true;
            mod->msg_table[i].use_topic_match = use_topic_match;
            return true;
        }
    }
    
    /* 扩展消息表 */
    if (mod->msg_table_len >= mod->msg_table_capacity) {
        uint32_t new_capacity = mod->msg_table_capacity == 0 ? 8 : mod->msg_table_capacity * 2;
        T_MsgProcEntryV2 *new_table = (T_MsgProcEntryV2 *)realloc(
            mod->msg_table, new_capacity * sizeof(T_MsgProcEntryV2));
        if (!new_table) {
            return false;
        }
        mod->msg_table = new_table;
        mod->msg_table_capacity = new_capacity;
    }
    
    /* 添加新处理器 */
    T_MsgProcEntryV2 *entry = &mod->msg_table[mod->msg_table_len];
    entry->strMsgId = strMsgId;
    entry->pfProc = handler;
    entry->pPrivate = NULL;
    entry->enabled = true;
    entry->use_topic_match = use_topic_match;
    mod->msg_table_len++;
    
    syslog("Framework V2: 模块 %u 添加消息处理器: %s\n", mod->id, strMsgId);
    return true;
}

/***********************************************************
*                    框架运行控制                          *
**********************************************************/

bool framework_v2_init_all_modules(FrameworkHandleV2 handle)
{
    if (!handle) return false;
    
    struct framework_context_v2 *context = (struct framework_context_v2 *)handle;
    
    syslog("Framework V2: 初始化所有模块...\n");
    struct module_v2 *current = context->modules;
    while (current) {
        syslog("Framework V2: 初始化模块: ID %u\n", current->id);
        current = current->next;
    }
    
    return true;
}

bool framework_v2_start_main_loop(FrameworkHandleV2 handle, uint32_t interval_ms)
{
    if (!handle) return false;
    
    struct framework_context_v2 *context = (struct framework_context_v2 *)handle;
    if (context->is_running) {
        return false;
    }
    
    syslog("Framework V2: 启动主循环，模块数量: %u，间隔: %u ms\n",
           context->module_count, interval_ms);
    
    context->is_running = true;
    
    /* 主循环 */
    while (context->is_running) {
        /* 1. 处理消息队列 */
        process_framework_message_queue_v2(context);
        
        /* 2. 调用所有模块的run函数 */
        struct module_v2 *current = context->modules;
        while (current && context->is_running) {
            current->run((ModuleHandleV2)current);
            current = current->next;
        }
        
        /* 3. 按照指定间隔休眠 */
        if (interval_ms > 0 && context->is_running) {
            usleep(interval_ms * 1000);  /* 转换为微秒 */
        }
    }
    
    syslog("Framework V2: 主循环已停止\n");
    return true;
}

void framework_v2_stop_main_loop(FrameworkHandleV2 handle)
{
    if (handle) {
        ((struct framework_context_v2 *)handle)->is_running = false;
    }
}
