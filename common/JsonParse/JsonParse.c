#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>

#include "JsonEx.h"
#include "JsonParsePriv.h"
#include "MO/MOChannel.h"
#include "cjson/cJSON.h"
#include "common/common.h"
/***********************************************************
 *						常量定义		                       		*
 **********************************************************/

/***********************************************************
 *				文件内部使用的宏                      *
 **********************************************************/


 /***********************************************************
 *			文件内部使用的数据类型 	*
 **********************************************************/
typedef E_StateCode (*Json2Bin)(INT8 *strJson, void *pBinSetup);
typedef E_StateCode (*Bin2Json)(void *pBinSetup, INT8 **pstrJson);

typedef struct
{
	INT8	*strListType;
	Json2Bin	pfJson2Bin;
	Bin2Json	pfBin2Json;
}T_MediaJsonParse;

/***********************************************************
 *						全局变量						*
 **********************************************************/

/***********************************************************
 *						本地变量						*
 **********************************************************/
static T_ListJsonMng	g_satJsonList[] = 
{
	{"venc", 	g_stVideoParamKey, sizeof(T_VEncInfo)},
	{"aenc", 	g_stAudioParamKey, sizeof(T_AEncInfo)},
	{"tsmux",	g_stMuxParamKey, sizeof(T_TsMuxInfo)},
	{"tsmuxservice",	g_stMuxServiceParamKey, sizeof(T_TsMuxServiceInfo)},
	{"encinfo", g_stEncParamKey, sizeof(T_EncInfo)},
	{"transcodeDecodePid", g_stTranscodeDecodePidKey, sizeof(T_TranscodeDecodePid)},
	{"transcodeDecodeParam", g_stTranscodeDecodeParamKey, sizeof(T_TranscodeDecodeParam)},
	{"proginfo", g_stProgInfoKey, sizeof(T_prog_info_t)},
	
	{NULL,	NULL, 0},
};

/***********************************************************
* 						本地函数						*
**********************************************************/
static E_StateCode MOChBaseInfoJson2Bin(INT8 *strJson, T_MOChBaseInfo *ptBinSetup)
{
	E_StateCode		eCode = STATE_CODE_NO_ERROR;
	cJSON			*ptCjson = NULL;
	cJSON			*ptemp = NULL;

	ptCjson = cJSON_Parse(strJson);
	if (NULL == ptCjson)
	{
		return STATE_CODE_INVALID_PARAM;
	}

	ptemp = cJSON_GetObjectItem(ptCjson, "prog_id");
	if(ptemp == NULL)
	{
		syswarn("Do not find prog_id, %s!\n", strJson);
		return STATE_CODE_INVALID_PARAM;
	}

	if (!cJSON_IsNumber(ptemp)) 
	{
        return STATE_CODE_INVALID_PARAM;
    }
	ptBinSetup->prog_id = ptemp->valuedouble;

	cJSON_Delete(ptCjson);
	return eCode;
}

static E_StateCode MOChBaseInfoBin2Json(T_MOChBaseInfo *ptBinSetup, INT8 **pstrJson)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;

	// cJSON		*ptSubJson = NULL;
	// cJSON		*ptItemJson = NULL;
	// INT32		iLoop = 0;
	// UINT32		uiCnt = 0;
	cJSON		*ptCjson = NULL;
	
	ptCjson = cJSON_CreateObject();
	if (NULL == ptCjson)
	{
		return STATE_CODE_ALLOCATION_FAILURE;
	}

	cJSON_AddNumberToObject(ptCjson, "prog_id", ptBinSetup->prog_id);

	*pstrJson = cJSON_Print(ptCjson);
	
	cJSON_Delete(ptCjson);
	return eCode;
}


static T_MediaJsonParse g_satJsonParse[] = 
{
	{"moch",  (Json2Bin)MOChBaseInfoJson2Bin, (Bin2Json)MOChBaseInfoBin2Json},
	{NULL, NULL, NULL}
};

/***********************************************************
* 					 全局函数						 *
**********************************************************/

static E_StateCode MediaParseSpecificListElement(INT8 *strListType, INT8 *strJson, void **ppData)
{
	T_MediaJsonParse	*ptItem = NULL;

	ptItem = g_satJsonParse;
	while (NULL != ptItem->strListType)
	{
		if (0 == strcmp(ptItem->strListType, strListType))
		{
			return ptItem->pfJson2Bin(strJson, ppData);
		}

		ptItem++;
	}

	return STATE_CODE_OBJECT_NOT_EXIST;
}

static E_StateCode MediaMakeSpecificListElement(INT8 *strListType, void *pData, INT8 **pstrJson)
{
	T_MediaJsonParse	*ptItem = NULL;

	ptItem = g_satJsonParse;
	while (NULL != ptItem->strListType)
	{
		if (0 == strcmp(ptItem->strListType, strListType))
		{
			return ptItem->pfBin2Json(pData, pstrJson);
		}

		ptItem++;
	}

	return STATE_CODE_OBJECT_NOT_EXIST;
}

T_ListJsonMng *FindJsonListMngNode(T_ListJsonMng *ptTable, INT8 *strListType)
{
	if ((NULL == ptTable) || (NULL == strListType))
	{
		return NULL;
	}
	
	while (NULL != ptTable->strListType)
	{
		if (0 == strcmp(ptTable->strListType, strListType))
		{
			return ptTable;
		}
		
		ptTable++;
	}

	return NULL;
}

E_StateCode JsonParseListElement(T_ListJsonMng *ptTable, INT8 *strListType, INT8 *strJson, void *pData)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	T_ListJsonMng *ptMng = NULL;
	void *ptStruct = NULL;
	UINT32 uiSize = 0;

	if ((NULL == strListType) || (NULL == strJson) || (NULL == pData))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	ptMng = FindJsonListMngNode(ptTable, strListType);
	if (NULL == ptMng)
	{
		syserr("FindJsonListMngNode failed, list type = %s.\n", strListType);
		return STATE_CODE_OBJECT_NOT_EXIST;
	}

	uiSize = ptMng->uiSize;

	ptStruct = cjson_parse_json_to_struct(strJson, ptMng->ptKey, uiSize);
	if (NULL == ptStruct)
	{
		syserr("cjson_parse_json_to_struct failed, %s.\n", strJson);
		return STATE_CODE_INVALID_PARAM;
	}
	memcpy(pData, ptStruct, uiSize);
	LJMEM_FREE(ptStruct);
	return eCode;	
}

E_StateCode JsonMakeListElement(T_ListJsonMng *ptTable, INT8 *strListType, void *pData, INT8 **ppstrJson)
{
	INT8		*strjson = NULL;
	T_ListJsonMng *ptMng = NULL;

	if ((NULL == strListType) || (NULL == ppstrJson) || (NULL == pData))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	ptMng = FindJsonListMngNode(ptTable, strListType);
	if (NULL == ptMng)
	{
		syserr("FindJsonListMngNode failed, list type = %s.\n", strListType);
		return STATE_CODE_OBJECT_NOT_EXIST;
	}

	strjson = cjson_encode_struct_to_json(pData, ptMng->ptKey);
	if (NULL == strjson)
	{
		syserr("cjson_encode_struct_to_cjson failed, %s.\n", pData);
		return STATE_CODE_ALLOCATION_FAILURE;
	}

	SAFESTRCPY(*ppstrJson, strjson, strlen(strjson) + 1);
	LJMEM_FREE(strjson);
	return STATE_CODE_NO_ERROR;
}

E_StateCode MediaParseListElement(INT8 *strListType, INT8 *strJson, void *pData)
{
	E_StateCode eCode = STATE_CODE_NO_ERROR;
	T_MediaJsonParse	*ptItem = NULL;
	ptItem = g_satJsonParse;
	
	if ((NULL == strListType) || (NULL == strJson) || (NULL == pData))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	eCode = MediaParseSpecificListElement(strListType, strJson, pData);
	if (STATE_OK(eCode))
	{
		return eCode;
	}

	eCode = JsonParseListElement(g_satJsonList, strListType, strJson, pData);
	if (!STATE_OK(eCode))
	{
		syserr("JsonParseListElement failed, strListType is %s\n", strListType);
		return eCode;
	}

	return eCode;
}

E_StateCode MediaMakeListElement(INT8 *strListType, void *pData, INT8 **pstrJson)
{
	E_StateCode eCode = STATE_CODE_NO_ERROR;
	T_MediaJsonParse	*ptItem = NULL;
	ptItem = g_satJsonParse;
	
	if ((NULL == strListType) || (NULL == pstrJson) || (NULL == pData))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	eCode = MediaMakeSpecificListElement(strListType, pData, pstrJson);
	if (STATE_OK(eCode))
	{
		return eCode;
	}
	eCode = JsonMakeListElement(g_satJsonList, strListType, pData, pstrJson);
	if (!STATE_OK(eCode))
	{
		syserr("JsonMakeListElement failed, strListType is %s\n", strListType);
		return eCode;
	}
	return eCode;
}


