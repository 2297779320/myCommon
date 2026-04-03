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

