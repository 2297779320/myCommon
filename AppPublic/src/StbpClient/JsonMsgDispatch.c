/**************************************************************************
* 版    权：Copyright (c) 2019
* 文件名称：JsonMsgDispatch.c
* 文件标识： 
* 内容摘要： JSON消息处理
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
//#include "osal_mem.h"
//#include "osal_mutex.h"
//#include "doublelink.h"
//#include "CommQue.h"
//#include "DCSocket.h"
#include "JsonMsgDispatch.h"

/***********************************************************
 *						常量定义		                       		*
 **********************************************************/

/***********************************************************
 *				文件内部使用的宏                      *
 **********************************************************/

 /***********************************************************
 *			文件内部使用的数据类型 	*
 **********************************************************/

/***********************************************************
 *						全局变量						*
 **********************************************************/

/***********************************************************
 *						本地变量						*
 **********************************************************/
static String128	g_sstrLocalDomin = "$l";
static String128	g_sstrLocalDevice = "$i";
static String128	g_sstrLocalModuleName = "default";
static String16		g_sstrDefaultVersion = "v1";
static String16		g_sstrDefaultPrefix = JSON_TYPE_RESPONSE;
static String16		g_sstrDefaultType = "$data";
static String16		g_sstrDefaultFuncId = "$set";
static String16		g_sstrDefaultClient = "0";

static char		*g_sapcTopicPartDefault[TOPIC_PART_CNT] = 
{
	g_sstrDefaultPrefix,
	g_sstrDefaultType,
	g_sstrDefaultClient,
	g_sstrLocalDomin,
	g_sstrLocalDevice,
	g_sstrLocalModuleName,
	g_sstrDefaultVersion,
	g_sstrDefaultFuncId
};

/***********************************************************
 *						本地函数					*
 **********************************************************/
 
/***********************************************************
 * 						全局函数						*
 **********************************************************/

BOOL IsFuncIdPartEqual(INT8 *strFmt, INT8 *strFuncId)
{
	INT8		*pcStart = NULL;
	INT8		*pcFind = NULL;
	INT8		*pcDst = NULL;

	if ('*' == *strFmt)
	{
		return SMP_TRUE;
	}
	
	pcDst = strFuncId;
	pcStart = strFmt;
	for (;;)
	{
		pcFind = strchr(pcStart, '%'); /* 查找%d*/
		if (NULL != pcFind)
		{
			if (0 != strncmp(pcStart, pcDst, (pcFind - pcStart)))
			{
				return SMP_FALSE;
			}
			
			pcDst += (pcFind - pcStart);
			while (isdigit(*pcDst)) pcDst++;
			
			pcFind += 2;
			pcStart = pcFind;
		}
		else
		{
			if (0 != strcmp(pcStart, pcDst))
			{
				return SMP_FALSE;
			}
			break;
		}
	}

	return SMP_TRUE;
}

BOOL IsFuncIdEqual(INT8 *strFmt, INT8 *strFuncId)
{
	INT8		*pcStart = NULL;
	INT8		*pcFind = NULL;
	INT8		*pcFind2 = NULL;
	INT8		*pcDst = NULL;

	pcDst = strFuncId;
	pcStart = strFmt;
	for (;;)
	{
		//dbprintf("%s  %s\n", pcStart, pcDst);
		pcFind = strchr(pcStart, '.'); 
		if (NULL != pcFind)
		{
			pcFind2 = strchr(pcDst, '.'); 
			if (NULL == pcFind2)
			{
				return SMP_FALSE;
			}

			if (*pcStart == '*')
			{
				pcStart = pcFind + 1;
				pcDst = pcFind2 + 1;
			}
			else
			{
				if ((pcFind2 - pcDst) != (pcFind - pcStart))
				{
					return SMP_FALSE;
				}

				if (0 != strncmp(pcStart, pcDst, (pcFind - pcStart)))
				{
					return SMP_FALSE;
				}

				pcStart = pcFind + 1;
				pcDst = pcFind2 + 1;
			}
		}
		else
		{
			pcFind2 = strchr(pcDst, '.'); 
			if (NULL != pcFind2)
			{
				return SMP_FALSE;
			}
			else
			{
				return IsFuncIdPartEqual(pcStart, pcDst);
			}
		}
	}

	return SMP_TRUE;
}

E_StateCode ParseFuncId(INT8 *strFmt, INT8 *strFuncId, UINT32 *puiIdx)
{
	INT8		*pcStart = NULL;
	INT8		*pcFind = NULL;
	INT8		*pcDst = NULL;
	String64	strValue = {0,};
	INT8		*pcValue = NULL;

	pcDst = strFuncId;
	pcStart = strFmt;
	for (;;)
	{
		pcFind = strchr(pcStart, '%'); /* 查找%d*/
		if (NULL != pcFind)
		{
			pcDst += (pcFind - pcStart);
			pcValue = strValue;
			while (isdigit(*pcDst)) *pcValue++ = *pcDst++;

			*pcValue = '\0';

			//printf("%d\n", atoi(pcValue));
			*puiIdx++ = atoi(strValue);
			
			pcFind += 2;
			pcStart = pcFind;
		}
		else
		{
			break;
		}
	}

	return STATE_CODE_NO_ERROR;
}

E_StateCode MakeFuncId(INT8 *strFmt, INT8 *strFuncId, UINT32 *puiIdx)
{
	INT8		*pcStart = NULL;
	INT8		*pcFind = NULL;

	*strFuncId = '\0';
	pcStart = strFmt;
	for (;;)
	{
		pcFind = strchr(pcStart, '%'); /* 查找%d*/
		if (NULL != pcFind)
		{
			*(strFuncId + strlen(strFuncId) + (pcFind - pcStart)) = '\0';
			strncpy(strFuncId + strlen(strFuncId), pcStart, pcFind - pcStart);
			pcFind += 2;
			sprintf(strFuncId + strlen(strFuncId), "%u", *puiIdx++);
			pcStart = pcFind;
		}
		else
		{
			strcpy(strFuncId + strlen(strFuncId), pcStart);
			break;
		}
	}

	//printf("%s\n", strFuncId);
	return STATE_CODE_NO_ERROR;
}

E_StateCode ParseFmtTopic(INT8 *strFmt, T_UserTopic *ptTopic)
{
	INT8		*pcCur = strFmt;
	INT8		*pcFind = NULL;
	INT32	iLoop = 0;
	T_MultiChar	*ptItem = NULL;

	UINT32	uiCntOfPoint = 0;

	//dbprintf("ParseFmtTopic::Topic is %s\n", strFmt);

	memset(ptTopic, 0x0, sizeof(T_UserTopic));

	pcFind = pcCur;
	for (;;)
	{
		pcFind = strchr(pcFind, '.');
		if (NULL == pcFind)
		{
			break;
		}

		uiCntOfPoint++;
		pcFind++;
	}

	//dbprintf("uiCntOfPoint is %d\n", uiCntOfPoint);
	if (uiCntOfPoint < TOPIC_PART_CNT - 1)
	{
		ptItem = &ptTopic->atParam[TOPIC_PART_FUNCID];
		ptItem->pcAddr = pcCur;
		ptItem->uiLen = strlen(pcCur);
		
		return STATE_CODE_NO_ERROR;
	}

	/* 从{prefix}字段开始解析*/
	ptItem = &ptTopic->atParam[0];
	for (iLoop = 0; iLoop < TOPIC_PART_CNT - 1; iLoop++, ptItem++)
	{
		pcFind = strchr(pcCur, '.');
		if (NULL == pcFind)
		{
			SysErr("The topic is not valid, %s\n", strFmt);
			return STATE_CODE_INVALID_PARAM;
		}

		if ('*' != *pcCur)
		{
			ptItem->pcAddr = pcCur;
			ptItem->uiLen = pcFind - pcCur;
		}
		
		pcCur = pcFind + 1;

		if (NULL != ptItem->pcAddr)
		{
			//dbprintf("UserTopic::part[%d], start::%d, len::%d\n", iLoop, (UINT32)(ptItem->pcAddr - strFmt), ptItem->uiLen);
		}
		else
		{
			//dbprintf("UserTopic::part[%d] is empty\n", iLoop);
		}
	}

	ptItem->pcAddr = pcCur;
	ptItem->uiLen = strlen(pcCur);
	//dbprintf("UserTopic::part[%d], start::%d, len::%d\n", iLoop, (UINT32)(ptItem->pcAddr - strFmt), ptItem->uiLen);
	
	return STATE_CODE_NO_ERROR;
}

E_StateCode ParseUserTopic(INT8 *strTopic, T_UserTopic *ptTopic)
{
	INT8		*pcCur = strTopic;
	INT8		*pcFind = NULL;
	INT32	iLoop = 0;
	T_MultiChar	*ptItem = NULL;
	
	//dbprintf("ParseUserTopic::Topic is %s\n", strTopic);
	memset(ptTopic, 0x0, sizeof(T_UserTopic));
	
	ptItem = &ptTopic->atParam[0];
	for (iLoop = 0; iLoop < TOPIC_PART_CNT - 1; iLoop++, ptItem++)
	{
		pcFind = strchr(pcCur, '.');
		if (NULL == pcFind)
		{
			SysErr("The topic is not valid, %s\n", strTopic);
			return STATE_CODE_INVALID_PARAM;
		}

		ptItem->pcAddr = pcCur;
		ptItem->uiLen = pcFind - pcCur;
		pcCur = pcFind + 1;

		//dbprintf("UserTopic::part[%d], start::%d, len::%d\n", iLoop, (UINT32)(ptItem->pcAddr - strTopic), ptItem->uiLen);
	}

	ptItem->pcAddr = pcCur;
	ptItem->uiLen = strlen(pcCur);
	//dbprintf("UserTopic::part[%d], start::%d, len::%d\n", iLoop, (UINT32)(ptItem->pcAddr - strTopic), ptItem->uiLen);
	
	return STATE_CODE_NO_ERROR;
}

E_StateCode MakeUserTopic(INT8 *strTopic, T_UserTopic *ptTopic)
{
	INT8		*pcCur = strTopic;
	INT32	iLoop = 0;
	INT32	jLoop = 0;
	T_MultiChar	*ptItem = NULL;

	INT8		*pcAddr = NULL;
	UINT32	uiLen = 0;
	
	ptItem = &ptTopic->atParam[0];
	for (iLoop = 0; iLoop < TOPIC_PART_CNT; iLoop++, ptItem++)
	{
		if (NULL == ptItem->pcAddr)
		{
			pcAddr = g_sapcTopicPartDefault[iLoop];
			uiLen = strlen(pcAddr);
		}
		else
		{
			pcAddr = ptItem->pcAddr;
			uiLen = ptItem->uiLen;
		}
		
		for (jLoop = 0; jLoop < uiLen; jLoop++)
		{
			*pcCur++ = pcAddr[jLoop];
		}

		if (iLoop != TOPIC_PART_CNT - 1)
		{
			*pcCur++ = '.';
		}
	}

	*pcCur++ = '\0';

	//dbprintf("MakeUserTopic:: %s\n", strTopic);
	return STATE_CODE_NO_ERROR;
}

BOOL IsMultiCharEqual(T_MultiChar *ptMChar1, T_MultiChar *ptMChar2)
{
	INT32		jLoop = 0;
	INT8			*pcFind = NULL;
	UINT32		uiCmpLen = 0;
	
	if ((NULL == ptMChar1->pcAddr) || (NULL == ptMChar2->pcAddr))
	{
		return SMP_TRUE;
	}

	pcFind = NULL;
	for (jLoop = 0; jLoop < ptMChar1->uiLen; jLoop++)
	{
		if ('*' == ptMChar1->pcAddr[jLoop])
		{
			pcFind = ptMChar1->pcAddr + jLoop;
			uiCmpLen = jLoop;
			break;
		}

		if (ptMChar2->pcAddr[jLoop] != ptMChar1->pcAddr[jLoop])
		{
			return SMP_FALSE;
		}
	}

	if (NULL != pcFind)
	{
		if (ptMChar2->uiLen < uiCmpLen)
		{
			return SMP_FALSE;
		}
	}
	else
	{
		if (ptMChar2->uiLen != ptMChar1->uiLen)
		{
			return SMP_FALSE;
		}
	}

	return SMP_TRUE;
}

 BOOL IsMultiCharEqualString(T_MultiChar *ptMChar1, INT8 *strValue)
{
	T_MultiChar	tMChar;
	
	if ((NULL == ptMChar1->pcAddr) || (NULL == strValue))
	{
		return SMP_TRUE;
	}

	tMChar.pcAddr = strValue;
	tMChar.uiLen = strlen(strValue);

	return IsMultiCharEqual(ptMChar1, &tMChar);
}

 BOOL IsTopicEqual(INT8 *strFmt, INT8 *strTopic)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	T_UserTopic	tTopic1;
	T_UserTopic	tTopic2;
	T_MultiChar	*ptItem1 = NULL;
	T_MultiChar	*ptItem2 = NULL;
	INT32		iLoop = 0;

	memset(&tTopic1, 0x0, sizeof(T_UserTopic));
	memset(&tTopic2, 0x0, sizeof(T_UserTopic));

	eCode = ParseFmtTopic(strFmt, &tTopic1);
	if (!STATE_OK(eCode))
	{
		return SMP_FALSE;
	}

	eCode = ParseUserTopic(strTopic, &tTopic2);
	if (!STATE_OK(eCode))
	{
		return SMP_FALSE;
	}

	ptItem1 = &tTopic1.atParam[0];
	ptItem2 = &tTopic2.atParam[0];
	for (iLoop = 0; iLoop < TOPIC_PART_CNT - 1; iLoop++, ptItem1++, ptItem2++)
	{
		if (!IsMultiCharEqual(ptItem1, ptItem2))
		{
			return SMP_FALSE;
		}
	}

	if ((NULL != ptItem1->pcAddr) && (NULL != ptItem2->pcAddr))
	{
		if (!IsFuncIdEqual(ptItem1->pcAddr, ptItem2->pcAddr))
		{
			return SMP_FALSE;
		}
	}
	
	return SMP_TRUE;
}

BOOL IsTopicEqual2(INT8 *strFmt, T_UserTopic *ptUserTopic)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	T_UserTopic	tTopic1;
	T_MultiChar	*ptItem1 = NULL;
	T_MultiChar	*ptItem2 = NULL;
	INT32		iLoop = 0;

	memset(&tTopic1, 0x0, sizeof(T_UserTopic));

	eCode = ParseFmtTopic(strFmt, &tTopic1);
	if (!STATE_OK(eCode))
	{
		return SMP_FALSE;
	}

	ptItem1 = &tTopic1.atParam[0];
	ptItem2 = &ptUserTopic->atParam[0];
	for (iLoop = 0; iLoop < TOPIC_PART_CNT - 1; iLoop++, ptItem1++, ptItem2++)
	{
		if (!IsMultiCharEqual(ptItem1, ptItem2))
		{
			return SMP_FALSE;
		}
	}

	if ((NULL != ptItem1->pcAddr) && (NULL != ptItem2->pcAddr))
	{
		if (!IsFuncIdEqual(ptItem1->pcAddr, ptItem2->pcAddr))
		{
			return SMP_FALSE;
		}
	}
	
	return SMP_TRUE;
}

T_JsonMsgDispatch *FindMsgDispathByUserTopic(T_JsonMsgDispatch *ptTable, T_UserTopic *ptTopic)
{
	T_JsonMsgDispatch	*ptTrans = NULL;
	
	ptTrans = ptTable;
	//printf("toFind::%s\n", strMsgName);
	while (NULL != ptTrans->pcTopic)
	{
		//printf("cmp with::%s\n", ptTrans->strTopicId);
		if (IsTopicEqual2((INT8 *)ptTrans->pcTopic, ptTopic))
		{
			return ptTrans;
		}
		ptTrans++;
	}

	return NULL;
}

 /**********************************************************************
* 函数名称：FindJsonMsgDispatch
* 功能描述：查找JSON消息处理项
* 输入参数：ptTable - 处理表
					strTopic - 消息主题
* 输出参数：无
* 返 回 值：	 T_JsonMsgDispatch *
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/03/11	     V1.0	           
***********************************************************************/
T_JsonMsgDispatch *FindJsonMsgDispatch(T_JsonMsgDispatch *ptTable, INT8 *strTopic)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	T_UserTopic	tUserTopic;

	if ((NULL == ptTable) || (NULL == strTopic))
	{
		return NULL;
	}

	memset(&tUserTopic, 0x0, sizeof(tUserTopic));
	eCode = ParseUserTopic(strTopic, &tUserTopic);
	if (!STATE_OK(eCode))
	{
		SysErr("ParseUserTopic failed, eCode = %d, %s\n", eCode, strTopic);
		return NULL;
	}

	return FindMsgDispathByUserTopic(ptTable, &tUserTopic);
}

 /**********************************************************************
* 函数名称：FindJsonMsgDispatch
* 功能描述：查找JSON消息处理项
* 输入参数：strTopicFmt - 消息格式，支持通配符'*'、%d
					strTopic - 消息主题
				strPrefix strType strDomin strDevId strModuleId uiVersion puiTopicIndex -
														主题参数，可以为NULL
* 输出参数：无
* 返 回 值：	E_StateCode
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/03/11	     V1.0	           
***********************************************************************/
E_StateCode MakeJsonMsgTopic(INT8 *strTopicFmt, 
							INT8 *strTopic,
							INT8 *strPrefix, 
							INT8 *strType, 
							INT8 *strDomin, 
							INT8 *strDevId,
							INT8 *strModuleId,
							UINT32 uiVersion,
							UINT32 *puiTopicIndex)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	T_UserTopic	tFmtTopic;
	String128		strFuncId = {0,};
	String8		strVersion = {0,};

	if ((NULL == strTopicFmt) || (NULL == strTopic))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	memset(&tFmtTopic, 0x0, sizeof(tFmtTopic));
	
	eCode = ParseFmtTopic(strTopicFmt, &tFmtTopic);
	if (!STATE_OK(eCode))
	{
		SysErr("ParseFmtTopic failed, eCode = %d, %s\n", eCode, strTopicFmt);
		return STATE_CODE_INVALID_PARAM;
	}

	if (NULL != puiTopicIndex)
	{
		MakeFuncId(tFmtTopic.atParam[TOPIC_PART_FUNCID].pcAddr, strFuncId, puiTopicIndex);
		tFmtTopic.atParam[TOPIC_PART_FUNCID].pcAddr = strFuncId;
		tFmtTopic.atParam[TOPIC_PART_FUNCID].uiLen = strlen(strFuncId);
	}

	if (NULL != strPrefix)
	{
		tFmtTopic.atParam[TOPIC_PART_PREFIX].pcAddr = strPrefix;
		tFmtTopic.atParam[TOPIC_PART_PREFIX].uiLen = strlen(strPrefix);
	}

	if (NULL != strType)
	{
		tFmtTopic.atParam[TOPIC_PART_TYPEMETHOD].pcAddr = strType;
		tFmtTopic.atParam[TOPIC_PART_TYPEMETHOD].uiLen = strlen(strType);
	}
	
	if (NULL != strDomin)
	{
		tFmtTopic.atParam[TOPIC_PART_DOMIN].pcAddr = strDomin;
		tFmtTopic.atParam[TOPIC_PART_DOMIN].uiLen = strlen(strDomin);
	}

	if (NULL != strDevId)
	{
		tFmtTopic.atParam[TOPIC_PART_DEV].pcAddr = strDevId;
		tFmtTopic.atParam[TOPIC_PART_DEV].uiLen = strlen(strDevId);
	}

	if (NULL != strModuleId)
	{
		tFmtTopic.atParam[TOPIC_PART_MODULE].pcAddr = strModuleId;
		tFmtTopic.atParam[TOPIC_PART_MODULE].uiLen = strlen(strModuleId);
	}

	if (uiVersion > 0)
	{
		sprintf(strVersion, "v%d", uiVersion);
		tFmtTopic.atParam[TOPIC_PART_VERSION].pcAddr = strVersion;
		tFmtTopic.atParam[TOPIC_PART_VERSION].uiLen = strlen(strVersion);
	}

	eCode = MakeUserTopic(strTopic, &tFmtTopic);
	
	return eCode;
}

/**********************************************************************
* 函数名称：GetTopicId
* 功能描述：从TOPIC中获取TOPIC ID
* 输入参数：strTopic - 
* 输出参数：
* 返 回 值：	TOPIC ID
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/03/11	     V1.0	           
***********************************************************************/
INT8 *GetTopicId(INT8 *strTopic)
{
	INT32		iLoop = 0;
	INT8			*pcFind = NULL;

	pcFind = strTopic;
	for (iLoop = 0; iLoop < 5; iLoop++)
	{
		pcFind = strchr(pcFind, '.');
		if (NULL == pcFind)
		{
			pcFind = strTopic;
			break;
		}

		pcFind++;
	}

	return pcFind;
}

/**********************************************************************
* 函数名称：GetClientId
* 功能描述：从TOPIC中获取客户ID
* 输入参数：strTopic - 
				uiMaxLen - 客户ID的最大长度
* 输出参数：strClientId - 
* 返 回 值：	TOPIC ID
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/03/11	     V1.0	           
***********************************************************************/
E_StateCode GetClientId(INT8 *strTopic, INT8 *strClientId, UINT32 uiMaxLen)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	T_UserTopic	tUserTopic;

	if (NULL == strTopic)
	{
		return STATE_CODE_INVALID_HANDLE;
	}
	
	memset(&tUserTopic, 0x0, sizeof(tUserTopic));
	eCode = ParseUserTopic(strTopic, &tUserTopic);
	if (!STATE_OK(eCode))
	{
		SysErr("ParseUserTopic failed, eCode = %d\n", eCode);
		return eCode;
	}

	if (tUserTopic.atParam[TOPIC_PART_CLIENT].uiLen > uiMaxLen)
	{
		tUserTopic.atParam[TOPIC_PART_CLIENT].uiLen = uiMaxLen;
	}
	strncpy(strClientId, tUserTopic.atParam[TOPIC_PART_CLIENT].pcAddr, tUserTopic.atParam[TOPIC_PART_CLIENT].uiLen);

	return STATE_CODE_NO_ERROR;
}

/**********************************************************************
* 函数名称：GetClientIdNumernic
* 功能描述：从TOPIC中获取客户ID,以数字形式返回
* 输入参数：strTopic - 
* 输出参数：
* 返 回 值：	TOPIC ID
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/03/11	     V1.0	           
***********************************************************************/
UINT32 GetClientIdNumernic(INT8 *strTopic)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	String64	strClientId = {0,};

	if (NULL == strTopic)
	{
		return 0;
	}
	
	eCode = GetClientId(strTopic, strClientId, sizeof(strClientId));
	if (!STATE_OK(eCode))
	{
		return 0;
	}
	
	if ('*' == strClientId[0])
	{
		return 0;
	}

	return atoi(strClientId);
}


/**********************************************************************
* 函数名称：SetJsonMsgLocalModuleName
* 功能描述：设置本地模块名
* 输入参数：strLocalModuleName 
* 输出参数：无
* 返 回 值：	E_StateCode
* 其它说明：
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/06    V1.0	tanrp	           
***********************************************************************/
E_StateCode SetJsonMsgLocalModuleName(INT8 *strLocalModuleName)
{
	if (NULL == strLocalModuleName)
	{
		return STATE_CODE_INVALID_HANDLE;
	}
	
	strcpy(g_sstrLocalModuleName, strLocalModuleName);
	
	return STATE_CODE_NO_ERROR;
}

