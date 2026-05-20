/**************************************************************************
* 版    权：Copyright (c) 2015
* 文件名称：JsonEx.c
* 文件标识：JSON扩展接口实现
* 内容摘要： 
* 其它说明：
* 当前版本： 
* 作    者：谭荣鹏
* 完成日期：2019年3月 4日
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

#include "JsonEx.h"

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

/***********************************************************
 * 						本地函数						*
 **********************************************************/

/***********************************************************
 * 						全局函数						*
 **********************************************************/ 
E_StateCode JsonParseObject(cJSON *ptCjson, const INT8 *strField, TlKeyInfo_t *ptKey, void *pObj)
{
	cJSON			*ptSub = NULL;
	int				iRet = 0;
	//E_StateCode		eCode = STATE_CODE_NO_ERROR;

	if ((NULL == pObj) ||
		(NULL == ptCjson) ||
		(NULL == ptKey))
	{
		SysErr("InvalidHandle.\n");
		return STATE_CODE_INVALID_HANDLE;
	}

	if (NULL == strField)
	{
		ptSub = ptCjson;
	}
	else
	{
		ptSub = cJSON_GetObjectItem(ptCjson, strField);
		if (NULL == ptSub)
		{
			SysErr("JSON %s is not exist.\n", strField);
			return STATE_CODE_OBJECT_NOT_EXIST;
		}
	}

	iRet = TlpJson2Obj(ptKey, ptSub, pObj);
	if (0 != iRet)
	{
		SysErr("CJson2Obj fail ret:%d.\n", iRet);
		return STATE_CODE_INVALID_PARAM;
	}
	
	return STATE_CODE_NO_ERROR;
}

E_StateCode JsonAddObject(cJSON *ptCjson, const INT8 *strField, TlKeyInfo_t *ptKey, void *pObj)
{
	cJSON			*ptSub = NULL;
	int				iRet = 0;
	//E_StateCode		eCode = STATE_CODE_NO_ERROR;

	if ((NULL == pObj) ||
		(NULL == ptCjson) ||
		(NULL == ptKey))
	{
		SysErr("InvalidHandle.\n");
		return STATE_CODE_INVALID_HANDLE;
	}

	if (NULL == strField)
	{
		ptSub = ptCjson;
	}
	else
	{
		ptSub = cJSON_CreateObject();
		if (NULL == ptSub)
		{
			SysErr("cJSON_CreateObject faile for %s failed!\n", strField);
			return STATE_CODE_ALLOCATION_FAILURE;
		}

		cJSON_AddItemToObject(ptCjson, strField, ptSub);
	}
	
	iRet = TlpObj2Json(ptKey, ptSub, pObj);
	if (0 != iRet)
	{
		SysErr("TlpObj2Json fail ret:%d.\n", iRet);
		return STATE_CODE_INVALID_PARAM;
	}
	
	return STATE_CODE_NO_ERROR;
}

E_StateCode JsonAddString(cJSON *ptCjson, const INT8 *strField, const INT8 *strValue)
{
	cJSON			*ptSub = NULL;

	if ((NULL == strField) ||
		(NULL == ptCjson) ||
		(NULL == strValue))
	{
		SysErr("InvalidHandle.\n");
		return STATE_CODE_INVALID_HANDLE;
	}

	ptSub = cJSON_CreateString(strValue);
	if (NULL == ptSub)
	{
		SysErr("cJSON_CreateString faile for %s failed!\n", strField);
		return STATE_CODE_ALLOCATION_FAILURE;
	}

	cJSON_AddItemToObject(ptCjson, strField, ptSub);
	return STATE_CODE_NO_ERROR;
}
 
E_StateCode JsonParseNumberArray(cJSON *ptCjson, const INT8 *strField, UINT32 *puiData, UINT32 *puiItems, UINT32 uiMaxItems)
{
	E_StateCode		eCode = STATE_CODE_NO_ERROR;
	cJSON		*ptSubJson = NULL;
	cJSON		*ptItemJson = NULL;
	INT32		iLoop = 0;
	UINT32		uiCnt = 0;
	//INT32		iRet = 0;

	if ((NULL == puiData) ||
		(NULL == ptCjson) ||
		(NULL == puiItems))
	{
		SysErr("InvalidHandle.\n");
		return STATE_CODE_INVALID_HANDLE;
	}
	
	if (NULL == strField)
	{
		ptSubJson = ptCjson;
	}
	else
	{
		ptSubJson = cJSON_GetObjectItem(ptCjson, strField);
		if (NULL == ptSubJson)
		{
			return STATE_CODE_INVALID_PARAM;
		}
	}

	uiCnt = cJSON_GetArraySize(ptSubJson);
	if (uiCnt > uiMaxItems)
	{
		uiCnt = uiMaxItems;
	}
	
	for (iLoop = 0; iLoop < uiCnt; iLoop++)
	{
		ptItemJson = cJSON_GetArrayItem(ptSubJson, iLoop);
		if (NULL != ptItemJson)
		{
			if (cJSON_Number == ptItemJson->type)
			{
				puiData[iLoop] = ptItemJson->valueint;
			}
			else
			{
				eCode = STATE_CODE_INVALID_PARAM;
				break;
			}
		}
		else
		{
			eCode = STATE_CODE_OBJECT_NOT_EXIST;
			break;
		}
	}

	if (STATE_OK(eCode))
	{
		*puiItems = uiCnt;
	}
	else
	{
		*puiItems = 0;
	}
	return eCode;
}

E_StateCode JsonAddNumberArray(cJSON *ptCjson, const INT8 *strField, UINT32 *puiData, UINT32 uiItems)
{
	E_StateCode		eCode = STATE_CODE_NO_ERROR;
	cJSON		*ptSubJson = NULL;
	cJSON		*ptItemJson = NULL;
	INT32		iLoop = 0;
	UINT32		uiCnt = uiItems;

	if ((NULL == puiData) ||
		(NULL == ptCjson))
	{
		SysErr("InvalidHandle.\n");
		return STATE_CODE_INVALID_HANDLE;
	}
	
	if (NULL == strField)
	{
		ptSubJson = ptCjson;
	}
	else
	{
		if (NULL == ptCjson)
		{
			return STATE_CODE_INVALID_HANDLE;
		}
		ptSubJson = cJSON_CreateArray();
		if (NULL == ptSubJson)
		{
			return STATE_CODE_ALLOCATION_FAILURE;
		}

		cJSON_AddItemToObject(ptCjson, strField, ptSubJson);
	}
	
	for (iLoop = 0; iLoop < uiCnt; iLoop++)
	{
		ptItemJson = cJSON_CreateNumber(puiData[iLoop]);
		if (NULL == ptItemJson)
		{
			eCode = STATE_CODE_ALLOCATION_FAILURE;
			break;
		}
		cJSON_AddItemToArray(ptSubJson, ptItemJson);
	}
	return eCode;
}

E_StateCode JsonParseObjectArray(cJSON *ptCjson, const INT8 *strField, TlKeyInfo_t *ptKey, void *pData, UINT32 *puiItems, UINT32 uiMaxItems)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	INT32		iRet = 0;
	UINT32		uiItemSize = 0;
	UINT8		*pucBinSetup = NULL;
	INT32		iLoop = 0;
	cJSON		*ptSubJson = NULL;
	cJSON		*ptItemJson = NULL;

	if ((NULL == pData) ||
		(NULL == ptCjson) ||
		(NULL == ptKey) ||
		(NULL == puiItems))
	{
		SysErr("InvalidHandle.\n");
		return STATE_CODE_INVALID_HANDLE;
	}
	
	if (NULL == strField)
	{
		ptSubJson = ptCjson;
	}
	else
	{
		ptSubJson = cJSON_GetObjectItem(ptCjson, strField);
		if (NULL == ptSubJson)
		{
			return STATE_CODE_INVALID_PARAM;
		}
	}
	
	*puiItems = cJSON_GetArraySize(ptSubJson);

	if (*puiItems > uiMaxItems)
	{
		*puiItems = uiMaxItems;
	}

	pucBinSetup = pData;
	uiItemSize = ptKey->csize;
	
	for (iLoop = 0; iLoop < *puiItems; iLoop++)
	{
		ptItemJson = cJSON_GetArrayItem(ptSubJson, iLoop);
		if (NULL != ptItemJson)
		{	
			iRet = TlpJson2Obj(ptKey, ptItemJson, pucBinSetup);
			if (0 != iRet)
			{
				SysErr("TlpJson2Obj fail ret:%d.\n", iRet);
				eCode =  STATE_CODE_INVALID_PARAM;
				break;
			}
		}
		else
		{
			eCode = STATE_CODE_OBJECT_NOT_EXIST;
			break;
		}

		pucBinSetup += uiItemSize;
	}
	
	return eCode;
}

E_StateCode JsonAddObjectArray(cJSON *ptCjson, const INT8 *strField, TlKeyInfo_t *ptKey, void *pData, UINT32 uiItems)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	//INT32		iRet = 0;
	UINT32		uiItemSize = 0;
	UINT8		*pucBinSetup = NULL;
	INT32		iLoop = 0;
	cJSON		*ptSubJson = NULL;
	cJSON		*ptItemJson = NULL;

	if ((NULL == ptCjson) ||
		(NULL == ptKey))
	{
		SysErr("InvalidHandle.\n");
		return STATE_CODE_INVALID_HANDLE;
	}
	
	if (NULL == strField)
	{
		ptSubJson = ptCjson;
	}
	else
	{
		if (NULL == ptCjson)
		{
			return STATE_CODE_INVALID_HANDLE;
		}
		ptSubJson = cJSON_CreateArray();
		if (NULL == ptSubJson)
		{
			return STATE_CODE_ALLOCATION_FAILURE;
		}

		cJSON_AddItemToObject(ptCjson, strField, ptSubJson);
	}

	if ((NULL == pData) || (0 == uiItems))
	{
		return STATE_CODE_NO_ERROR;
	}
	
	pucBinSetup = pData;
	uiItemSize = ptKey->csize;

	//dbprintf("uiItemSize = %d\n", uiItemSize);
	for (iLoop = 0; iLoop < uiItems; iLoop++)
	{
		ptItemJson = cJSON_CreateObject();
		if (NULL == ptItemJson)
		{
			eCode = STATE_CODE_ALLOCATION_FAILURE;
			break;
		}

		TlpObj2Json(ptKey, ptItemJson, pucBinSetup);
		
		cJSON_AddItemToArray(ptSubJson, ptItemJson);

		pucBinSetup += uiItemSize;
	}
	
	return eCode;
}

E_StateCode JsonAddObjectArrayWithExternal(cJSON *ptCjson, const INT8 *strField, TlKeyInfo_t *ptKey, void *pData, UINT32 uiItems, T_MExternFileld *ptExtern, UINT32 uiExternCnt)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	//INT32		iRet = 0;
	UINT32		uiItemSize = 0;
	UINT8		*pucBinSetup = NULL;
	INT32		iLoop = 0;
	INT32		jLoop = 0;
	cJSON		*ptSubJson = NULL;
	cJSON		*ptItemJson = NULL;
	INT8		**ppcValue = NULL;
	INT8		*pcValue = NULL;

	if ((NULL == ptCjson) ||
		(NULL == ptKey))
	{
		SysErr("InvalidHandle.\n");
		return STATE_CODE_INVALID_HANDLE;
	}
	
	if (NULL == strField)
	{
		ptSubJson = ptCjson;
	}
	else
	{
		if (NULL == ptCjson)
		{
			return STATE_CODE_INVALID_HANDLE;
		}
		ptSubJson = cJSON_CreateArray();
		if (NULL == ptSubJson)
		{
			return STATE_CODE_ALLOCATION_FAILURE;
		}

		cJSON_AddItemToObject(ptCjson, strField, ptSubJson);
	}

	if ((NULL == pData) || (0 == uiItems))
	{
		return STATE_CODE_NO_ERROR;
	}
	
	pucBinSetup = pData;
	uiItemSize = ptKey->csize;

	//dbprintf("uiItemSize = %d\n", uiItemSize);
	//dbprintf("uiItems = %d\n", uiItems);
	for (iLoop = 0; iLoop < uiItems; iLoop++)
	{
		ptItemJson = cJSON_CreateObject();
		if (NULL == ptItemJson)
		{
			eCode = STATE_CODE_ALLOCATION_FAILURE;
			break;
		}

		TlpObj2Json(ptKey, ptItemJson, pucBinSetup);

		for (jLoop = 0; jLoop < uiExternCnt; jLoop++)
		{
			ppcValue = (INT8 **)(pucBinSetup + ptExtern[jLoop].uiOffset);
			pcValue = *ppcValue;
			if ((NULL != ptExtern[jLoop].strField) && (NULL != pcValue))
			{
				JsonAddString(ptItemJson, ptExtern[jLoop].strField, pcValue);
			}
		}

		//dbprintf("cJSON_AddItemToArray = %d\n", iLoop);
		cJSON_AddItemToArray(ptSubJson, ptItemJson);

		pucBinSetup += uiItemSize;
	}
	
	return eCode;
}

E_StateCode JsonMakeObjectStringWithExternal(void *pObj, TlKeyInfo_t *ptKey, INT8 *strExternalField, INT8 *strExternalValue, INT8 **pstrJsonString)
{
	cJSON			*ptCjson = NULL;
	int				iRet = 0;

	if ((NULL == pObj) ||
		(NULL == ptKey) ||
		(NULL == pstrJsonString))
	{
		SysErr("InvalidHandle.\n");
		return STATE_CODE_INVALID_HANDLE;
	}

	ptCjson = cJSON_CreateObject();

	iRet = TlpObj2Json(ptKey, ptCjson, pObj);
	if (0 != iRet)
	{
		SysErr("TlpObj2Json fail ret:%d.\n", iRet);
		cJSON_Delete(ptCjson);
		return STATE_CODE_INVALID_HANDLE;
	}

	if ((NULL != strExternalField) && (NULL != strExternalValue))
	{
		JsonAddString(ptCjson, strExternalField, strExternalValue);
	}

	*pstrJsonString = cJSON_Print(ptCjson);

	cJSON_Delete(ptCjson);
	return STATE_CODE_NO_ERROR;
}

E_StateCode JsonMakeObjectStringWithMultiExternal(void *pObj, TlKeyInfo_t *ptKey, INT8 **pstrExternalField, INT8 **pstrExternalValue, INT32 iExternalCnt, INT8 **pstrJsonString)
{
	cJSON			*ptCjson = NULL;
	int				iRet = 0;
	int				iLoop = 0;

	if ((NULL == pObj) ||
		(NULL == ptKey) ||
		(NULL == pstrJsonString))
	{
		SysErr("InvalidHandle.\n");
		return STATE_CODE_INVALID_HANDLE;
	}

	ptCjson = cJSON_CreateObject();

	iRet = TlpObj2Json(ptKey, ptCjson, pObj);
	if (0 != iRet)
	{
		SysErr("TlpObj2Json fail ret:%d.\n", iRet);
		cJSON_Delete(ptCjson);
		return STATE_CODE_INVALID_HANDLE;
	}

	for (iLoop = 0; iLoop < iExternalCnt; iLoop++)
	{
		if ((NULL != pstrExternalValue[iLoop]) && (NULL != pstrExternalValue[iLoop]))
		{
			JsonAddString(ptCjson, pstrExternalValue[iLoop], pstrExternalValue[iLoop]);
		}
	}

	*pstrJsonString = cJSON_Print(ptCjson);

	cJSON_Delete(ptCjson);
	return STATE_CODE_NO_ERROR;
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
	INT32		iRet = 0;
	T_ListJsonMng *ptMng = NULL;
	cJSON 		*ptJson = NULL;
	cJSON		*ptSubJson = NULL;

	if ((NULL == strListType) || (NULL == strJson) || (NULL == pData))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	ptMng = FindJsonListMngNode(ptTable, strListType);
	if (NULL == ptMng)
	{
		SysErr("FindJsonListMngNode failed, list type = %s.\n", strListType);
		return STATE_CODE_OBJECT_NOT_EXIST;
	}

	memset(pData, 0x0, ptMng->ptKey->csize);

	ptJson = cJSON_Parse(strJson);
	if (NULL == ptJson)
	{
		SysErr("%s::cJSON_Parse failed, %s.\n", __FUNCTION__, strJson);
		return STATE_CODE_INVALID_PARAM;
	}

	if (cJSON_Array == ptJson->type)
	{
		ptSubJson = cJSON_GetArrayItem(ptJson, 0);
		if (NULL == ptSubJson)
		{
			//SysErr("cJSON_GetArrayItem[0] failed!\n");
			eCode = STATE_CODE_OBJECT_NOT_EXIST;
			goto cleanup;
		}
	}
	else
	{
		ptSubJson = ptJson;
	}

	iRet = TlpJson2Obj(ptMng->ptKey, ptSubJson, pData);
	if (0 != iRet)
	{
		SysErr("CJson2Obj fail ret:%d.\n", iRet);
		eCode =  STATE_CODE_INVALID_PARAM;
		goto cleanup;
	}

cleanup:
	if (NULL != ptJson)
	{
		cJSON_Delete(ptJson);
	}
	return eCode;
	
}

E_StateCode JsonMakeListElement(T_ListJsonMng *ptTable, INT8 *strListType, void *pData, INT8 **pstrJson)
{
	cJSON		*ptCjson = NULL;
	int			iRet = 0;
	INT32		iLoop = 0;
	INT8			*pcValue = NULL;
	T_ListJsonMng *ptMng = NULL;

	if ((NULL == strListType) || (NULL == pstrJson) || (NULL == pData))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	ptMng = FindJsonListMngNode(ptTable, strListType);
	if (NULL == ptMng)
	{
		SysErr("FindJsonListMngNode failed, list type = %s.\n", strListType);
		return STATE_CODE_OBJECT_NOT_EXIST;
	}

	ptCjson = cJSON_CreateObject();

	iRet = TlpObj2Json(ptMng->ptKey, ptCjson, pData);
	if (0 != iRet)
	{
		SysErr("TlpObj2Json fail ret:%d.\n", iRet);
		cJSON_Delete(ptCjson);
		return STATE_CODE_INVALID_HANDLE;
	}

	for (iLoop = 0; iLoop < MAX_EXTERN_CNT; iLoop++)
	{
		pcValue = (INT8 *)pData + ptMng->atExternFiled[iLoop].uiOffset;
		if ((NULL != ptMng->atExternFiled[iLoop].strField) && (NULL != pcValue))
		{
			JsonAddString(ptCjson, ptMng->atExternFiled[iLoop].strField, pcValue);
		}
	}

	*pstrJson = cJSON_Print(ptCjson);

	cJSON_Delete(ptCjson);
	return STATE_CODE_NO_ERROR;
}

UINT32 JsonGetListElementSize(T_ListJsonMng *ptTable, INT8 *strListType)
{
	T_ListJsonMng *ptMng = NULL;

	if ((NULL == ptTable) || (NULL == strListType))
	{
		return 0;
	}

	ptMng = FindJsonListMngNode(ptTable, strListType);
	if (NULL == ptMng)
	{
		SysErr("FindJsonListMngNode failed, list type = %s.\n", strListType);
		return 0;
	}

	return ptMng->ptKey->csize;
}

E_StateCode JsonParseList(T_ListJsonMng *ptTable, INT8 *strListType, INT8 *strJson, void **ppData, UINT32 *puiCnt)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	cJSON 		*ptJson = NULL;
	void			*pData = NULL;
	UINT32		uiCnt = 0;
	UINT32		uiAllocSize = 0;
	T_ListJsonMng *ptMng = NULL;
		
	if ((NULL == strListType) || (NULL == strJson) || (NULL == ppData) || (NULL == puiCnt))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	ptMng = FindJsonListMngNode(ptTable, strListType);
	if (NULL == ptMng)
	{
		SysErr("FindJsonListMngNode failed, list type = %s.\n", strListType);
		return STATE_CODE_OBJECT_NOT_EXIST;
	}

	ptJson = cJSON_Parse(strJson);
	if (NULL == ptJson)
	{
		SysErr("%s::cJSON_Parse failed, %s.\n", __FUNCTION__, strJson);
		return STATE_CODE_INVALID_PARAM;
	}

	if(cJSON_Array != ptJson->type)
	{
		SysErr("type was not Array but %d?!\n", ptJson->type);
		eCode =	STATE_CODE_INVALID_PARAM;
		goto cleanup;
	}

	uiCnt = cJSON_GetArraySize(ptJson);
	if (0 == uiCnt)
	{
		goto cleanup;
	}

	uiAllocSize = ptMng->ptKey->csize * uiCnt;
	pData = malloc(uiAllocSize);
	if (NULL == pData)
	{
		SysErr("malloc failed!\n");
		eCode = STATE_CODE_ALLOCATION_FAILURE;
		goto cleanup;
	}

	memset(pData, 0x0, uiAllocSize);
	eCode = JsonParseObjectArray(ptJson, NULL, ptMng->ptKey, pData, &uiCnt, uiCnt);
	if (!STATE_OK(eCode))
	{
		SysErr("JsonParseObjectArray failed, eCode = %d\n", eCode);
		goto cleanup;
	}
	
cleanup:
	cJSON_Delete(ptJson);
	if (!STATE_OK(eCode))
	{
		if (NULL != pData)
		{
			free(pData);
			pData = NULL;
			uiCnt = 0;
		}
	}
	
	*puiCnt = uiCnt;
	*ppData = pData;
	
	return eCode;
}

E_StateCode JsonMakeList(T_ListJsonMng *ptTable, INT8 *strListType, void *pData, UINT32 uiCnt, INT8 **pstrJson)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	cJSON 		*ptJson = NULL;
	T_ListJsonMng *ptMng = NULL;

	if ((NULL == strListType) || (NULL == pstrJson))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	ptMng = FindJsonListMngNode(ptTable, strListType);
	if (NULL == ptMng)
	{
		SysErr("FindJsonListMngNode failed, list type = %s.\n", strListType);
		return STATE_CODE_OBJECT_NOT_EXIST;
	}

	ptJson = cJSON_CreateArray();
	if (NULL == ptJson)
	{
		return STATE_CODE_ALLOCATION_FAILURE;
	}

	if ((NULL != pData) && (0 != uiCnt))
	{
		//dbprintf("JsonAddObjectArrayWithExternal...\n");
		eCode = JsonAddObjectArrayWithExternal(ptJson, NULL, ptMng->ptKey, pData, uiCnt, ptMng->atExternFiled, MAX_EXTERN_CNT);
		if (!STATE_OK(eCode))
		{
			SysErr("JsonAddObjectArrayWithExternal failed, eCode = %d\n", eCode);
			goto cleanup;
		}
	}
	
	*pstrJson = cJSON_Print(ptJson);

cleanup:
	cJSON_Delete(ptJson);
	return eCode;
}

E_StateCode JsonParseIds(INT8 *strJson, UINT32 **ppData, UINT32 *puiCnt)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	cJSON 		*ptJson = NULL;
	void			*pData = NULL;
	UINT32		uiCnt = 0;
	UINT32		uiAllocSize = 0;
		
	if ((NULL == strJson) || (NULL == ppData) || (NULL == puiCnt))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	ptJson = cJSON_Parse(strJson);
	if (NULL == ptJson)
	{
		SysErr("%s::cJSON_Parse failed, %s.\n", __FUNCTION__, strJson);
		return STATE_CODE_INVALID_HANDLE;
	}

	uiCnt = cJSON_GetArraySize(ptJson);
	if (0 == uiCnt)
	{
		goto cleanup;
	}

	uiAllocSize = sizeof(UINT32) * uiCnt;
	pData = malloc(uiAllocSize);
	if (NULL == pData)
	{
		SysErr("malloc failed!\n");
		eCode = STATE_CODE_ALLOCATION_FAILURE;
		goto cleanup;
	}

	memset(pData, 0x0, uiAllocSize);
	eCode = JsonParseNumberArray(ptJson, NULL, pData, &uiCnt, uiCnt);
	if (!STATE_OK(eCode))
	{
		SysErr("JsonParseNumberArray failed, eCode = %d\n", eCode);
		goto cleanup;
	}
	
cleanup:
	cJSON_Delete(ptJson);
	if (!STATE_OK(eCode))
	{
		if (NULL != pData)
		{
			free(pData);
			pData = NULL;
			uiCnt = 0;
		}
	}
	
	*puiCnt = uiCnt;
	*ppData = pData;
	
	return eCode;
}

E_StateCode JsonMakeIds(UINT32 *pData, UINT32 uiCnt, INT8 **pstrJson)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	cJSON 		*ptJson = NULL;

	if ((NULL == pstrJson) || (NULL == pData))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	ptJson = cJSON_CreateArray();
	if (NULL == ptJson)
	{
		return STATE_CODE_ALLOCATION_FAILURE;
	}

	eCode = JsonAddNumberArray(ptJson, NULL, pData, uiCnt);
	if (!STATE_OK(eCode))
	{
		SysErr("JsonAddNumberArray failed, eCode = %d\n", eCode);
		goto cleanup;
	}
	
	*pstrJson = cJSON_Print(ptJson);

cleanup:
	cJSON_Delete(ptJson);
	return eCode;
}

E_StateCode MParseObjectOrOneElementArray(TlKeyInfo_t *ptKey, INT8 *strJson, void *pData)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	INT32		iRet = 0;
	cJSON 		*ptJson = NULL;
	cJSON		*ptSubJson = NULL;

	if ((NULL == ptKey) || (NULL == strJson) || (NULL == pData))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	ptJson = cJSON_Parse(strJson);
	if (NULL == ptJson)
	{
		SysErr("%s::cJSON_Parse failed, %s.\n", __FUNCTION__, strJson);
		return STATE_CODE_INVALID_PARAM;
	}

	if (cJSON_Array == ptJson->type)
	{
		ptSubJson = cJSON_GetArrayItem(ptJson, 0);
		if (NULL == ptSubJson)
		{
			//SysErr("cJSON_GetArrayItem[0] failed!\n");
			eCode = STATE_CODE_INVALID_PARAM;
			goto cleanup;
		}
	}
	else
	{
		ptSubJson = ptJson;
	}

	iRet = TlpJson2Obj(ptKey, ptSubJson, pData);
	if (0 != iRet)
	{
		SysErr("CJson2Obj fail ret:%d.\n", iRet);
		eCode =  STATE_CODE_INVALID_PARAM;
		goto cleanup;
	}

cleanup:
	if (NULL != ptJson)
	{
		cJSON_Delete(ptJson);
	}
	return eCode;
	
}

E_StateCode JsonParseFieldList(T_ListJsonMng *ptTable, const INT8 *strField, INT8 *strListType, INT8 *strJson, void **ppData, UINT32 *puiCnt)
{
	E_StateCode eCode = STATE_CODE_NO_ERROR;
	cJSON		*ptJson = NULL;
	void			*pData = NULL;
	UINT32		uiCnt = 0;
	UINT32		uiAllocSize = 0;
	T_ListJsonMng *ptMng = NULL;
	cJSON		*ptSubJson = NULL;
		
	if ((NULL == strListType) || (NULL == strJson) || (NULL == ppData) || (NULL == puiCnt))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	ptMng = FindJsonListMngNode(ptTable, strListType);
	if (NULL == ptMng)
	{
		SysErr("FindJsonListMngNode failed, list type = %s.\n", strListType);
		return STATE_CODE_OBJECT_NOT_EXIST;
	}

	ptJson = cJSON_Parse(strJson);
	if (NULL == ptJson)
	{
		SysErr("%s::cJSON_Parse failed, %s.\n", __FUNCTION__, strJson);
		return STATE_CODE_INVALID_PARAM;
	}

	if (NULL == strField)
	{
		ptSubJson = ptJson;
	}
	else
	{
		ptSubJson = cJSON_GetObjectItem(ptJson, strField);
		if (NULL == ptSubJson)
		{
			return STATE_CODE_INVALID_PARAM;
		}
	}

	uiCnt = cJSON_GetArraySize(ptSubJson);
	if (0 == uiCnt)
	{
		goto cleanup;
	}

	uiAllocSize = ptMng->ptKey->csize * uiCnt;
	pData = malloc(uiAllocSize);
	if (NULL == pData)
	{
		SysErr("malloc failed!\n");
		eCode = STATE_CODE_ALLOCATION_FAILURE;
		goto cleanup;
	}

	memset(pData, 0x0, uiAllocSize);
	eCode = JsonParseObjectArray(ptSubJson, NULL, ptMng->ptKey, pData, &uiCnt, uiCnt);
	if (!STATE_OK(eCode))
	{
		SysErr("JsonParseObjectArray failed, eCode = %d\n", eCode);
		goto cleanup;
	}
	
cleanup:
	cJSON_Delete(ptJson);
	if (!STATE_OK(eCode))
	{
		if (NULL != pData)
		{
			free(pData);
			pData = NULL;
			uiCnt = 0;
		}
	}
	
	*puiCnt = uiCnt;
	*ppData = pData;
	
	return eCode;
}

E_StateCode JsonStringStrip(INT8 *pcSrc, INT8 **ppcDst)
{
	E_StateCode eCode = STATE_CODE_NO_ERROR;
	cJSON	*ptJson = NULL;

	if ((NULL == pcSrc) || (NULL == ppcDst))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	ptJson = cJSON_Parse(pcSrc);
	if (NULL == ptJson)
	{
		SysErr("%s::cJSON_Parse failed, %s.\n", __FUNCTION__, pcSrc);
		return STATE_CODE_INVALID_PARAM;
	}

	*ppcDst = cJSON_PrintUnformatted(ptJson);
	
	cJSON_Delete(ptJson);

	return eCode;
}

