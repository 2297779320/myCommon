/**************************************************************************
*  Copyright (c) 2024
*  File Name: DeviceCtrl.c
*  Description: 设备控制模块 - 使用 V2 Topic 表驱动消息处理
**************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "typedef.h"
#include "osal.h"
#include "global.h"
#include "SampleApp.h"
#include "StbpClient.h"
#include "DeviceCtrl.h"
#include "framework_v2.h"

/***********************************************************
*                    Private Types                         *
**********************************************************/
typedef struct {
    UINT32  uiDevId;
    BOOL    bOn;
    UINT32  uiSwitchCnt;
} T_DeviceInfo;

typedef struct {
    T_DeviceInfo  atDev[MAX_DEVICE_CNT];
    UINT32        uiDevCnt;
    ModuleHandleV2 hModule;
} T_DeviceCtrl;

static T_DeviceCtrl g_stDeviceCtrl = {0};

/***********************************************************
*                    V2 Message Handlers                   *
**********************************************************/

/**
 * @brief 处理设备开启命令
 * Topic: $request.set.*.*.*.sample.v1.devCtrl.on
 */
static E_StateCode DeviceCtrlOnHandler(
    void *pPrivate, T_FrameworkMsgV2 *ptMsg,
    char *pcResMsg, char **ppcResData, uint32_t *puiDataSize, bool *pbDelayRes)
{
    HANDLE hUser = GetUserClientHandle();
    INT32 iDevId = 0;
    INT8 strState[128];

    (void)pPrivate;
    (void)pcResMsg;
    (void)pbDelayRes;

    if (NULL == ptMsg)
    {
        return STATE_CODE_INVALID_HANDLE;
    }

    dbprintf("[DeviceCtrl] Processing ON command: %s\n", 
             ptMsg->pcBody ? (char*)ptMsg->pcBody : "(null)");

    /* 解析设备ID */
    if (NULL != ptMsg->pcBody)
    {
        sscanf((char*)ptMsg->pcBody, "{\"devId\":%d", &iDevId);
    }

    if (iDevId >= 0 && iDevId < MAX_DEVICE_CNT)
    {
        g_stDeviceCtrl.atDev[iDevId].bOn = TRUE;
        g_stDeviceCtrl.atDev[iDevId].uiSwitchCnt++;
        dbprintf("[DeviceCtrl] Device %d turned ON (total switches: %u)\n",
            iDevId, g_stDeviceCtrl.atDev[iDevId].uiSwitchCnt);

        /* 发布状态变化 */
        if (NULL != hUser && StbpClientConnected(hUser))
        {
            snprintf(strState, sizeof(strState),
                "{\"devId\":%d,\"state\":true}", iDevId);
            StbpClientPublish(hUser, TOPIC_DEV_STATE, strState);
        }

        /* 准备响应 */
        static char response[128];
        snprintf(response, sizeof(response), 
                 "{\"result\":\"success\",\"devId\":%d,\"action\":\"on\"}", iDevId);
        *ppcResData = response;
        *puiDataSize = strlen(response);
    }

    return STATE_CODE_NO_ERROR;
}

/**
 * @brief 处理设备关闭命令
 * Topic: $request.set.*.*.*.sample.v1.devCtrl.off
 */
static E_StateCode DeviceCtrlOffHandler(
    void *pPrivate, T_FrameworkMsgV2 *ptMsg,
    char *pcResMsg, char **ppcResData, uint32_t *puiDataSize, bool *pbDelayRes)
{
    HANDLE hUser = GetUserClientHandle();
    INT32 iDevId = 0;
    INT8 strState[128];

    (void)pPrivate;
    (void)pcResMsg;
    (void)pbDelayRes;

    if (NULL == ptMsg)
    {
        return STATE_CODE_INVALID_HANDLE;
    }

    dbprintf("[DeviceCtrl] Processing OFF command: %s\n",
             ptMsg->pcBody ? (char*)ptMsg->pcBody : "(null)");

    /* 解析设备ID */
    if (NULL != ptMsg->pcBody)
    {
        sscanf((char*)ptMsg->pcBody, "{\"devId\":%d", &iDevId);
    }

    if (iDevId >= 0 && iDevId < MAX_DEVICE_CNT)
    {
        g_stDeviceCtrl.atDev[iDevId].bOn = FALSE;
        g_stDeviceCtrl.atDev[iDevId].uiSwitchCnt++;
        dbprintf("[DeviceCtrl] Device %d turned OFF (total switches: %u)\n",
            iDevId, g_stDeviceCtrl.atDev[iDevId].uiSwitchCnt);

        /* 发布状态变化 */
        if (NULL != hUser && StbpClientConnected(hUser))
        {
            snprintf(strState, sizeof(strState),
                "{\"devId\":%d,\"state\":false}", iDevId);
            StbpClientPublish(hUser, TOPIC_DEV_STATE, strState);
        }

        /* 准备响应 */
        static char response[128];
        snprintf(response, sizeof(response),
                 "{\"result\":\"success\",\"devId\":%d,\"action\":\"off\"}", iDevId);
        *ppcResData = response;
        *puiDataSize = strlen(response);
    }

    return STATE_CODE_NO_ERROR;
}

/**
 * @brief V2 消息处理表（Topic 驱动）
 */
static T_MsgProcEntryV2 g_satDeviceCtrlTable[] =
{
    {"$request.set.*.*.*.sample.v1.devCtrl.on",  DeviceCtrlOnHandler,  NULL, true, true},
    {"$request.set.*.*.*.sample.v1.devCtrl.off", DeviceCtrlOffHandler, NULL, true, true},
    {NULL, NULL, NULL, false, false}  /* 结束标记 */
};

/* 导出消息表和长度 */
const T_MsgProcEntryV2* GetDeviceCtrlMsgTable(void)
{
    return g_satDeviceCtrlTable;
}

uint32_t GetDeviceCtrlMsgTableLen(void)
{
    return 2;  /* 2个处理器 */
}

/***********************************************************
*                    Module Functions                      *
**********************************************************/
bool DeviceCtrlInit(ModuleHandleV2 module, void* config)
{
    (void)config;

    memset(&g_stDeviceCtrl, 0x0, sizeof(g_stDeviceCtrl));
    g_stDeviceCtrl.uiDevCnt = MAX_DEVICE_CNT;
    g_stDeviceCtrl.hModule = module;

    dbprintf("[DeviceCtrl] Initialized with %d devices (V2).\n", g_stDeviceCtrl.uiDevCnt);
    return true;
}

void DeviceCtrlRun(ModuleHandleV2 module)
{
    /* 模块运行逻辑 - 消息处理由框架自动分发 */
}

void DeviceCtrlDestroy(ModuleHandleV2 module)
{
    memset(&g_stDeviceCtrl, 0x0, sizeof(g_stDeviceCtrl));
    dbprintf("[DeviceCtrl] Deleted (V2).\n");
}
