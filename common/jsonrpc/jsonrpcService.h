/**
 * @file jsonrpcService.h
 * @brief JSON-RPC 服务端 -- TCP 监听、方法注册、请求分发与回复
 *
 * @details
 * 提供 JSON-RPC 服务端的创建、方法注册/注销、启动/停止，
 * 以及消息的分配、释放和回复接口。
 *
 * @see jsonrpc.h（依赖 T_JsonRpcMsg, cJSON, E_StateCode, HANDLE）
 * @see defs.h（依赖）
 */

#ifndef JSONRPC_SERVICE_H
#define JSONRPC_SERVICE_H

#include "jsonrpc.h"
#include "defs.h"
EXTERN_C_BLOCK


// 方法处理函数类型
typedef cJSON* (*jsonrpc_method_handler)(cJSON *params, cJSON *id, void *user_data);


/**********************************************************************
 * 函数名称：jsonrpc_service_create
 * 功能描述：创建JSON-RPC服务端
 * 输入参数：无
 * 输出参数：无
 * 返 回 值：    状态码
 * 其它说明：
 * 修改日期        版本号     修改人        修改内容
 * -----------------------------------------------
 * 2025/11/10        V1.0              chengjiahao
 ***********************************************************************/
HANDLE jsonrpc_service_create(int port, void* ptUserData);

/**********************************************************************
 * 函数名称：jsonrpc_service_free
 * 功能描述：释放JSON-RPC服务端
 * 输入参数：无
 * 输出参数：无
 * 返 回 值：    状态码
 * 其它说明：
 * 修改日期        版本号     修改人        修改内容
 * -----------------------------------------------
 * 2025/11/10        V1.0              chengjiahao
 ***********************************************************************/
void jsonrpc_service_free(HANDLE hService);

/**********************************************************************
 * 函数名称：jsonrpc_service_register_method
 * 功能描述：注册JSON-RPC方法
 * 输入参数：无
 * 输出参数：无
 * 返 回 值：    状态码
 * 其它说明：
 * 修改日期        版本号     修改人        修改内容
 * -----------------------------------------------
 * 2025/11/10        V1.0              chengjiahao
 ***********************************************************************/
E_StateCode jsonrpc_service_register_method(HANDLE hservice, const char *name, 
                                   jsonrpc_method_handler handler, void *user_data);

/**********************************************************************
 * 函数名称：jsonrpc_service_unregister_method
 * 功能描述：注销JSON-RPC方法
 * 输入参数：无
 * 输出参数：无
 * 返 回 值：    状态码
 * 其它说明：
 * 修改日期        版本号     修改人        修改内容
 * -----------------------------------------------
 * 2025/11/10        V1.0              chengjiahao
 ***********************************************************************/                                   
E_StateCode jsonrpc_service_unregister_method(HANDLE hservice, const char *name);

/**********************************************************************
 * 函数名称：jsonrpc_service_start
 * 功能描述：启动JSON-RPC服务端
 * 输入参数：无
 * 输出参数：无
 * 返 回 值：    状态码
 * 其它说明：
 * 修改日期        版本号     修改人        修改内容
 * -----------------------------------------------
 * 2025/11/10        V1.0              chengjiahao
 ***********************************************************************/
int jsonrpc_service_start(HANDLE hservice);

/**********************************************************************
 * 函数名称：jsonrpc_service_stop
 * 功能描述：停止JSON-RPC服务端
 * 输入参数：无
 * 输出参数：无
 * 返 回 值：    状态码
 * 其它说明：
 * 修改日期        版本号     修改人        修改内容
 * -----------------------------------------------
 * 2025/11/10        V1.0              chengjiahao
 ***********************************************************************/
void jsonrpc_service_stop(HANDLE hservice);

/**********************************************************************
 * 函数名称：JsonRpcServerAllocMsg
 * 功能描述：获取JSON-RPC消息
 * 输入参数：无
 * 输出参数：无
 * 返 回 值：    状态码
 * 其它说明：
 * 修改日期        版本号     修改人        修改内容
 * -----------------------------------------------
 * 2025/11/10        V1.0              chengjiahao
 ***********************************************************************/
T_JsonRpcMsg* JsonRpcServerAllocMsg(HANDLE hService, UINT32 time);

/**********************************************************************
 * 函数名称：JsonRpcServerFreeMsg
 * 功能描述： 释放JSON-RPC消息
 * 输入参数：无
 * 输出参数：无
 * 返 回 值：    状态码
 * 其它说明：
 * 修改日期        版本号     修改人        修改内容
 * -----------------------------------------------
 * 2025/11/10        V1.0              chengjiahao
 ***********************************************************************/
E_StateCode JsonRpcServerFreeMsg(HANDLE hService, T_JsonRpcMsg* ptMsg);

/**********************************************************************
 * 函数名称：JsonRpcServerReply
 * 功能描述：回复JSON-RPC消息
 * 输入参数：无
 * 输出参数：无
 * 返 回 值：    状态码
 * 其它说明：
 * 修改日期        版本号     修改人        修改内容
 * -----------------------------------------------
 * 2025/11/10        V1.0              chengjiahao
 ***********************************************************************/
void JsonRpcServerReply(HANDLE hService, UINT32 uiCallId, E_StateCode eCode, void *data);

EXTERN_C_BLOCK_END

#endif // JSONRPC_SERVICE_H