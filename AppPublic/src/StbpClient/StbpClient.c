/**************************************************************************
* 版    权：Copyright (c) 2019
* 文件名称：StbpClient.c
* 文件标识： 
* 内容摘要： 控制接口封装
* 其它说明：
* 当前版本： 
* 作    者： 
* 完成日期：2019年01月 23日
*
* 修改记录1	：
*	修改日期：
*	版 本 号：
*	修 改 人：
*	修改内容：
**************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "TlPublic.h"

#include "typedef.h"
//#include "osal.h"
#include "osal_mem.h"
#include "osal_mutex.h"
//#include "doublelink.h"
#include "CommQue.h"
//#include "DCSocket.h"

#include "StbpClient.h"

/***********************************************************
 *						常量定义		                       		*
 **********************************************************/
#define STBP_CLIENT_TIMEOUT			5000

#define STBP_CLIENT_MSG_QUE_LEN		1000

/***********************************************************
 *				文件内部使用的宏                      *
 **********************************************************/

 /***********************************************************
 *			文件内部使用的数据类型 	*
 **********************************************************/
typedef struct
{
	tlHdl_t		hStbpConn;
	String32		strIp;
	UINT16		uwPort;
	String128   strName;
	T_MutexObj	tMutex;
	T_MutexObj	tStateMutex;

	BOOL		bConnected;
	BOOL		bNeedSync;
	BOOL		bFirst;

	BOOL		bSubscribe;
	CommQueID	hJsonMsgQue;

	StbpClientCb	pfCb;
	void			*pCbCtx;
}T_StbpClientObj;

/***********************************************************
 *						全局变量						*
 **********************************************************/

/***********************************************************
 *						本地变量						*
 **********************************************************/
static tlHdl_t	 g_shLoop = NULL;

static TlKeyInfo_t g_stStbpResultKey[] =
{ 
	TL_MAKE_OPT_KEY_INFO(TL_KEY_OptIgnor, T_StbpResult, TL_KEY_TYPE_U32, "code", iCode, NULL),
	TL_MAKE_OPT_KEY_INFO(TL_KEY_OptIgnor, T_StbpResult, TL_KEY_TYPE_StringPtr, "msg", pcMsg, NULL),
	//TL_MAKE_OPT_KEY_INFO(TL_KEY_OptIgnor, T_StbpResult, TL_KEY_TYPE_StringPtr, "data", pcData, NULL),
	
	TL_MAKE_END_INFO()
};

static TlKeyInfo_t g_stStbpResultCodeKey[] =
{ 
	TL_MAKE_OPT_KEY_INFO(TL_KEY_OptIgnor, T_StbpResult, TL_KEY_TYPE_U32, "code", iCode, NULL),
		
	TL_MAKE_END_INFO()
};


/***********************************************************
 *						本地函数					*
 **********************************************************/
static void StbpClientStateChange(tlHdl_t connHdl, 
                TlcConnState_e preState,
                TlcConnState_e currState,
                void *pUsrCtx)
{
	T_StbpClientObj *ptObj = pUsrCtx;
	
	dbprintf("connHdl[%p] [%s]->[%s]\n", 
		connHdl, 
		TlcConnState2Str(preState), 
		TlcConnState2Str(currState));
	if (NULL == ptObj)
	{
		return;
	}

	OSAL_MutexLock(&ptObj->tStateMutex);
	if (TLC_CONN_STATE_Connected == currState)
	{
		ptObj->bConnected = SMP_TRUE;
		if (ptObj->bFirst)
		{
			ptObj->bFirst = SMP_FALSE;
			ptObj->bNeedSync = SMP_TRUE;
		}
		else
		{
			ptObj->bNeedSync = SMP_TRUE;
		}

		//dbprintf("set sync flag of %s\n", ptObj->strIp);
	}
	else
	{
		ptObj->bConnected = SMP_FALSE;
	}
	OSAL_MutexUnlock(&ptObj->tStateMutex);
}

static E_StateCode StbpClientCreateStbp(T_StbpClientObj *ptObj, tlBool bReconnect)
{
    E_StateCode     eCode = STATE_CODE_NO_ERROR;
    INT32           iRet = 0;
    
    TlcStbpcCfg_t cfg = 
    {
        .port = 18300,
        .pIpaddr = NULL,
        .pName = NULL,
        .pDomainId = NULL,
        .pDevId = NULL,
        .pUsername = NULL,
        .pPassword = NULL,
        .maxWtBufSize = 8 * 1024 * 1024,
        .allowReconnect = TRUE,
        .overWebsocket = FALSE,
        .pUsrCtx = NULL,
        .fStateChange = StbpClientStateChange,   
    };

    cfg.pIpaddr = ptObj->strIp;
    cfg.port = ptObj->uwPort;
    cfg.pUsrCtx = ptObj;
    cfg.allowReconnect = bReconnect;
    
    dbprintf("StbpClientCreateStbp: %s, %d, %d\n", ptObj->strIp, ptObj->uwPort, cfg.allowReconnect);

#if 0
    /* test*/
    if (0 == strcmp(ptObj->strIp, "127.0.0.1"))
    {
        cfg.pIpaddr = "192.168.30.104";
    }
    
    cfg.pUsername = "service";
    cfg.pPassword = "service";
#else
    cfg.pUsername = getenv("TZSP_STBP_SERVICE_USER");
    cfg.pPassword = getenv("TZSP_STBP_SERVICE_PASS");
    if (NULL == cfg.pUsername)
    {
        cfg.pUsername = "service";
        cfg.pPassword = "service";
    }
    dbprintf("user is %s, pwd is %s\n", cfg.pUsername, cfg.pPassword);
#endif

    iRet = TlcMgeStbpcCreate(g_shLoop, &cfg, &ptObj->hStbpConn);
    if (0 != iRet)
    {
        SysErr("TlcMgeStbpcCreate failed, iRet = %d\n", iRet);
        eCode = STATE_CODE_SOCKET_CREATE_FAILURE;
        return eCode;
    }

    iRet = TlcMgeStbpcConnect(ptObj->hStbpConn, 0);
    if (0 != iRet)
    {
        SysErr("TlcMgeStbpcConnect failed, iRet = %d\n", iRet);
        eCode = STATE_CODE_SOCKET_CREATE_FAILURE;
        return eCode;
    }

    return eCode;
}

static void StbpClientDecoderDeleteStbp(T_StbpClientObj *ptObj)
{
	INT32	iRet = 0;
	if (NULL != ptObj->hStbpConn)
	{
		iRet = TlcMgeStbpcDisconnect(ptObj->hStbpConn, 3000);
		if (iRet != 0)
		{
			SysErr("TlcMgeStbpcDisconnect failed, iRet = %d\n", iRet);
		}
		
		TlcMgeStbpcDestroy(ptObj->hStbpConn);
		ptObj->hStbpConn = NULL;
	}
}

static void StbpcMsgAsyncCb(void *pUsrCtx, 
                    const char *pTopic, 		/* 发出请求的TOPIC*/
                    int callbackId, 
                    int result, 
                    TlcStbpMsg_t *pMsg, 
                    tlBool *pIsKeepMsg)
{
	T_StbpClientObj		*ptObj = pUsrCtx;
	INT8					*strResJson = NULL;
	T_StbpResult			tResult;
	
	if (NULL == ptObj)
	{
		return;
	}

	if (NULL == pTopic)
	{
		SysErr("The topic is null!!!\n");
		return;
	}
	
	if (NULL != ptObj->pfCb)
	{
		if (NULL != pMsg)
		{
			strResJson = pMsg->pPayload;
		}

		if (0 == result)
		{
			TlpJsonString2Obj(g_stStbpResultCodeKey, strResJson, &tResult);
			result = tResult.iCode;
		}

		ptObj->pfCb(ptObj->pCbCtx, callbackId, (INT8 *)pTopic, result, strResJson);
	}

	if (NULL != pIsKeepMsg)
	{
		*pIsKeepMsg = SMP_FALSE;
	}
}

void StbpClientSubscribeCb(tlHdl_t connHdl, 
                    void *pUsrCtx, 
                    TlcStbpMsg_t *pMsg, 
                    tlBool *pIsKeepMsg)
{
	T_StbpClientObj		*ptObj = pUsrCtx;
	TlcStbpMsg_t			**ptTx = NULL;
	
	if (NULL == ptObj)
	{
		return;
	}

	ptTx = (TlcStbpMsg_t **)CommQue_GetEmpty(ptObj->hJsonMsgQue, OSAL_TIMEOUT_NONE);
	if (NULL == ptTx)
	{
		SysErr("StbpClientSubscribeCb::CommQue_GetEmpty failed! %p %s\n", ptObj, ptObj->strName);
		*pIsKeepMsg = SMP_FALSE;
		return;
	}

	*ptTx = pMsg;

	CommQue_PutFull(ptObj->hJsonMsgQue, (TlcStbpMsg_t **)ptTx);
	
	if (NULL != pIsKeepMsg)
	{
		*pIsKeepMsg = SMP_TRUE;
	}
}

static void StbpClientFreeQue(T_StbpClientObj *ptObj)
{
	TlcStbpMsg_t		**pptUserMsg = NULL;
	TlcStbpMsg_t		*ptMsg = NULL;

	if (NULL == ptObj)
	{
		return;
	}

	for (;;)
	{
		pptUserMsg = (TlcStbpMsg_t **)CommQue_GetFull(ptObj->hJsonMsgQue, OSAL_TIMEOUT_NONE);
		if (NULL == pptUserMsg)
		{
			return;
		}

		ptMsg = *pptUserMsg;

		TlcMgeStbpcFreeMsg(ptMsg);
		
		CommQue_PutEmpty(ptObj->hJsonMsgQue, (void *)pptUserMsg);
	}
}

/***********************************************************
 * 						全局函数						*
 **********************************************************/
/**********************************************************************
* 函数名称：StbpClientInit
* 功能描述：控制模块初始化
* 输入参数：无
* 输出参数：无
* 返 回 值：	E_StateCode
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
E_StateCode StbpClientInit()
{
	INT32	iRet = 0;
	
	if (NULL == g_shLoop)
	{
		iRet = TlcMgeLoopCreate(&g_shLoop);
		if (0 != iRet)
		{
			SysErr("TlcMgeLoopCreate failed, iRet = %d\n", iRet);
			return STATE_CODE_ALLOCATION_FAILURE;
		}

		 iRet =  TlcMgeLoopStart(g_shLoop, "shLoop");
		 if (0 != iRet)
	 	{
			SysErr("TlcMgeLoopCreate failed, iRet = %d\n", iRet);
			TlcMgeLoopDestroy(g_shLoop);
			return STATE_CODE_INIT_FAILURE;
		}
	}
	
	return STATE_CODE_NO_ERROR;
}

/**********************************************************************
* 函数名称：StbpClientDestroy
* 功能描述：控制模块销毁
* 输入参数：无
* 输出参数：无
* 返 回 值：	E_StateCode
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
void StbpClientDestroy()
{	
	if (NULL != g_shLoop)
	{
		TlcMgeLoopStop(g_shLoop);
		TlcMgeLoopDestroy(g_shLoop);
		g_shLoop = NULL;
	}
	
	return;
}

E_StateCode ReconnectStbpClient(HANDLE hUser)
{
	T_StbpClientObj		*ptObj = hUser;
	E_StateCode		eCode = STATE_CODE_NO_ERROR;
	INT32			iRet = 0;
    if (NULL == ptObj)
    {
        return STATE_CODE_INVALID_HANDLE;
    }
    iRet = TlcMgeStbpcConnect(ptObj->hStbpConn, 0);
    if (0 != iRet)
    {
        SysErr("TlcMgeStbpcConnect failed, iRet = %d\n", iRet);
        eCode = STATE_CODE_SOCKET_CREATE_FAILURE;
        return eCode;
    }
    return STATE_CODE_NO_ERROR;
}

/**********************************************************************
* 函数名称：CreateStbpClientObj
* 功能描述：创建控制客户端
* 输入参数：strIp - IP地址
					uwPort - 端口
					pfCb - 异步消息发送的回调函数, 可为空
					pCbCtx - 回调上下文, 可为空
					bSubscibe - 是否允许订阅消息
* 输出参数：无
* 返 回 值：	 HANDLE
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
HANDLE CreateStbpClientObj1(INT8 *strIp, UINT16 uwPort, StbpClientCb pfCb, void *pCbCtx, BOOL bSubscibe, BOOL bReconnect)
{
	T_StbpClientObj		*ptObj = NULL;
	E_StateCode		eCode = STATE_CODE_NO_ERROR;
	INT32			iRet = 0;
	
	if (NULL == strIp)
	{
		return NULL;
	}

	if (NULL == g_shLoop)
	{
		iRet = TlcMgeLoopCreate(&g_shLoop);
		if (0 != iRet)
		{
			SysErr("TlcMgeLoopCreate failed, iRet = %d\n", iRet);
			return NULL;
		}

		 iRet =  TlcMgeLoopStart(g_shLoop, "shLoop");
		 if (0 != iRet)
	 	{
			SysErr("TlcMgeLoopCreate failed, iRet = %d\n", iRet);
			TlcMgeLoopDestroy(g_shLoop);
			return NULL;
		}
	}
	
	NewMemory(ptObj, T_StbpClientObj, 1, OnFail(return NULL));
	
	OSAL_MutexInit(&ptObj->tMutex);
	OSAL_MutexInit(&ptObj->tStateMutex);

	ptObj->bFirst = SMP_TRUE;

	strcpy(ptObj->strIp, strIp);
	ptObj->uwPort = uwPort;
	strcpy(ptObj->strName, "shLoop");
    eCode = StbpClientCreateStbp(ptObj, bReconnect);
	if (!STATE_OK(eCode))
	{
		SysErr("StbpClientCreateStbp failed, eCode = %d\n", eCode);
		goto cleanup;
	}

	ptObj->pfCb = pfCb;
	ptObj->pCbCtx = pCbCtx;
	ptObj->bSubscribe = bSubscibe;

	if (bSubscibe)
	{
		ptObj->hJsonMsgQue = CommQue_Create(STBP_CLIENT_MSG_QUE_LEN, 
											sizeof(TlcStbpMsg_t **), 
											NULL);
		if (NULL == ptObj->hJsonMsgQue)
		{
			SysErr("CommQue_Create hJsonMsgQue failed!\n");
			eCode = STATE_CODE_ALLOCATION_FAILURE;
			goto cleanup;
		}
	}
cleanup:
	if (!STATE_OK(eCode))
	{
		DeleteStbpClientObj(ptObj);
		ptObj = NULL;
	}
	return ptObj;
}

/**********************************************************************
* 函数名称：CreateStbpClientObj
* 功能描述：创建控制客户端
* 输入参数：strIp - IP地址
					uwPort - 端口
					pfCb - 异步消息发送的回调函数, 可为空
					pCbCtx - 回调上下文, 可为空
					bSubscibe - 是否允许订阅消息
* 输出参数：无
* 返 回 值：	 HANDLE
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
HANDLE CreateStbpClientObj(INT8 *strIp, UINT16 uwPort, StbpClientCb pfCb, void *pCbCtx, BOOL bSubscibe)
{
    return CreateStbpClientObj1(strIp, uwPort, pfCb, pCbCtx, bSubscibe, TRUE);
}

/**********************************************************************
* 函数名称：CreateStbpClientObj2
* 功能描述：创建控制客户端2
* 输入参数：strIp - IP地址
					uwPort - 端口
					pfCb - 异步消息发送的回调函数, 可为空
					pCbCtx - 回调上下文, 可为空
					bSubscibe - 是否允许订阅消息
* 输出参数：无
* 返 回 值：	 HANDLE
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
HANDLE CreateStbpClientObj2(INT8 *strIp, UINT16 uwPort, INT8 *strName, StbpClientCb pfCb, void *pCbCtx, BOOL bSubscibe)
{
	T_StbpClientObj		*ptObj = NULL;
	E_StateCode		eCode = STATE_CODE_NO_ERROR;
	INT32			iRet = 0;
	
	if ((NULL == strIp) || (NULL == strName))
	{
		return NULL;
	}

	if (NULL == g_shLoop)
	{
		iRet = TlcMgeLoopCreate(&g_shLoop);
		if (0 != iRet)
		{
			SysErr("TlcMgeLoopCreate failed, iRet = %d\n", iRet);
			return NULL;
		}

		 iRet =  TlcMgeLoopStart(g_shLoop, strName);
		 if (0 != iRet)
	 	{
			SysErr("TlcMgeLoopCreate failed, iRet = %d\n", iRet);
			TlcMgeLoopDestroy(g_shLoop);
			return NULL;
		}
	}
	
	NewMemory(ptObj, T_StbpClientObj, 1, OnFail(return NULL));
	
	OSAL_MutexInit(&ptObj->tMutex);
	OSAL_MutexInit(&ptObj->tStateMutex);

	ptObj->bFirst = SMP_TRUE;

	strcpy(ptObj->strIp, strIp);
	ptObj->uwPort = uwPort;
	strcpy(ptObj->strName, strName);
	eCode = StbpClientCreateStbp(ptObj, TRUE);
	if (!STATE_OK(eCode))
	{
		SysErr("StbpClientCreateStbp failed, eCode = %d\n", eCode);
		goto cleanup;
	}

	ptObj->pfCb = pfCb;
	ptObj->pCbCtx = pCbCtx;
	ptObj->bSubscribe = bSubscibe;

	if (bSubscibe)
	{
		ptObj->hJsonMsgQue = CommQue_Create(STBP_CLIENT_MSG_QUE_LEN, 
											sizeof(TlcStbpMsg_t **), 
											NULL);
		if (NULL == ptObj->hJsonMsgQue)
		{
			SysErr("CommQue_Create hJsonMsgQue failed!\n");
			eCode = STATE_CODE_ALLOCATION_FAILURE;
			goto cleanup;
		}
	}
cleanup:
	if (!STATE_OK(eCode))
	{
		DeleteStbpClientObj(ptObj);
		ptObj = NULL;
	}
	return ptObj;
}

/**********************************************************************
* 函数名称：DeleteStbpClientObj
* 功能描述：删除控制客户端管理句柄
* 输入参数：hUser
* 输出参数：无
* 返 回 值：	 无
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
void DeleteStbpClientObj(HANDLE hUser)
{
	T_StbpClientObj		*ptObj = hUser;
	
	if (NULL == ptObj)
	{
		return;
	}

	StbpClientDecoderDeleteStbp(ptObj);
	
	if (NULL != ptObj->hJsonMsgQue)
	{
		StbpClientFreeQue(ptObj);
		CommQue_Delete(ptObj->hJsonMsgQue);
	}
	OSAL_MutexDestroy(&ptObj->tStateMutex);
	OSAL_MutexDestroy(&ptObj->tMutex);
	free(ptObj);
	return;
}

/**********************************************************************
* 函数名称：StbpClientSendMsg
* 功能描述：发送命令
* 输入参数：hUser
					strTopic - 主题
					strJsonMsg - 消息体
					uiTimeOut - 超时
* 输出参数：ppcRes - 返回消息，可为空
* 返 回 值：	E_StateCode
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
E_StateCode StbpClientSendMsg(HANDLE hUser, INT8 *strTopic, INT8 *strJsonMsg, UINT32 uiTimeOut, INT8 **ppcRes)
{
	T_StbpClientObj	*ptObj = hUser;
	E_StateCode		eCode = STATE_CODE_NO_ERROR;
	INT32			iRet = 0;
	//String256			strTopic;
	TlcStbpMsg_t		*ptJsonResMsg = NULL;
	INT32			iLen = 0;

	if ((NULL == hUser) || (NULL == strTopic))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	if (NULL == strJsonMsg)
	{
		iLen = 0;
	}
	else
	{
		iLen = strlen(strJsonMsg);
	}
	
	if (0 == uiTimeOut)
	{
		uiTimeOut = 10;
	}
	
	iRet = TlcMgeStbpcRequest(ptObj->hStbpConn, 
							strTopic, 
							strJsonMsg, 
							iLen, 
							uiTimeOut, 
							&ptJsonResMsg);
	if (0 != iRet)
	{
		SysErr("TlcMgeStbpcRequest failed, strTopic = %s, iRet = %d\n", strTopic, iRet);
		eCode = STATE_CODE_UNDEFINED_ERROR;

		goto cleanup;
	}

	if (NULL != ppcRes)
	{
		if (NULL != ptJsonResMsg->pPayload)
		{
			*ppcRes = malloc(ptJsonResMsg->payloadSize + 1);
			memcpy(*ppcRes, ptJsonResMsg->pPayload, ptJsonResMsg->payloadSize);
			*(*ppcRes + ptJsonResMsg->payloadSize) = '\0';
		}
	}

	TlcMgeStbpcFreeMsg(ptJsonResMsg);

cleanup:
	return eCode;
}

/**********************************************************************
* 函数名称：StbpClientSendMsgAsync
* 功能描述：异步发送命令
* 输入参数：hUser
					strTopic - 主题
					strJsonMsg - 消息体
					uiTimeOut - 超时，如为0，采用异步发送
* 输出参数：puiCallId - 调用ID
* 返 回 值：	E_StateCode
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
E_StateCode StbpClientSendMsgAsync(HANDLE hUser, INT8 *strTopic, INT8 *strJsonMsg, UINT32 *puiCallId)
{
	T_StbpClientObj	*ptObj = hUser;
	E_StateCode		eCode = STATE_CODE_NO_ERROR;
	INT32			iRet = 0;
	INT32			iCallId = 0;
	INT32			iLen = 0;

	if ((NULL == hUser) || (NULL == strTopic))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	if (NULL == strJsonMsg)
	{
		iLen = 0;
	}
	else
	{
		iLen = strlen(strJsonMsg);
	}

	if (NULL != puiCallId)
	{
		iRet = TlcMgeStbpcRequestAsync(ptObj->hStbpConn, 
									ptObj, 
									StbpcMsgAsyncCb, 
									NULL, 
									strTopic, 
									strJsonMsg, 
									iLen, 
									STBP_CLIENT_TIMEOUT, 
									&iCallId);
		*puiCallId = iCallId;
	}
	else
	{
		iRet = TlcMgeStbpcPublish(ptObj->hStbpConn, 
						(const char *)strTopic, 
						(const void *)strJsonMsg,
						iLen);
	}
	
	if (0 != iRet)
	{
		SysErr("TlcMgeStbpcRequestAsync failed, iRet = %d\n", iRet);
		eCode = STATE_CODE_UNDEFINED_ERROR;
		goto cleanup;
	}

cleanup:
	return eCode;
}

/**********************************************************************
* 函数名称：StbpClientPublish
* 功能描述：发布消息
* 输入参数：hUser
					strTopic - 订阅的主题
					strJsonMsg - JSON消息,可为空
* 输出参数：无
* 返 回 值：	 E_StateCode
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
E_StateCode StbpClientPublish(HANDLE hUser, const INT8 *strTopic, INT8 *strJsonMsg)
{
	T_StbpClientObj	*ptObj = hUser;
	E_StateCode		eCode = STATE_CODE_NO_ERROR;
	INT32			iRet = 0;
	INT32			iLen = 0;

	if ((NULL == hUser) || (NULL == strTopic))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	if (NULL == strJsonMsg)
	{
		iLen = 0;
	}
	else
	{
		iLen = strlen(strJsonMsg);
	}
	
	iRet = TlcMgeStbpcPublish(ptObj->hStbpConn, 
						(const char *)strTopic, 
						(const void *)strJsonMsg,
						iLen);
	if (0 != iRet)
	{
		SysErr("StbpClientPublish failed, iRet = %d, to %s\n%s\n", iRet, ptObj->strIp, strTopic);
		return STATE_CODE_INVALID_PARAM;
	}

	return eCode;
}

/**********************************************************************
* 函数名称：StbpClientPublishWithRetCode
* 功能描述：带状态码发布消息
* 输入参数：hUser
					strTopic - 订阅的主题
					strDataInfo - JSON数据,可为空
					iRetCode - 返回码
					strRetInfo - 返回信息
* 输出参数：无
* 返 回 值：	 E_StateCode
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
E_StateCode StbpClientPublishWithRetCode(HANDLE hUser, const INT8 *strTopic, INT8 *strDataInfo, INT32 iRetCode, INT8 *strRetInfo)
{
	T_StbpClientObj	*ptObj = hUser;
	E_StateCode		eCode = STATE_CODE_NO_ERROR;
	INT32			iRet = 0;
	INT32			iLen = 0;
	INT32			iPrintLen = 0;
	INT32			iCurLen = 0;
	INT8				*pcResponse = NULL;

	if ((NULL == hUser) || (NULL == strTopic))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	if (0 == strTopic[0])
	{
		return STATE_CODE_NO_ERROR;
	}

	if (NULL == strDataInfo)
	{
		iLen = 0;
	}
	else
	{
		iLen = strlen(strDataInfo);
	}

	if (NULL != strRetInfo)
	{
		iLen += strlen(strRetInfo);
	}

	iLen += 256;

	pcResponse = malloc(iLen);
	if (NULL == pcResponse)
	{
		SysErr("malloc response buffer failed!\n");
		return STATE_CODE_ALLOCATION_FAILURE;
	}

	iPrintLen = sprintf(pcResponse, "{\"code\":%d", iRetCode);
	iCurLen += iPrintLen;
	if (NULL != strRetInfo)
	{
		iPrintLen = sprintf(pcResponse + iCurLen, ",\"msg\":\"%s\"", strRetInfo);
		iCurLen += iPrintLen;
	}

	if (NULL != strDataInfo)
	{
		iPrintLen = sprintf(pcResponse + iCurLen, ",\"data\":%s", strDataInfo);
		iCurLen += iPrintLen;
	}

	strcat(pcResponse, "}");
	iCurLen++;

	//dbprintf("pcResponse is %s\n", pcResponse);
	iRet = TlcMgeStbpcPublish(ptObj->hStbpConn, 
						(const char *)strTopic, 
						(const void *)pcResponse,
						iCurLen);
	//dbprintf("TlcMgeStbpcPublish finished.\n", pcResponse);

	free(pcResponse);
	
	if (0 != iRet)
	{
		SysErr("StbpClientPublishWithRetCode failed, iRet = %d, %s\n", iRet, strTopic);
		return STATE_CODE_INVALID_PARAM;
	}

	return eCode;
}

/**********************************************************************
* 函数名称：StbpClientSubscribe
* 功能描述：订阅消息
* 输入参数：hUser
					strTopic - 订阅的主题，支持通配操作
* 输出参数：puiSubscribeId - 订阅ID
* 返 回 值：	 E_StateCode
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
E_StateCode StbpClientSubscribe(HANDLE hUser, const INT8 *strTopic, UINT32 *puiSubscribeId)
{
	T_StbpClientObj	*ptObj = hUser;
	E_StateCode		eCode = STATE_CODE_NO_ERROR;
	INT32			iRet = 0;
	INT32			iSid = 0;

	if ((NULL == hUser) || (NULL == strTopic))
	{
		return STATE_CODE_INVALID_HANDLE;
	}
	
	iRet = TlcMgeStbpcSubscribe(ptObj->hStbpConn, ptObj, StbpClientSubscribeCb, (const char *)strTopic, &iSid);
	if (0 != iRet)
	{
		SysErr("TlcMgeStbpcSubscribe failed, iRet = %d, %s\n", iRet, strTopic);
		return STATE_CODE_INVALID_PARAM;
	}

	if (NULL != puiSubscribeId)
	{
		*puiSubscribeId = iSid;
	}
	return eCode;
}

/**********************************************************************
* 函数名称：StbpClientUnSubscribe
* 功能描述：退订阅
* 输入参数：hUser
					uiSubscribeId - 订阅ID
* 输出参数：无
* 返 回 值：	 E_StateCode
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
E_StateCode StbpClientUnSubscribe(HANDLE hUser, UINT32 uiSubscribeId)
{
	T_StbpClientObj	*ptObj = hUser;
	E_StateCode		eCode = STATE_CODE_NO_ERROR;

	if (NULL == hUser)
	{
		return STATE_CODE_INVALID_HANDLE;
	}
	
	TlcMgeStbpcUnsubscribe(ptObj->hStbpConn, uiSubscribeId);
	
	return eCode;
}

/**********************************************************************
* 函数名称：StbpClientAllocMsg
* 功能描述：分配消息
* 输入参数：hUser
* 输出参数：无
* 返 回 值：	 TlcStbpMsg_t *
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
TlcStbpMsg_t *StbpClientAllocMsg(HANDLE hUser)
{
	T_StbpClientObj	*ptObj = hUser;
	TlcStbpMsg_t		**pptUserMsg = NULL;
	TlcStbpMsg_t		*ptMsg = NULL;

	if (NULL == hUser)
	{
		return NULL;
	}
	
	pptUserMsg = (TlcStbpMsg_t **)CommQue_GetFull(ptObj->hJsonMsgQue, OSAL_TIMEOUT_NONE);
	if (NULL == pptUserMsg)
	{
		return NULL;
	}

	ptMsg = *pptUserMsg;
	
	CommQue_PutEmpty(ptObj->hJsonMsgQue, (void *)pptUserMsg);
	
	return ptMsg;
}

/**********************************************************************
* 函数名称：StbpClientFreeMsg
* 功能描述：释放消息
* 输入参数：hUser
					ptStbpMsg
* 输出参数：无
* 返 回 值：	 无
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
void StbpClientFreeMsg(HANDLE hUser, TlcStbpMsg_t *ptStbpMsg)
{
	if (NULL == ptStbpMsg)
	{
		return;
	}
	
	TlcMgeStbpcFreeMsg(ptStbpMsg);
}

/**********************************************************************
* 函数名称：StbpClientConnected
* 功能描述：判断连接是否成功
* 输入参数：hUser
* 输出参数：无
* 返 回 值：	 BOOL
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
BOOL StbpClientConnected(HANDLE hUser)
{
	T_StbpClientObj	*ptObj = hUser;

	if (NULL == hUser)
	{
		return SMP_FALSE;
	}
		
	return ptObj->bConnected;
}

/**********************************************************************
* 函数名称：StbpClientGetConnectionHandle
* 功能描述：获取Stbp连接句柄
* 输入参数：hUser
					ptStbpMsg
* 输出参数：无
* 返 回 值：	 无
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
HANDLE StbpClientGetConnectionHandle(HANDLE hUser)
{
	T_StbpClientObj	*ptObj = hUser;

	if (NULL == hUser)
	{
		return NULL;
	}
		
	return ptObj->hStbpConn;
}

/**********************************************************************
* 函数名称：StbpClientGetSyncFlag
* 功能描述：获取同步请求标志
* 输入参数：hUser
* 输出参数：无
* 返 回 值：	 无
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
BOOL StbpClientGetSyncFlag(HANDLE hUser)
{
	T_StbpClientObj	*ptObj = hUser;

	if (NULL == hUser)
	{
		return SMP_FALSE;
	}
		
	return ptObj->bNeedSync;
}

/**********************************************************************
* 函数名称：StbpClientClearSyncFlag
* 功能描述：清除同步请求标志
* 输入参数：hUser
* 输出参数：无
* 返 回 值：	 无
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
void StbpClientClearSyncFlag(HANDLE hUser)
{
	T_StbpClientObj	*ptObj = hUser;

	if (NULL == hUser)
	{
		return;
	}
		
	ptObj->bNeedSync = SMP_FALSE;
}

/**********************************************************************
* 函数名称：StbpClientParseResult
* 功能描述：解析反馈数据
* 输入参数：strResJson
* 输出参数：pptResult
* 返 回 值：	 E_StateCode
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
E_StateCode StbpClientParseResult(INT8 *strResJson, T_StbpResult **pptResult)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	T_StbpResult	*ptResult = NULL;
	INT32		iRet = 0;
	cJSON 		*ptJson = NULL;
	cJSON		*ptSubJson = NULL;


	if ((NULL == strResJson) || (NULL == pptResult))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	ptResult = malloc(sizeof(T_StbpResult));
	if (NULL == ptResult)
	{
		return STATE_CODE_ALLOCATION_FAILURE;
	}
	memset(ptResult, 0x0, sizeof(T_StbpResult));

	ptJson = cJSON_Parse(strResJson);
	if (NULL == ptJson)
	{
		return STATE_CODE_INVALID_PARAM;
	}

	iRet = TlpJson2Obj(g_stStbpResultKey, ptJson, ptResult);
	if (0 != iRet)
	{
		SysErr("CJson2Obj fail ret:%d.\n", iRet);
		eCode =  STATE_CODE_INVALID_PARAM;
		goto cleanup;
	}

	ptSubJson = cJSON_GetObjectItem(ptJson, "data");
	if (NULL != ptSubJson)
	{
		ptResult->pcData = cJSON_Print(ptSubJson);
	}
	
	*pptResult = ptResult;

cleanup:
	if (NULL != ptJson)
	{
		cJSON_Delete(ptJson);
	}
	return eCode;
}

/**********************************************************************
* 函数名称：StbpClientParseResult
* 功能描述：解析反馈数据
* 输入参数：ptResult
* 输出参数：无
* 返 回 值：	 E_StateCode
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
void StbpClientFreeResult(T_StbpResult *ptResult)
{
	if (NULL == ptResult)
	{
		return;
	}

	if (NULL != ptResult->pcMsg)
	{
		free(ptResult->pcMsg);
	}

	if (NULL != ptResult->pcData)
	{
		free(ptResult->pcData);
	}
	free(ptResult);
}

/**********************************************************************
* 函数名称：StbpClientSendMsgAndParseResult
* 功能描述：发送命令并解析结果
* 输入参数：hUser
					strTopic - 主题
					strJsonMsg - 消息体
					uiTimeOut - 超时
* 输出参数：ppcRes - 返回消息，可为空
* 返 回 值：	E_StateCode
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
E_StateCode StbpClientSendMsgAndParseResult(HANDLE hUser, INT8 *strTopic, INT8 *strJsonMsg, UINT32 uiTimeOut, T_StbpResult **pptResult)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	INT8			*pcResJsonMsg = NULL;
	//INT32		iRet = 0;
	T_StbpResult	*ptStbpResult = NULL;
	
	eCode = StbpClientSendMsg(hUser, strTopic, strJsonMsg, uiTimeOut, &pcResJsonMsg);
	if (!STATE_OK(eCode))
	{
		SysErr("StbpClientSendMsg failed, eCode = %d\n", eCode);
		return eCode;
	}

	//dbprintf("\n%s\n", pcResJsonMsg);
	eCode = StbpClientParseResult(pcResJsonMsg, &ptStbpResult);
	if (!STATE_OK(eCode))
	{
		SysErr("StbpClientParseResult failed, eCode = %d\n", eCode);
		goto cleanup;
	}
	
	if (0 != ptStbpResult->iCode)
	{
		SysErr("The result is %d\n\n", ptStbpResult->iCode);
		eCode = ptStbpResult->iCode;
		goto cleanup;
	}

	if (NULL != pptResult)
	{
		*pptResult = ptStbpResult;
		ptStbpResult = NULL;
	}

cleanup:
	if (NULL != pcResJsonMsg)
	{
		free(pcResJsonMsg);
	}

	if (NULL != ptStbpResult)
	{
		StbpClientFreeResult(ptStbpResult);
	}
	return eCode;
}

HANDLE StbpClientGetLoopHandle()
{
	return g_shLoop;
}

/**********************************************************************
* 函数名称：StbpClientPrintMsg
* 功能描述：打印信息
* 输入参数：hUser
* 输出参数：无
* 返 回 值：	
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
void StbpClientPrintMsg(HANDLE hUser)
{
	T_StbpClientObj	*ptObj = hUser;
	TlcStbpMsg_t		**pptUserMsg = NULL;
	TlcStbpMsg_t		**pptFirst = NULL;
	UINT32				uiCnt = 0;
	INT32				iLoop = 0;
	TlcStbpMsg_t		*ptMsg = NULL;
	tlSize_t			uiSendLen = 0;
	tlSize_t			uiRecvLen = 0;

	if (NULL == hUser)
	{
		return;
	}

	uiCnt = CommQue_GetPacketNumForRead(ptObj->hJsonMsgQue);
	SysLog("Cnt is %d/%d\n", uiCnt, CommQue_GetPacketNum(ptObj->hJsonMsgQue));

	TlcMgeStbpcReturnSendLenAndRecvLenInside(ptObj->hStbpConn, &uiSendLen, &uiRecvLen);
	SysLog("SendLen is %d\n", uiSendLen);
	SysLog("RecvLen is %d\n", uiRecvLen);

	for (iLoop = 0; iLoop < uiCnt; iLoop++)
	{
		pptUserMsg = (TlcStbpMsg_t **)CommQue_GetFull(ptObj->hJsonMsgQue, OSAL_TIMEOUT_NONE);
		if (NULL == pptUserMsg)
		{
			return;
		}

		ptMsg = *pptUserMsg;

		SysLog("Msg %d::%s, %s\n", iLoop, ptMsg->topic, ptMsg->pPayload);
		
		CommQue_PutFull(ptObj->hJsonMsgQue, (void *)pptUserMsg);

		if (NULL == pptFirst)
		{
			pptFirst = pptUserMsg;
		}
		else
		{
			if (pptFirst == pptUserMsg)
			{
				break;
			}
		}
	}
	
	return;
}

/**********************************************************************
* 函数名称：StpbClientMakeResult
* 功能描述：生成返回消息
* 输入参数：iRetCode - 返回码
			strMsg - 返回信息
			strData - JSON数据,可为空		
* 输出参数：ppcResult - 
* 返 回 值：	 E_StateCode
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/23	     V1.0	           
***********************************************************************/
E_StateCode StpbClientMakeResult(INT32 iRetCode, INT8 *strMsg, INT8 *strData, INT8 **ppcResult)
{
	E_StateCode		eCode = STATE_CODE_NO_ERROR;
	//INT32			iRet = 0;
	INT32			iLen = 0;
	INT32			iPrintLen = 0;
	INT32			iCurLen = 0;
	INT8				*pcResponse = NULL;

	if ((NULL == ppcResult))
	{
		return STATE_CODE_INVALID_HANDLE;
	}
	
	if (NULL == strData)
	{
		iLen = 0;
	}
	else
	{
		iLen = strlen(strData);
	}

	if (NULL != strMsg)
	{
		iLen += strlen(strMsg);
	}

	iLen += 256;

	pcResponse = malloc(iLen);
	if (NULL == pcResponse)
	{
		SysErr("malloc response buffer failed!\n");
		return STATE_CODE_ALLOCATION_FAILURE;
	}

	iPrintLen = sprintf(pcResponse, "{\"code\":%d", iRetCode);
	iCurLen += iPrintLen;
	if (NULL != strMsg)
	{
		iPrintLen = sprintf(pcResponse + iCurLen, ",\"msg\":\"%s\"", strMsg);
		iCurLen += iPrintLen;
	}

	if (NULL != strData)
	{
		iPrintLen = sprintf(pcResponse + iCurLen, ",\"data\":%s", strData);
		iCurLen += iPrintLen;
	}

	strcat(pcResponse, "}");
	iCurLen++;

	*ppcResult = pcResponse;
	return eCode;
}

