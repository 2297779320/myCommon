/**
 * @file framework_v2_example.c
 * @brief Framework V2 使用示例
 *
 * 演示如何使用 V2 框架的 Topic 通配符路由、表驱动分发等功能
 */

#include "framework_v2.h"
#include <stdio.h>
#include <string.h>

/***********************************************************
*                    示例模块定义                          *
**********************************************************/

/* 设备控制模块的消息处理表 */
static E_StateCode DeviceCtrlOnHandler(void *pPrivate, T_FrameworkMsgV2 *ptMsg,
    char *pcResMsg, char **ppcResData, uint32_t *puiDataSize, bool *pbDelayRes)
{
    printf("[DeviceCtrl] ON command received: %s\n", (char*)ptMsg->pcBody);
    printf("[DeviceCtrl] Topic: %s\n", ptMsg->strMsgId);
    
    /* 准备响应 */
    static char response[] = "{\"result\":\"success\",\"action\":\"on\"}";
    *ppcResData = response;
    *puiDataSize = strlen(response);
    
    return STATE_CODE_NO_ERROR;
}

static E_StateCode DeviceCtrlOffHandler(void *pPrivate, T_FrameworkMsgV2 *ptMsg,
    char *pcResMsg, char **ppcResData, uint32_t *puiDataSize, bool *pbDelayRes)
{
    printf("[DeviceCtrl] OFF command received: %s\n", (char*)ptMsg->pcBody);
    printf("[DeviceCtrl] Topic: %s\n", ptMsg->strMsgId);
    
    /* 准备响应 */
    static char response[] = "{\"result\":\"success\",\"action\":\"off\"}";
    *ppcResData = response;
    *puiDataSize = strlen(response);
    
    return STATE_CODE_NO_ERROR;
}

/* V2 消息处理表（支持 Topic 通配符） */
static T_MsgProcEntryV2 g_DeviceCtrlTable[] = {
    {"$request.set.*.*.*.sample.v1.devCtrl.on",  DeviceCtrlOnHandler,  NULL, true, true},
    {"$request.set.*.*.*.sample.v1.devCtrl.off", DeviceCtrlOffHandler, NULL, true, true},
    {NULL, NULL, NULL, false, false}  /* 结束标记 */
};

/* 模块生命周期函数 */
static bool DeviceCtrlV2_Init(ModuleHandleV2 module, void* config)
{
    printf("[DeviceCtrlV2] Module initialized\n");
    module->private_data = NULL;
    return true;
}

static void DeviceCtrlV2_Run(ModuleHandleV2 module)
{
    /* 模块运行逻辑（可选） */
}

static void DeviceCtrlV2_Destroy(ModuleHandleV2 module)
{
    printf("[DeviceCtrlV2] Module destroyed\n");
}

/***********************************************************
*                    主函数示例                            *
**********************************************************/

int main(int argc, char *argv[])
{
    printf("========================================\n");
    printf("  Framework V2 Example\n");
    printf("========================================\n");
    
    /* 1. 创建 V2 框架 */
    FrameworkHandleV2 fw = framework_v2_create(100);
    if (!fw) {
        printf("Failed to create framework V2\n");
        return -1;
    }
    
    /* 2. 注册设备控制模块 */
    uint32_t devCtrlId = framework_v2_register_module(
        fw,
        DeviceCtrlV2_Init,
        DeviceCtrlV2_Run,
        DeviceCtrlV2_Destroy,
        g_DeviceCtrlTable,
        2,  /* 2个消息处理器 */
        NULL
    );
    
    if (devCtrlId == 0) {
        printf("Failed to register DeviceCtrl module\n");
        framework_v2_destroy(fw);
        return -1;
    }
    
    printf("DeviceCtrl module registered, ID: %u\n", devCtrlId);
    
    /* 3. 发送测试消息（使用 Topic 通配符匹配） */
    printf("\n--- Test 1: Send ON command ---\n");
    const char *onData = "{\"devId\":1}";
    framework_v2_send_message(
        fw,
        devCtrlId,        /* sender */
        0,                /* receiver (0=广播) */
        "$request.set.0.1.2.sample.v1.devCtrl.on",  /* Topic */
        NULL,             /* strReply */
        1,                /* call_id */
        strlen(onData),   /* data_len */
        onData,           /* data */
        0,                /* copy_type (0=浅拷贝) */
        OSAL_TIMEOUT_NONE /* timeout */
    );
    
    /* 4. 处理消息 */
    framework_v2_process_messages(fw);
    
    /* 5. 发送 OFF 命令 */
    printf("\n--- Test 2: Send OFF command ---\n");
    const char *offData = "{\"devId\":2}";
    framework_v2_send_message(
        fw,
        devCtrlId,
        0,
        "$request.set.0.1.2.sample.v1.devCtrl.off",
        NULL,
        2,
        strlen(offData),
        offData,
        0,
        OSAL_TIMEOUT_NONE
    );
    
    framework_v2_process_messages(fw);
    
    /* 6. 测试 Topic 不匹配的情况 */
    printf("\n--- Test 3: Send unmatched topic ---\n");
    const char *unmatchedData = "{\"test\":true}";
    framework_v2_send_message(
        fw,
        devCtrlId,
        0,
        "$request.get.0.1.2.sample.v1.devCtrl.state",  /* 不匹配任何处理器 */
        NULL,
        3,
        strlen(unmatchedData),
        unmatchedData,
        0,
        OSAL_TIMEOUT_NONE
    );
    
    framework_v2_process_messages(fw);
    printf("[Main] No handler matched (expected)\n");
    
    /* 7. 清理 */
    printf("\n--- Cleanup ---\n");
    framework_v2_destroy(fw);
    
    printf("\n========================================\n");
    printf("  Framework V2 Example Complete\n");
    printf("========================================\n");
    
    return 0;
}
