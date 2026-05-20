/**************************************************************************
*  Copyright (c) 2024
*  File Name: DeviceCtrl.c
*  Description: Device control module - handles on/off commands via message dispatch
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
#include "framework_def.h"

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
    ModuleHandle  hModule;
} T_DeviceCtrl;

/***********************************************************
*                    Message Handlers                      *
**********************************************************/
static void DeviceCtrlMessageHandler(ModuleHandle module, const T_ModuleMsg* msg)
{
    T_DeviceCtrl *ptDevCtrl = NULL;
    HANDLE hUser = NULL;
    INT32 iDevId = 0;
    INT8 strState[128];

    if (NULL == module || NULL == msg || NULL == msg->data)
    {
        return;
    }

    ptDevCtrl = (T_DeviceCtrl *)module->private_data;
    if (NULL == ptDevCtrl)
    {
        return;
    }

    hUser = GetUserClientHandle();

    dbprintf("[DeviceCtrl] Processing message type: %u, data: %s\n",
        msg->type, (const char*)msg->data);

    /* Parse device ID from JSON body if available */
    if (NULL != msg->data && msg->data_len > 0)
    {
        sscanf((const char*)msg->data, "{\"devId\":%d", &iDevId);
    }

    /* Handle based on message type */
    if (msg->type == 1) /* DEV_CTRL_ON */
    {
        if (iDevId >= 0 && iDevId < MAX_DEVICE_CNT)
        {
            ptDevCtrl->atDev[iDevId].bOn = TRUE;
            ptDevCtrl->atDev[iDevId].uiSwitchCnt++;
            dbprintf("[DeviceCtrl] Device %d turned ON (total switches: %u)\n",
                iDevId, ptDevCtrl->atDev[iDevId].uiSwitchCnt);

            /* Publish state change */
            if (NULL != hUser && StbpClientConnected(hUser))
            {
                snprintf(strState, sizeof(strState),
                    "{\"devId\":%d,\"state\":true}", iDevId);
                StbpClientPublish(hUser, TOPIC_DEV_STATE, strState);
            }
        }
    }
    else if (msg->type == 2) /* DEV_CTRL_OFF */
    {
        if (iDevId >= 0 && iDevId < MAX_DEVICE_CNT)
        {
            ptDevCtrl->atDev[iDevId].bOn = FALSE;
            ptDevCtrl->atDev[iDevId].uiSwitchCnt++;
            dbprintf("[DeviceCtrl] Device %d turned OFF (total switches: %u)\n",
                iDevId, ptDevCtrl->atDev[iDevId].uiSwitchCnt);

            /* Publish state change */
            if (NULL != hUser && StbpClientConnected(hUser))
            {
                snprintf(strState, sizeof(strState),
                    "{\"devId\":%d,\"state\":false}", iDevId);
                StbpClientPublish(hUser, TOPIC_DEV_STATE, strState);
            }
        }
    }
}

/***********************************************************
*                    Module Functions                      *
**********************************************************/
bool DeviceCtrlInit(ModuleHandle module, void* config)
{
    T_DeviceCtrl *ptPrivate = NULL;

    (void)config;

    ptPrivate = (T_DeviceCtrl *)malloc(sizeof(T_DeviceCtrl));
    if (NULL == ptPrivate)
    {
        SysErr("malloc failed!\n");
        return false;
    }
    memset(ptPrivate, 0x0, sizeof(T_DeviceCtrl));
    ptPrivate->uiDevCnt = MAX_DEVICE_CNT;
    ptPrivate->hModule = module;

    /* Register message handlers for device control */
    module_register_handler(module, 1, DeviceCtrlMessageHandler);  /* ON */
    module_register_handler(module, 2, DeviceCtrlMessageHandler);  /* OFF */

    /* Store private data in module */
    module->private_data = ptPrivate;

    dbprintf("[DeviceCtrl] Initialized with %d devices.\n", ptPrivate->uiDevCnt);
    return true;
}

void DeviceCtrlRun(ModuleHandle module)
{
    T_DeviceCtrl *ptPrivate = NULL;

    if (NULL == module)
    {
        return;
    }

    ptPrivate = (T_DeviceCtrl *)module->private_data;
    if (NULL == ptPrivate)
    {
        return;
    }

    /* Module run logic here - message handlers handle the work */
    /* This function is called periodically by the framework */
}

void DeviceCtrlDestroy(ModuleHandle module)
{
    T_DeviceCtrl *ptPrivate = NULL;

    if (NULL == module)
    {
        return;
    }

    ptPrivate = (T_DeviceCtrl *)module->private_data;
    if (NULL == ptPrivate)
    {
        return;
    }

    memset(ptPrivate, 0x0, sizeof(T_DeviceCtrl));
    free(ptPrivate);
    module->private_data = NULL;

    dbprintf("[DeviceCtrl] Deleted.\n");
}
