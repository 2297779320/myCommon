/**************************************************************************
*  Copyright (c) 2024
*  File Name: UserAgent.c
*  Description: User agent module - independent STBP client with identity isolation
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
#include "UserAgentPriv.h"
#include "framework_def.h"

/***********************************************************
*                    Internal Functions                    *
**********************************************************/
static E_StateCode UserAgentOnStbpConnected(T_UserAgent *ptPrivate)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;

    /* Publish initial device info */
    StbpClientPublish(ptPrivate->hUser, JSON_DEV_NAME, NULL);
    StbpClientPublish(ptPrivate->hUser, JSON_LOCAL_IPADDR, NULL);
    StbpClientPublish(ptPrivate->hUser, JSON_SERVER_INFO, NULL);

    dbprintf("[UserAgent] Initial reports published.\n");
    return eCode;
}

/***********************************************************
*                    Message Handlers                      *
**********************************************************/
static void UserAgentMessageHandler(ModuleHandle module, const T_ModuleMsg* msg)
{
    T_UserAgent *ptPrivate = NULL;

    if (NULL == module || NULL == msg)
    {
        return;
    }

    ptPrivate = (T_UserAgent *)module->private_data;
    if (NULL == ptPrivate || NULL == ptPrivate->hUser)
    {
        return;
    }

    dbprintf("[UserAgent] Received internal message type: %u\n", msg->type);
    /* Process internal framework messages here */
}

/***********************************************************
*                    Module Functions                      *
**********************************************************/
bool UserAgentInit(ModuleHandle module, void* config)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    T_UserAgent *ptPrivate = NULL;
    String256 strTopic = {0};
    INT32 iStbpSrvPort = STBP_DEFAULT_PORT;

    (void)config;

    /* Allocate module private data */
    ptPrivate = (T_UserAgent *)malloc(sizeof(T_UserAgent));
    if (NULL == ptPrivate)
    {
        SysErr("malloc failed!\n");
        return false;
    }
    memset(ptPrivate, 0x0, sizeof(T_UserAgent));

    /* Create independent STBP client - identity isolation from Global */
    dbprintf("[UserAgent] STBP server port is %d\n", iStbpSrvPort);
    ptPrivate->hUser = CreateStbpClientObj2(
        STBP_CONNECT_IP, iStbpSrvPort, "SampleUserAgent", NULL, NULL, TRUE);
    if (NULL == ptPrivate->hUser)
    {
        SysErr("CreateStbpClientObj2 failed!\n");
        free(ptPrivate);
        return false;
    }

    /* Subscribe to topics */
    sprintf(strTopic, "$request.*.*.*.*.%s.*.>", "sample");
    eCode = StbpClientSubscribe(ptPrivate->hUser, strTopic, NULL);
    if (!STATE_OK(eCode))
    {
        SysErr("UserClientSubscribe failed, %s.\n", strTopic);
        DeleteStbpClientObj(ptPrivate->hUser);
        free(ptPrivate);
        return false;
    }

    sprintf(strTopic, "$report.$data.*.*.*.%s", JSON_DEV_NAME);
    eCode = StbpClientSubscribe(ptPrivate->hUser, strTopic, NULL);
    if (!STATE_OK(eCode))
    {
        SysErr("UserClientSubscribe failed, %s.\n", strTopic);
        DeleteStbpClientObj(ptPrivate->hUser);
        free(ptPrivate);
        return false;
    }

    sprintf(strTopic, "$report.heartbeat.*.*.*.%s.v1.state", "sample");
    eCode = StbpClientSubscribe(ptPrivate->hUser, strTopic, NULL);
    if (!STATE_OK(eCode))
    {
        SysErr("UserClientSubscribe failed, %s.\n", strTopic);
        DeleteStbpClientObj(ptPrivate->hUser);
        free(ptPrivate);
        return false;
    }

    dbprintf("[UserAgent] Subscribed to topics.\n");

    /* Register message handler */
    module_register_handler(module, 0, UserAgentMessageHandler);

    /* Store private data in module */
    module->private_data = ptPrivate;

    return true;
}

void UserAgentRun(ModuleHandle module)
{
    T_UserAgent *ptPrivate = NULL;
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    TlcStbpMsg_t *ptStbpMsg = NULL;
    T_ModuleMsg tFwMsg;

    if (NULL == module)
    {
        return;
    }

    ptPrivate = (T_UserAgent *)module->private_data;
    if (NULL == ptPrivate)
    {
        return;
    }

    /* Wait for STBP connection and publish ready message */
    static BOOL bInit = FALSE;
    if (!bInit)
    {
        if (StbpClientConnected(ptPrivate->hUser))
        {
            StbpClientPublish(ptPrivate->hUser, TOPIC_SYS_READY, "{\"isReady\":true}");
            eCode = UserAgentOnStbpConnected(ptPrivate);
            if (STATE_OK(eCode))
            {
                bInit = TRUE;
                dbprintf("[UserAgent] Initialized and connected.\n");
            }
        }
        return;
    }

    /* Receive STBP messages */
    ptStbpMsg = StbpClientAllocMsg(ptPrivate->hUser);
    if (NULL == ptStbpMsg)
    {
        return;
    }

    dbprintf("[UserAgent] Received topic: %s\n", ptStbpMsg->topic);

    /* Route message to module dispatcher via framework message queue */
    memset(&tFwMsg, 0x0, sizeof(tFwMsg));
    tFwMsg.sender = module->id;
    tFwMsg.receiver = 0;  /* Broadcast */
    tFwMsg.type = 0;  /* UserAgent message type */
    tFwMsg.data_len = ptStbpMsg->payloadSize;
    tFwMsg.data = (void*)ptStbpMsg->pPayload;
    tFwMsg.copy_type = MSG_COPY_SHALLOW;

    /* Send message to framework queue for processing */
    module_send_message(module, 0, 0, 0, ptStbpMsg->payloadSize, 
                       ptStbpMsg->pPayload, MSG_COPY_SHALLOW, OSAL_TIMEOUT_NONE);

    StbpClientFreeMsg(ptPrivate->hUser, ptStbpMsg);
}

void UserAgentDestroy(ModuleHandle module)
{
    T_UserAgent *ptPrivate = NULL;

    if (NULL == module)
    {
        return;
    }

    ptPrivate = (T_UserAgent *)module->private_data;
    if (NULL == ptPrivate)
    {
        return;
    }

    /* Delete independent STBP client */
    if (NULL != ptPrivate->hUser)
    {
        DeleteStbpClientObj(ptPrivate->hUser);
    }

    free(ptPrivate);
    module->private_data = NULL;

    dbprintf("[UserAgent] Deleted.\n");
}

int UserAgentCmpMsgId(const char *strFmt, const char *strTopic)
{
    BOOL bEqual = FALSE;
    bEqual = IsTopicEqual((INT8 *)strFmt, (INT8 *)strTopic);
    if (!bEqual)
    {
        bEqual = IsTopicEqual((INT8 *)strTopic, (INT8 *)strFmt);
    }
    return bEqual ? 0 : 1;
}
