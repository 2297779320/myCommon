/**************************************************************************
*  Copyright (c) 2024
*  File Name: UserAgent.c
*  Description: 用户代理模块 - 使用 V2 框架，独立 STBP 客户端
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
#include "framework_v2.h"

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
*                    V2 Message Handlers                   *
**********************************************************/
/**
 * @brief 处理外部 STBP 消息并发布到内部框架
 */
static E_StateCode UserAgentExternalMsgHandler(
    void *pPrivate, T_FrameworkMsgV2 *ptMsg,
    char *pcResMsg, char **ppcResData, uint32_t *puiDataSize, bool *pbDelayRes)
{
    T_UserAgent *ptPrivate = (T_UserAgent *)pPrivate;

    (void)pcResMsg;
    (void)ppcResData;
    (void)puiDataSize;
    (void)pbDelayRes;

    if (NULL == ptPrivate || NULL == ptPrivate->hUser)
    {
        return STATE_CODE_INVALID_HANDLE;
    }

    dbprintf("[UserAgent] External message received: %s\n", ptMsg->strMsgId);
    
    /* 转发到 STBP */
    if (ptMsg->pcBody)
    {
        StbpClientPublish(ptPrivate->hUser, ptMsg->strMsgId, (char*)ptMsg->pcBody);
    }

    return STATE_CODE_NO_ERROR;
}

static T_MsgProcEntryV2 g_satUserAgentTable[] =
{
    {"$request.*.*.*.*.sample.*", UserAgentExternalMsgHandler, NULL, true, true},
    {"$report.*.*.*.*.sample.*",  UserAgentExternalMsgHandler, NULL, true, true},
    {NULL, NULL, NULL, false, false}
};

const T_MsgProcEntryV2* GetUserAgentMsgTable(void)
{
    return g_satUserAgentTable;
}

uint32_t GetUserAgentMsgTableLen(void)
{
    return 2;
}

/***********************************************************
*                    Module Functions                      *
**********************************************************/
bool UserAgentInit(ModuleHandleV2 module, void* config)
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

    /* Create independent STBP client */
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
        SysErr("Subscribe failed, %s.\n", strTopic);
        DeleteStbpClientObj(ptPrivate->hUser);
        free(ptPrivate);
        return false;
    }

    sprintf(strTopic, "$report.$data.*.*.*.%s", JSON_DEV_NAME);
    eCode = StbpClientSubscribe(ptPrivate->hUser, strTopic, NULL);
    if (!STATE_OK(eCode))
    {
        SysErr("Subscribe failed, %s.\n", strTopic);
        DeleteStbpClientObj(ptPrivate->hUser);
        free(ptPrivate);
        return false;
    }

    dbprintf("[UserAgent] Subscribed to topics (V2).\n");

    /* 将私有数据关联到模块 */
    module->private_data = ptPrivate;

    return true;
}

void UserAgentRun(ModuleHandleV2 module)
{
    T_UserAgent *ptPrivate = NULL;
    TlcStbpMsg_t *ptStbpMsg = NULL;

    if (NULL == module) return;

    ptPrivate = (T_UserAgent *)module->private_data;
    if (NULL == ptPrivate || NULL == ptPrivate->hUser) return;

    /* Wait for STBP connection */
    static BOOL bInit = FALSE;
    if (!bInit)
    {
        if (StbpClientConnected(ptPrivate->hUser))
        {
            StbpClientPublish(ptPrivate->hUser, TOPIC_SYS_READY, "{\"isReady\":true}");
            UserAgentOnStbpConnected(ptPrivate);
            bInit = TRUE;
            dbprintf("[UserAgent] Connected and ready (V2).\n");
        }
        return;
    }

    /* Receive STBP messages并转发到框架 */
    ptStbpMsg = StbpClientAllocMsg(ptPrivate->hUser);
    if (NULL == ptStbpMsg) return;

    dbprintf("[UserAgent] Received topic: %s\n", ptStbpMsg->topic);

    /* 发送到框架消息队列 */
    module_v2_send_message(
        module,
        0,  /* 广播 */
        ptStbpMsg->topic,
        NULL,
        0,
        ptStbpMsg->payloadSize,
        ptStbpMsg->pPayload,
        0,  /* 浅拷贝 */
        OSAL_TIMEOUT_NONE
    );

    StbpClientFreeMsg(ptPrivate->hUser, ptStbpMsg);
}

void UserAgentDestroy(ModuleHandleV2 module)
{
    T_UserAgent *ptPrivate = NULL;

    if (NULL == module) return;

    ptPrivate = (T_UserAgent *)module->private_data;
    if (NULL == ptPrivate) return;

    /* Delete independent STBP client */
    if (NULL != ptPrivate->hUser)
    {
        DeleteStbpClientObj(ptPrivate->hUser);
    }

    free(ptPrivate);
    module->private_data = NULL;

    dbprintf("[UserAgent] Deleted (V2).\n");
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
