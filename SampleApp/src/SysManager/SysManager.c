/**************************************************************************
*  Copyright (c) 2024
*  File Name: SysManager.c
*  Description: 系统管理模块 - 使用 V2 框架
**************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "typedef.h"
#include "osal.h"
#include "tsk.h"
#include "global.h"
#include "SampleApp.h"
#include "StbpClient.h"
#include "SysManager.h"
#include "framework_v2.h"

/***********************************************************
*                    Private Types                         *
**********************************************************/
typedef struct {
    BOOL            bDone;
    TSK_Handle      hHeartbeatTsk;
    UINT32          uiHeartbeatCnt;
    ModuleHandleV2  hModule;
} T_SysManager;

static T_SysManager g_stSysManager = {0};

/***********************************************************
*                    Internal Functions                    *
**********************************************************/
static void HeartbeatFxn(void *param)
{
    T_SysManager *ptObj = (T_SysManager *)param;
    HANDLE hUser = NULL;

    if (NULL == ptObj) return;

    dbprintf("[SysManager] Heartbeat thread started.\n");

    while (!ptObj->bDone)
    {
        hUser = GetUserClientHandle();
        if (NULL != hUser && StbpClientConnected(hUser))
        {
            ptObj->uiHeartbeatCnt++;
            StbpClientPublish(hUser, TOPIC_HEARTBEAT, "{\"uptime\": 0}");
            dbprintf("[SysManager] Heartbeat #%u published.\n", ptObj->uiHeartbeatCnt);
        }

        OSAL_Sleep(HEARTBEAT_INTERVAL_MS);
    }

    dbprintf("[SysManager] Heartbeat thread stopped.\n");
}

/***********************************************************
*                    V2 Message Handlers                   *
**********************************************************/
static E_StateCode SysManagerHeartbeatHandler(
    void *pPrivate, T_FrameworkMsgV2 *ptMsg,
    char *pcResMsg, char **ppcResData, uint32_t *puiDataSize, bool *pbDelayRes)
{
    (void)pPrivate;
    (void)ptMsg;
    (void)pcResMsg;
    (void)ppcResData;
    (void)puiDataSize;
    (void)pbDelayRes;

    dbprintf("[SysManager] Heartbeat message received\n");
    return STATE_CODE_NO_ERROR;
}

static T_MsgProcEntryV2 g_satSysManagerTable[] =
{
    {"$report.heartbeat.*.*.*.sample.v1.state", SysManagerHeartbeatHandler, NULL, true, true},
    {NULL, NULL, NULL, false, false}
};

const T_MsgProcEntryV2* GetSysManagerMsgTable(void)
{
    return g_satSysManagerTable;
}

uint32_t GetSysManagerMsgTableLen(void)
{
    return 1;
}

/***********************************************************
*                    Module Functions                      *
**********************************************************/
bool SysManagerInit(ModuleHandleV2 module, void* config)
{
    TSK_Attrs tAttr = DEFAULT_TSK_ATTR;

    (void)config;
    (void)module;

    memset(&g_stSysManager, 0x0, sizeof(g_stSysManager));
    g_stSysManager.hModule = module;

    /* Create heartbeat thread */
    tAttr.name = "SysMgrHeartbeat";
    g_stSysManager.hHeartbeatTsk = TSK_create(HeartbeatFxn, &tAttr, &g_stSysManager);
    if (NULL == g_stSysManager.hHeartbeatTsk)
    {
        SysErr("Create heartbeat thread failed!\n");
        return false;
    }

    dbprintf("[SysManager] Initialized (V2).\n");
    return true;
}

void SysManagerRun(ModuleHandleV2 module)
{
    /* 消息处理由框架自动分发 */
}

void SysManagerDestroy(ModuleHandleV2 module)
{
    g_stSysManager.bDone = TRUE;
    if (NULL != g_stSysManager.hHeartbeatTsk)
    {
        TSK_delete(g_stSysManager.hHeartbeatTsk);
    }
    dbprintf("[SysManager] Deleted (V2).\n");
}
