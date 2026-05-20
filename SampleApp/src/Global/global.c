/**************************************************************************
*  Copyright (c) 2024
*  File Name: global.c
*  Description: Global initialization - STBP client creation
**************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "typedef.h"
#include "osal.h"
#include "global.h"
#include "SampleApp.h"
#include "StbpClient.h"

/***********************************************************
*                    Global State                          *
**********************************************************/
typedef struct {
    HANDLE  hUserClient;
    BOOL    bInitialized;
} T_GlobalMnt;

static T_GlobalMnt g_stGlobalMnt = {0};

/***********************************************************
*                    Global Functions                      *
**********************************************************/
E_StateCode GlobalInit(void)
{
    INT32 iStbpSrvPort = STBP_DEFAULT_PORT;
    INT32 i;

    memset(&g_stGlobalMnt, 0x0, sizeof(g_stGlobalMnt));

    /* Create global STBP client */
    g_stGlobalMnt.hUserClient = CreateStbpClientObj2(
        STBP_CONNECT_IP, iStbpSrvPort, "SampleGlobal", NULL, NULL, TRUE);
    if (NULL == g_stGlobalMnt.hUserClient)
    {
        SysErr("CreateStbpClientObj2 failed!\n");
        return STATE_CODE_ALLOCATION_FAILURE;
    }

    /* Wait for STBP connection */
    dbprintf("Waiting for STBP connection...\n");
    for (i = 0; i < 100; i++)
    {
        if (StbpClientConnected(g_stGlobalMnt.hUserClient))
        {
            break;
        }
        OSAL_Sleep(10);
    }

    if (StbpClientConnected(g_stGlobalMnt.hUserClient))
    {
        dbprintf("STBP connected.\n");
    }
    else
    {
        SysErr("STBP connection timeout (running without network).\n");
        /* Continue anyway for demo purposes */
    }

    g_stGlobalMnt.bInitialized = TRUE;
    return STATE_CODE_NO_ERROR;
}

void GlobalDestroy(void)
{
    if (NULL != g_stGlobalMnt.hUserClient)
    {
        DeleteStbpClientObj(g_stGlobalMnt.hUserClient);
        g_stGlobalMnt.hUserClient = NULL;
    }
    g_stGlobalMnt.bInitialized = FALSE;
    dbprintf("Global destroyed.\n");
}

void *GetUserClientHandle(void)
{
    return g_stGlobalMnt.hUserClient;
}

INT32 GetStbpServerPort(void)
{
    return STBP_DEFAULT_PORT;
}
