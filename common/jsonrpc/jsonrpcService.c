#include "jsonrpcService.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include "cJSON.h"
#include "common/common.h"

 /***********************************************************
 *                      常量定义                                    *
 **********************************************************/

/***********************************************************
 *              文件内部使用的宏                      *
 **********************************************************/
#define RPC_MAX_CONNECTIONS  1  // 只允许一个客户端连接

#define JSONRPC_SERVICE_CHECK_AND_SET(ptSetup, retVal) \
	do                                              \
	{                                               \
		if (NULL == ptSetup)                        \
		{                                           \
			return retVal;                          \
		}                                           \
	} while (0)

 /***********************************************************
 *          文件内部使用的数据类型     *
 **********************************************************/

typedef struct jsonrpc_method
{
    DECLARE_LIST_NODE_AT_HEAD();
    char *method_name;
    jsonrpc_method_handler handler;
    void *user_data;
} jsonrpc_method_t;

typedef struct __jsonrpc_service_t
{
    int port;
    int server_fd;

    TSK_Handle hDetectTsk;
    BOOL bTskDone;

    T_StdListDef  tMethodList;
	T_MutexObj tMutex; // 操作锁

    void *user_data;

    int client_fd;

} jsonrpc_service_t;

typedef struct
{
    int client_fd;
    jsonrpc_service_t *service;
} client_context_t;

/* ========== 内部函数 ========== */

static int socket_send(int fd, const char *data, size_t len)
{
    size_t sent = 0;
    while (sent < len)
    {
        ssize_t n = send(fd, data + sent, len - sent, 0);
        if (n <= 0)
        {
            return -1;
        }
        sent += n;
    }
    return 0;
}

static int socket_recv(int fd, char *buffer, size_t max_len)
{
    size_t received = 0;

    // 先接收长度头 (4字节)
    uint32_t msg_len = 0;
    if (recv(fd, &msg_len, sizeof(msg_len), MSG_WAITALL) != sizeof(msg_len))
    {
        return -1;
    }

    msg_len = ntohl(msg_len);
    if (msg_len == 0 || msg_len > max_len - 1)
    {
        return -1;
    }

    // 接收消息体
    while (received < msg_len)
    {
        ssize_t n = recv(fd, buffer + received, msg_len - received, 0);
        if (n <= 0)
        {
            return -1;
        }
        received += n;
    }

    buffer[received] = '\0';
    return received;
}

static jsonrpc_method_t *MethodFindNode(jsonrpc_service_t *ptObj, const char *name)
{
	jsonrpc_method_t *ptNode = NULL;
	jsonrpc_method_t *ptNextNode = NULL;


	ptNode = (jsonrpc_method_t *)StdListGetHeadNode(&ptObj->tMethodList);

	while (NULL != ptNode)
	{
		ptNextNode = (jsonrpc_method_t *)StdListGetNextNode((PT_StdNodeDef)ptNode);
		if (strcmp(ptNode->method_name, name) == 0)
        {
            return ptNode;
        }
		ptNode = ptNextNode;
	}
	return NULL;
}

static int handle_jsonrpc_request(jsonrpc_service_t *service, const char *request,
                                  int client_fd)
{
    cJSON *request_json = cJSON_Parse(request);
    cJSON *response_json = NULL;
    char *response_str = NULL;
    int ret = -1;

    if (!request_json)
    {
        response_json = jsonrpc_create_error(JSONRPC_PARSE_ERROR, "Parse error", NULL, NULL);
        goto done;
    }

    // 验证JSON-RPC版本
    cJSON *version = cJSON_GetObjectItem(request_json, "jsonrpc");
    if (!version || !cJSON_IsString(version) ||
        strcmp(version->valuestring, JSONRPC_VERSION) != 0)
    {
        response_json = jsonrpc_create_error(JSONRPC_INVALID_REQUEST,
                                             "Invalid JSON-RPC version", NULL, NULL);
        goto done;
    }

    // 获取方法名
    cJSON *method = cJSON_GetObjectItem(request_json, "method");
    if (!method || !cJSON_IsString(method))
    {
        response_json = jsonrpc_create_error(JSONRPC_INVALID_REQUEST,
                                             "Method not specified", NULL, NULL);
        goto done;
    }

    // 获取ID
    cJSON *id = cJSON_GetObjectItem(request_json, "id");

    // 查找方法
    jsonrpc_method_t *method_handler = MethodFindNode(service, method->valuestring);
    if (!method_handler)
    {
        response_json = jsonrpc_create_error(JSONRPC_METHOD_NOT_FOUND,
                                             "Method not found", NULL, id);
        goto done;
    }

    // 获取参数
    cJSON *params = cJSON_GetObjectItem(request_json, "params");

    // 调用方法
    cJSON *result = method_handler->handler(params, id, method_handler->user_data);
    if (result)
    {
        response_json = jsonrpc_create_response(result, id);
    }
    else
    {
        response_json = jsonrpc_create_error(JSONRPC_INTERNAL_ERROR,
                                             "error", NULL, id);
    }

done:
    if (response_json)
    {
        response_str = cJSON_PrintUnformatted(response_json);
        if (response_str)
        {
            // 发送响应
            uint32_t len = htonl(strlen(response_str));
            if (socket_send(client_fd, (char *)&len, sizeof(len)) == 0)
            {
                socket_send(client_fd, response_str, strlen(response_str));
                ret = 0;
            }
            free(response_str);
        }
        cJSON_Delete(response_json);
    }

    if (request_json)
        cJSON_Delete(request_json);
    return ret;
}

static void *handle_client_connection(void *arg)
{
    client_context_t *ctx = (client_context_t *)arg;
    int client_fd = ctx->client_fd;
    jsonrpc_service_t *service = ctx->service;
    free(ctx);

    char buffer[4096];

    while (1)
    {
        int len = socket_recv(client_fd, buffer, sizeof(buffer));
        if (len <= 0)
        {
            break;
        }

        syslog("Service: Received request: %s\n", buffer);
        handle_jsonrpc_request(service, buffer, client_fd);
    }

    close(client_fd);
    return NULL;
}

static void *service_main_loop(void *arg)
{
    jsonrpc_service_t *service = (jsonrpc_service_t *)arg;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // 创建socket
    if ((service->server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed");
        return NULL;
    }

    // 设置socket选项
    int opt = 1;
    if (setsockopt(service->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt failed");
        close(service->server_fd);
        return NULL;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(service->port);

    if (bind(service->server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        close(service->server_fd);
        return NULL;
    }

    if (listen(service->server_fd, RPC_MAX_CONNECTIONS) < 0)
    {
        perror("listen failed");
        close(service->server_fd);
        return NULL;
    }

    syslog("JSON-RPC Service started on port %d\n", service->port);

    while (!service->bTskDone)
    {
        int client_fd = accept(service->server_fd, (struct sockaddr *)&address,
                               (socklen_t *)&addrlen);
        if (client_fd < 0)
        {
            if (!service->bTskDone)
            {
                perror("accept failed");
            }
            continue;
        }

        syslog("Service: New client connected\n");

        // 为每个客户端创建新线程
        client_context_t *ctx = malloc(sizeof(client_context_t));
        if (!ctx) {
            syserr("malloc client_context_t failed\n");
            close(client_fd);
            continue;
        }
        ctx->client_fd = client_fd;
        ctx->service = service;
        service->client_fd = client_fd;
        pthread_t client_thread;
        pthread_create(&client_thread, NULL, handle_client_connection, ctx);
        pthread_detach(client_thread);
    }

    close(service->server_fd);
    return NULL;
}

void JsonRpcServerReply(HANDLE hService, UINT32 uiCallId, E_StateCode eCode, void *data)
{
    // E_StateCode eCode = STATE_CODE_NO_ERROR;
    jsonrpc_service_t *service = NULL;
    cJSON *response_json = NULL;
    char *response_str = NULL;
    JSONRPC_SERVICE_CHECK_AND_SET(hService, );

    service = (jsonrpc_service_t *)hService;

    cJSON *result = cJSON_Parse(data);
    response_json = jsonrpc_create_response(result, cJSON_CreateNumber(uiCallId));
    cJSON_AddNumberToObject(response_json, "eCode", eCode);

    if (response_json)
    {
        response_str = cJSON_PrintUnformatted(response_json);
        if (response_str)
        {
            // 发送响应
            uint32_t len = htonl(strlen(response_str));
            if (socket_send(service->client_fd, (char *)&len, sizeof(len)) == 0)
            {
                socket_send(service->client_fd, response_str, strlen(response_str));
            }
            free(response_str);
        }
        cJSON_Delete(response_json);
    }
    cJSON_Delete(result);
}

HANDLE jsonrpc_service_create(int port, void* ptUserData)
{
    jsonrpc_service_t *service = malloc(sizeof(jsonrpc_service_t));
    if (!service)
        return NULL;

    service->port = port;
    service->server_fd = -1;
    service->bTskDone = FALSE;
    OSAL_MutexInit(&service->tMutex);
    StdListInit(&service->tMethodList);
    service->user_data = ptUserData;

    return service;
}

void jsonrpc_service_free(HANDLE hService)
{
    jsonrpc_service_t *ptObj = (jsonrpc_service_t *)hService;
    if (!ptObj)
        return;

    jsonrpc_service_stop(ptObj);

    OSAL_MutexDestroy(&ptObj->tMutex);
    free(ptObj);
}

int jsonrpc_service_register_method(HANDLE hservice, const char *name,
                                    jsonrpc_method_handler handler, void *user_data)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    jsonrpc_method_t *ptNode = NULL;
    jsonrpc_service_t *service = NULL;
    JSONRPC_SERVICE_CHECK_AND_SET(hservice, STATE_CODE_INVALID_HANDLE);

    service = (jsonrpc_service_t *)hservice;

    if (!service || !name || !handler)
        return STATE_CODE_INVALID_PARAM;

    ptNode = MethodFindNode(service, name);
    if(ptNode != NULL)
    {
        syslog("Service: Method '%s' already registered\n", name);
        return eCode;
    }

    LJ_SAFE_MALLOC(ptNode, sizeof(jsonrpc_method_t));
    if (!ptNode)
        return STATE_CODE_ALLOCATION_FAILURE;

    ptNode->method_name = strdup(name);
    ptNode->handler = handler;
    ptNode->user_data = user_data;

    OSAL_MutexLock(&service->tMutex);
    StdListPushBack(&service->tMethodList, (PT_StdNodeDef)ptNode);
    OSAL_MutexUnlock(&service->tMutex);
    syslog("Service: Registered method '%s'\n", name);
    return eCode;
}

E_StateCode jsonrpc_service_unregister_method(HANDLE hservice, const char *name)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    jsonrpc_method_t *ptNode = NULL;
    jsonrpc_service_t *service = NULL;
    JSONRPC_SERVICE_CHECK_AND_SET(hservice, STATE_CODE_INVALID_HANDLE);

    service = (jsonrpc_service_t *)hservice;

    if (!service || !name)
        return STATE_CODE_INVALID_PARAM;

    ptNode = MethodFindNode(service, name);

    if(ptNode == NULL)
    {
        syslog("Service: Method '%s' not found\n", name);
        return eCode;
    }

    OSAL_MutexLock(&service->tMutex);
    StdListRemove(&service->tMethodList, (PT_StdNodeDef)ptNode);
    free(ptNode->method_name);
    free(ptNode);
    OSAL_MutexUnlock(&service->tMutex);
    syslog("Service: unRegistered method '%s'\n", name);
    return eCode;
}

E_StateCode jsonrpc_service_start(HANDLE hservice)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    jsonrpc_service_t *service = NULL;
    JSONRPC_SERVICE_CHECK_AND_SET(hservice, STATE_CODE_INVALID_HANDLE);

    service = (jsonrpc_service_t *)hservice;

    OSAL_MutexLock(&service->tMutex);

    // if (service->bTskDone)//!待完善
    // {
    //     goto end;
    // }
        
    TSK_Attrs attrs = DEFAULT_TSK_ATTR;
    service->bTskDone = FALSE;

    attrs.name = "detect";
    attrs.bFifo = TRUE;
    attrs.priority = 10;
    attrs.stackSize = 1024 * 1024;

    service->hDetectTsk = TSK_create(service_main_loop, &attrs, (void*)service);
    if (!service->hDetectTsk) 
    {
        syserr("TSK_create failed\n");
        eCode =  STATE_CODE_ALLOCATION_FAILURE;
    }
// end:
    OSAL_MutexUnlock(&service->tMutex);
    return eCode;
}

void jsonrpc_service_stop(HANDLE hservice)
{
    jsonrpc_service_t *service = (jsonrpc_service_t *)hservice;

    JSONRPC_SERVICE_CHECK_AND_SET(hservice, );

    OSAL_MutexLock(&service->tMutex);
    service->bTskDone = TRUE;
    if (service->server_fd != -1)
    {
        shutdown(service->server_fd, SHUT_RDWR);
        close(service->server_fd);
        service->server_fd = -1;
    }
    TSK_Handle hTsk = service->hDetectTsk;
    service->hDetectTsk = NULL;
    OSAL_MutexUnlock(&service->tMutex);

    /* 先解锁再等待线程结束，避免线程内需要 tMutex 时死锁 */
    if (hTsk) {
        TSK_delete(hTsk);
    }
    syslog("Service stopped\n");
}