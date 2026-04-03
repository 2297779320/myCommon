#include "jsonrpcClient.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

/***********************************************************
 *              文件内部使用的宏                      *
 **********************************************************/
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
typedef struct  __jsonrpc_client_t
{
    char *host;
    int port;
    int sock_fd;
    int connected;
} jsonrpc_client_t;

static int client_connect(HANDLE hclient)
{
    jsonrpc_client_t *ptObj = (jsonrpc_client_t *)hclient;
    if (ptObj->connected)
        return 0;

    struct sockaddr_in serv_addr;

    if ((ptObj->sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket creation failed");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(ptObj->port);

    if (inet_pton(AF_INET, ptObj->host, &serv_addr.sin_addr) <= 0)
    {
        perror("invalid address");
        close(ptObj->sock_fd);
        return -1;
    }

    if (connect(ptObj->sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connection failed");
        close(ptObj->sock_fd);
        return -1;
    }

    ptObj->connected = 1;
    syslog("Client: Connected to %s:%d\n", ptObj->host, ptObj->port);
    return 0;
}

static int client_send_request(jsonrpc_client_t *client, const char *request,
                               char *response, size_t max_response)
{
    if (!client->connected && client_connect(client) != 0)
    {
        return -1;
    }

    uint32_t len = htonl(strlen(request));
    if (send(client->sock_fd, &len, sizeof(len), 0) != sizeof(len))
    {
        client->connected = 0;
        return -1;
    }

    if (send(client->sock_fd, request, strlen(request), 0) != (ssize_t)strlen(request))
    {
        client->connected = 0;
        return -1;
    }

    uint32_t response_len = 0;
    if (recv(client->sock_fd, &response_len, sizeof(response_len), MSG_WAITALL) != sizeof(response_len))
    {
        client->connected = 0;
        return -1;
    }

    response_len = ntohl(response_len);
    if (response_len == 0 || response_len >= max_response)
    {
        return -1;
    }

    size_t received = 0;
    while (received < response_len)
    {
        ssize_t n = recv(client->sock_fd, response + received, response_len - received, 0);
        if (n <= 0)
        {
            client->connected = 0;
            return -1;
        }
        received += n;
    }

    response[received] = '\0';
    return received;
}

HANDLE jsonrpc_client_create(const char *host, int port)
{
    jsonrpc_client_t *client = malloc(sizeof(jsonrpc_client_t));
    if (!client)
        return NULL;

    client->host = strdup(host);
    client->port = port;
    client->sock_fd = -1;
    client->connected = 0;

    return client;
}

void jsonrpc_client_free(HANDLE hclient)
{
    jsonrpc_client_t *client = (jsonrpc_client_t *)hclient;
    if (!client)
        return;

    if (client->sock_fd != -1)
    {
        close(client->sock_fd);
    }

    free(client->host);
    free(client);
}

E_StateCode jsonrpc_client_call(HANDLE hclient, const char *method, 
                          cJSON *params, cJSON **result)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    jsonrpc_client_t *client = (jsonrpc_client_t *)hclient;

    if (!client || !method)
    {
        return STATE_CODE_INVALID_PARAM;
    }

    /* request_id 用静态 mutex 保护，保证多线程并发调用时 ID 唯一递增 */
    static int request_id = 1;
    static pthread_mutex_t request_id_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&request_id_mutex);
    int cur_id = request_id++;
    pthread_mutex_unlock(&request_id_mutex);

    // 创建请求
    cJSON *request = jsonrpc_create_request(method, params, cJSON_CreateNumber(cur_id));
    if (!request)
    {
        return STATE_CODE_FAILED_TO_PROCEED_COMMAND;
    }

    char *request_str = cJSON_PrintUnformatted(request);
    cJSON_Delete(request);

    if (!request_str)
    {
        return STATE_CODE_INVALID_PARAM;
    }

    printf("Client: Sending request: %s\n", request_str);

    // 发送请求并接收响应
    char response_buf[4096];
    int response_len = client_send_request(client, request_str, response_buf, sizeof(response_buf));
    free(request_str);

    if (response_len <= 0)
    {
        return STATE_CODE_INVALID_PARAM;
    }

    printf("Client: Received response: %s\n", response_buf);

    // 解析响应
    cJSON *response = cJSON_Parse(response_buf);
    if (!response)
    {
        return STATE_CODE_INVALID_PARAM;
    }

    // 检查错误
    cJSON *error = cJSON_GetObjectItem(response, "error");
    if (error)
    {
        cJSON *code = cJSON_GetObjectItem(error, "code");
        eCode = code ? code->valueint : STATE_CODE_FAILED_TO_PROCEED_COMMAND;
        cJSON_Delete(response);
        return eCode;
    }

    // 获取结果
    cJSON *temp = cJSON_GetObjectItem(response, "result");
    if (temp)
    {
        *result = cJSON_Duplicate(temp, 1);
    }

    cJSON_Delete(response);

    return eCode;
}

// int jsonrpc_client_call_async(jsonrpc_client_t *client, const char *method,
//                               cJSON *params, cJSON **result)
// {
//     // 简化实现：同步调用
//     int error_code;
//     *result = jsonrpc_client_call(client, method, params, &error_code);
//     return error_code;
// }