/**************************************************************************
*  Copyright (c) 2024
*  File Name: UserAgentMsg.c
*  Description: User agent message handling
**************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "osal.h"
#include "global.h"
#include "SampleApp.h"
#include "StbpClient.h"
#include "JsonMsgDispatch.h"
#include "UserAgentPriv.h"
#include "framework_def.h"

/***********************************************************
*                    Message Handler                       *
**********************************************************/
static void UserAgentExternalMessageHandler(ModuleHandle module, const T_ModuleMsg* msg)
{
    T_UserAgent *ptPrivate = NULL;
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    INT8 *pcBody = NULL;

    if (NULL == module || NULL == msg)
    {
        return;
    }

    ptPrivate = (T_UserAgent *)module->private_data;
    if (NULL == ptPrivate || NULL == ptPrivate->hUser)
    {
        return;
    }

    dbprintf("[UserAgent] Publish %.*s, %.*s\n", 
             256, msg->data ? (const char*)msg->data : "(null)",
             256, msg->data ? (const char*)msg->data : "(null)");

    if (NULL == msg->data || msg->data_len == 0)
    {
        pcBody = "{}";
    }
    else
    {
        pcBody = (INT8 *)msg->data;
    }

    /* Publish to STBP */
    eCode = StbpClientPublish(ptPrivate->hUser, "$report.data", pcBody);

    if (!STATE_OK(eCode))
    {
        SysErr("Publish failed\n");
    }
}

/***********************************************************
*                    Module Integration                    *
**********************************************************/
/* This function is called during module initialization to register handlers */
void UserAgentRegisterMsgHandlers(ModuleHandle module)
{
    module_register_handler(module, 100, UserAgentExternalMessageHandler);
}
