/**************************************************************************
*  Copyright (c) 2024
*  File Name: SysManager.c
*  Description: System management module - heartbeat and lifecycle
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
#include "framework_def.h"

/***********************************************************
*                    Private Types                         *
**********************************************************/
typedef struct {
    BOOL            bDone;
    TSK_Handle      hHeartbeatTsk;
    UINT32          uiHeartbeatCnt;
    ModuleHandle    hModule;
} T_SysManager;

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
*                    Message Handlers                      *
**********************************************************/
static void SysManagerMessageHandler(ModuleHandle module, const T_ModuleMsg* msg)
{
    if (NULL == module || NULL == msg)
    {
        return;
    }

    dbprintf("[SysManager] Received message type: %u\n", msg->type);
    /* Process module-specific messages here if needed */
}

/***********************************************************
*                    Module Functions                      *
**********************************************************/
bool SysManagerInit(ModuleHandle module, void* config)
{
    T_SysManager *ptPrivate = NULL;
    TSK_Attrs tAttr = DEFAULT_TSK_ATTR;

    (void)config;

    ptPrivate = (T_SysManager *)malloc(sizeof(T_SysManager));
    if (NULL == ptPrivate)
    {
        SysErr("malloc failed!\n");
        return false;
    }
    memset(ptPrivate, 0x0, sizeof(T_SysManager));
    ptPrivate->hModule = module;

    /* Register message handler */
    module_register_handler(module, 0, SysManagerMessageHandler);

    /* Create heartbeat thread */
    tAttr.name = "SysMgrHeartbeat";
    ptPrivate->hHeartbeatTsk = TSK_create(HeartbeatFxn, &tAttr, ptPrivate);
    if (NULL == ptPrivate->hHeartbeatTsk)
    {
        SysErr("Create heartbeat thread failed!\n");
        free(ptPrivate);
        return false;
    }

    /* Store private data in module */
    module->private_data = ptPrivate;

    dbprintf("[SysManager] Initialized.\n");
    return true;
}

void SysManagerRun(ModuleHandle module)
{
    T_SysManager *ptPrivate = NULL;

    if (NULL == module)
    {
        return;
    }

    ptPrivate = (T_SysManager *)module->private_data;
    if (NULL == ptPrivate)
    {
        return;
    }

    /* Module run logic here - heartbeat thread handles the work */
    /* This function is called periodically by the framework */
}

void SysManagerDestroy(ModuleHandle module)
{
    T_SysManager *ptPrivate = NULL;

    if (NULL == module)
    {
        return;
    }

    ptPrivate = (T_SysManager *)module->private_data;
    if (NULL == ptPrivate)
    {
        return;
    }

    ptPrivate->bDone = TRUE;
    if (NULL != ptPrivate->hHeartbeatTsk)
    {
        TSK_delete(ptPrivate->hHeartbeatTsk);
    }

    free(ptPrivate);
    module->private_data = NULL;

    dbprintf("[SysManager] Deleted.\n");
}
