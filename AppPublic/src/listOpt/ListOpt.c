/**************************************************************************
* ��    Ȩ��Copyright (c) 2015
* �ļ����ƣ�ListOpt.c
* �ļ���ʶ���б������ļ�
* ����ժҪ�� 
* ����˵����
* ��ǰ�汾�� 
* ��    �ߣ�̷����
* ������ڣ�2019��3�� 4��
*
* �޸ļ�¼1	��
*	�޸����ڣ�
*	�� �� �ţ�
*	�� �� �ˣ�
*	�޸����ݣ�
**************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../inc/listOpt/ListOpt.h"

/***********************************************************
 *						��������		                       		*
 **********************************************************/

/***********************************************************
 *				�ļ��ڲ�ʹ�õĺ�                      *
 **********************************************************/
#define SQL_CAN_PRINT(strSql) (NULL == strchr(strSql, '%'))
	
 /***********************************************************
 *			�ļ��ڲ�ʹ�õ��������� 	*
 **********************************************************/

/***********************************************************
 *						ȫ�ֱ���						*
 **********************************************************/
TlKeyInfo_t g_tListQueryKey[] =
{
	TL_MAKE_OPT_KEY_INFO(TL_KEY_OptIgnor, TlcItemQueryParams_t, TL_KEY_TYPE_StringPtr, "columns", pColumns, NULL),
	TL_MAKE_OPT_KEY_INFO(TL_KEY_OptIgnor, TlcItemQueryParams_t, TL_KEY_TYPE_StringPtr, "where", pWhere, NULL),
	TL_MAKE_OPT_KEY_INFO(TL_KEY_OptIgnor, TlcItemQueryParams_t, TL_KEY_TYPE_StringPtr, "orderBy", pOrderBy, NULL),
	TL_MAKE_OPT_KEY_INFO(TL_KEY_OptIgnor, TlcItemQueryParams_t, TL_KEY_TYPE_S32, "limit", limit, NULL),
	TL_MAKE_OPT_KEY_INFO(TL_KEY_OptIgnor, TlcItemQueryParams_t, TL_KEY_TYPE_S32, "offset", offset, NULL),
	TL_MAKE_OPT_KEY_INFO(TL_KEY_OptIgnor, TlcItemQueryParams_t, TL_KEY_TYPE_Bool, "withTotal", withTotal, NULL),

	TL_MAKE_END_INFO()
};

TlKeyInfo_t g_tListResultKey[] =
{
	TL_MAKE_OPT_KEY_INFO(TL_KEY_OptIgnor, TlcItemQueryResult_t, TL_KEY_TYPE_S32, "total", total, NULL),
	TL_MAKE_OPT_KEY_INFO(TL_KEY_OptIgnor, TlcItemQueryResult_t, TL_KEY_TYPE_S32, "limit", limit, NULL),
	TL_MAKE_OPT_KEY_INFO(TL_KEY_OptIgnor, TlcItemQueryResult_t, TL_KEY_TYPE_S32, "offset", offset, NULL),

	TL_MAKE_END_INFO()
};

/***********************************************************
 *						���ر���						*
 **********************************************************/

/***********************************************************
 * 						���غ���						*
 **********************************************************/

/***********************************************************
 * 						ȫ�ֺ���						*
 **********************************************************/
/**********************************************************************
* �������ƣ�ListCreateTable
* ������������������
* ���������hStbp - ͨ�ž��
					strTableKey -��������
* ���������
* �� �� ֵ��	 ״̬��
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
E_StateCode ListCreateTable(HANDLE hStbp, INT8 *strTableKey, T_TableConf *ptConf)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	INT32		iRet = 0;

	if ((NULL == hStbp) || (NULL == strTableKey) || (NULL == ptConf))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	
	iRet = TlaStbpAddTable(hStbp, ptConf);
	if (0 != iRet)
	{
		SysErr("TlaStbpAddTable failed, iRet = %d\n", iRet);
		return STATE_CODE_UNABLE_TO_OPEN_FILE;
	}
	
	return eCode;
}

/**********************************************************************
* �������ƣ�ListDeleteTable
* ����������ɾ������
* ���������hStbp - ͨ�ž��
					strTableKey -��������
* ���������
* �� �� ֵ��	 ״̬��
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
E_StateCode ListDeleteTable(HANDLE hStbp, INT8 *strTableKey)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;

	if ((NULL == hStbp) || (NULL == strTableKey))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	TlaStbpDelTable(hStbp, strTableKey);
	
	return eCode;
}

/**********************************************************************
* �������ƣ�ListAddItems2
* �����������ڱ�����������Ŀ
* ���������hStbp - ͨ�ž��
					ptListMng -�б������ṹ
					strTableKey 	- ��������
					strParseKey - �����ؼ���
					pData - �����ӵ�����
					uiCnt - ��Ŀ��
* ���������pstrResJson - ���д������JSON
* �� �� ֵ��	 ״̬��
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
E_StateCode ListAddItems2(HANDLE hStbp, T_ListJsonMng *ptListMng, INT8 *strTableKey, INT8 *strParseKey, void *pData, UINT32 uiCnt, INT8 **pstrResJson)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	INT8		*pcJson = NULL;
	UINT32	uiElementSize = 0;
	UINT32	*puiItemId = 0;
	UINT32	*ptr = NULL;
	INT32	iLoop = 0;
	INT32	iRet = 0;

	if ((NULL == hStbp) || (NULL == ptListMng) || (NULL == strTableKey) || (NULL == pData))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	if (NULL == strParseKey)
	{
		strParseKey = strTableKey;
	}
	
	eCode = JsonMakeList(ptListMng, strParseKey, pData, uiCnt, &pcJson);
	if (!STATE_OK(eCode))
	{
		SysErr("JsonMakeList failed, eCode = %d\n", eCode);
		goto cleanup;
	}

	puiItemId = malloc(uiCnt * sizeof(UINT32));
	if (NULL == puiItemId)
	{
		SysErr("malloc failed!\n");
		return STATE_CODE_ALLOCATION_FAILURE;
	}

	dbprintf("ListAddItems2 %s\n", pcJson);
	/* �洢�����ݿ�*/
	iRet = TlaStbpAddStrItems(hStbp, strTableKey, pcJson, uiCnt, (int *)puiItemId);
	if (0 != iRet)
	{
		SysErr("TlaStbpAddStrItems failed, iRet = %d, pcJson = %s\n", iRet, pcJson);
		eCode = STATE_CODE_UNDEFINED_ERROR;
		goto cleanup;
	}

	free(pcJson);
	pcJson = NULL;

	if (uiCnt > 1)
	{
		uiElementSize = JsonGetListElementSize(ptListMng, strParseKey);
		for (iLoop = 0; iLoop < uiCnt; iLoop++)
		{
			ptr = (UINT32 *)((UINT8 *)pData + uiElementSize * iLoop);
			*ptr = *(puiItemId + iLoop);
		}
	}
	else
	{
		ptr = (UINT32 *)pData;
		*ptr = *puiItemId;
	}

	if (NULL != pstrResJson)
	{
		JsonMakeList(ptListMng, strParseKey, pData, uiCnt, pstrResJson);
	}
	
cleanup:
	if (NULL != pcJson)
	{
		free(pcJson);
	}

	if (NULL != puiItemId)
	{
		free(puiItemId);
	}
	return eCode;
}

/**********************************************************************
* �������ƣ�ListAddItemsByJson2
* �����������ڱ�����������Ŀ
* ���������hStbp - ͨ�ž��
					ptListMng -�б������ṹ
					strTableKey 	- ��������
					strParseKey - �����ؼ���
					strJson - ��������json
* ���������pstrResJson - ���д������JSON
					ppData - �����������
					puiCnt - ��Ŀ��
* �� �� ֵ��	 ״̬��
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
E_StateCode ListAddItemsByJson2(HANDLE hStbp, T_ListJsonMng *ptListMng, INT8 *strTableKey, INT8 *strParseKey, INT8 *strJson, INT8 **pstrResJson, void **ppData, UINT32 *puiCnt)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	void		*pData = NULL;
	UINT32	uiCnt = 0;
	BOOL	bKeepData = SMP_FALSE;

	if ((NULL == hStbp) || (NULL == ptListMng) || (NULL == strTableKey) || (NULL == strJson))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	if (NULL == strParseKey)
	{
		strParseKey = strTableKey;
	}
	
	eCode = JsonParseList(ptListMng, strParseKey, strJson, &pData, &uiCnt);
	if (!STATE_OK(eCode))
	{
		SysErr("JsonParseList failed, eCode = %d\n", eCode);
		return eCode;
	}

	eCode = ListAddItems2(hStbp, ptListMng, strTableKey, strParseKey, pData, uiCnt, pstrResJson);
	if (!STATE_OK(eCode))
	{
		goto cleanup;
	}

	if ((NULL != ppData) && (NULL != puiCnt))
	{
		*ppData = pData;
		*puiCnt = uiCnt;
		bKeepData = SMP_TRUE;
	}
	
cleanup:
	if ((NULL != pData) && (!bKeepData))
	{
		free(pData);
	}

	return eCode;
}

/**********************************************************************
* �������ƣ�ListUpdateItems
* �������������±�����Ŀ
* ���������hStbp - ͨ�ž��
					ptListMng -�б������ṹ
					strTableKey 	- ��������
					strParseKey - �����ؼ���
					pData - �����ӵ�����
					uiCnt - ��Ŀ��
* ���������pstrResJson - ����JSON
* �� �� ֵ��	 ״̬��
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
E_StateCode ListUpdateItems2(HANDLE hStbp, T_ListJsonMng *ptListMng, INT8 *strTableKey, INT8 *strParseKey, void *pData, UINT32 uiCnt)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	INT8		*pcJson = NULL;
	INT32	iRet = 0;

	if ((NULL == hStbp) || (NULL == ptListMng) || (NULL == strTableKey) || (NULL == pData))
	{
		return STATE_CODE_INVALID_HANDLE;
	}
	
	if (NULL == strParseKey)
	{
		strParseKey = strTableKey;
	}	
	
	eCode = JsonMakeList(ptListMng, strParseKey, pData, uiCnt, &pcJson);
	if (!STATE_OK(eCode))
	{
		SysErr("JsonMakeList failed, eCode = %d\n", eCode);
		goto cleanup;
	}
	
	/* �洢�����ݿ�*/
	iRet = TlaStbpUpdateStrItems(hStbp, strTableKey, pcJson);
	if (0 != iRet)
	{
		SysLog("TlaStbpUpdateStrItems failed, iRet = %d, try to add items!!!\n", iRet);
		iRet = TlaStbpAddStrItems(hStbp, strTableKey, pcJson, uiCnt, NULL);
		if (0 != iRet)
		{
			SysErr("TlaStbpAddStrItems failed, iRet = %d\n", iRet);
			eCode = STATE_CODE_UNDEFINED_ERROR;
			goto cleanup;
		}
	}
	
	free(pcJson);
	pcJson = NULL;
	
cleanup:
	if (NULL != pcJson)
	{
		free(pcJson);
	}
	
	return eCode;
}

/**********************************************************************
* �������ƣ�ListUpdateItemsByJson
* �������������±�����Ŀ
* ���������hStbp - ͨ�ž��
					ptListMng -�б������ṹ
					strTableKey 	- ��������
					strJson - ��������json
* ���������ppData - �����������
					puiCnt - ��Ŀ��
* �� �� ֵ��	 ״̬��
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
E_StateCode ListUpdateItemsByJson2(HANDLE hStbp, T_ListJsonMng *ptListMng, INT8 *strTableKey, INT8 *strParseKey, INT8 *strJson, void **ppData, UINT32 *puiCnt)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	void		*pData = NULL;
	UINT32	uiCnt = 0;
	BOOL	bKeepData = SMP_FALSE;

	if ((NULL == hStbp) || (NULL == ptListMng) || (NULL == strTableKey) || (NULL == strJson))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	if (NULL == strParseKey)
	{
		strParseKey = strTableKey;
	}	
	
	eCode = JsonParseList(ptListMng, strParseKey, strJson, &pData, &uiCnt);
	if (!STATE_OK(eCode))
	{
		SysErr("JsonParseList failed, eCode = %d\n", eCode);
		return eCode;
	}

	eCode = ListUpdateItems2(hStbp, ptListMng, strTableKey, strParseKey, pData, uiCnt);
	if (!STATE_OK(eCode))
	{
		goto cleanup;
	}

	if ((NULL != ppData) && (NULL != puiCnt))
	{
		*ppData = pData;
		*puiCnt = uiCnt;
		bKeepData = SMP_TRUE;
	}
	
cleanup:
	if ((NULL != pData) && (!bKeepData))
	{
		free(pData);
	}

	return eCode;
}

/**********************************************************************
* �������ƣ�ListAddItems
* �����������ڱ�����������Ŀ
* ���������hStbp - ͨ�ž��
					ptListMng -�б������ṹ
					strTableKey 	- ��������
					pData - �����ӵ�����
					uiCnt - ��Ŀ��
* ���������pstrResJson - ���д������JSON
* �� �� ֵ��	 ״̬��
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
E_StateCode ListAddItems(HANDLE hStbp, T_ListJsonMng *ptListMng, INT8 *strTableKey, void *pData, UINT32 uiCnt, INT8 **pstrResJson)
{
	return ListAddItems2(hStbp, ptListMng, strTableKey, NULL, pData, uiCnt, pstrResJson);
}

/**********************************************************************
* �������ƣ�ListAddItemsByJson
* �����������ڱ�����������Ŀ
* ���������hStbp - ͨ�ž��
					ptListMng -�б������ṹ
					strTableKey 	- ��������
					strJson - ��������json
* ���������pstrResJson - ���д������JSON
					ppData - �����������
					puiCnt - ��Ŀ��
* �� �� ֵ��	 ״̬��
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
E_StateCode ListAddItemsByJson(HANDLE hStbp, T_ListJsonMng *ptListMng, INT8 *strTableKey, INT8 *strJson, INT8 **pstrResJson, void **ppData, UINT32 *puiCnt)
{
	return ListAddItemsByJson2(hStbp, ptListMng, strTableKey, NULL, strJson, pstrResJson, ppData, puiCnt);
}

/**********************************************************************
* �������ƣ�ListUpdateItems
* �������������±�����Ŀ
* ���������hStbp - ͨ�ž��
					ptListMng -�б������ṹ
					strTableKey 	- ��������
					pData - �����ӵ�����
					uiCnt - ��Ŀ��
* ���������pstrResJson - ����JSON
* �� �� ֵ��	 ״̬��
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
E_StateCode ListUpdateItems(HANDLE hStbp, T_ListJsonMng *ptListMng, INT8 *strTableKey, void *pData, UINT32 uiCnt)
{
	return ListUpdateItems2(hStbp, ptListMng, strTableKey, NULL, pData, uiCnt);
}

/**********************************************************************
* �������ƣ�ListUpdateItemsByJson
* �������������±�����Ŀ
* ���������hStbp - ͨ�ž��
					ptListMng -�б������ṹ
					strTableKey 	- ��������
					strJson - ��������json
* ���������ppData - �����������
					puiCnt - ��Ŀ��
* �� �� ֵ��	 ״̬��
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
E_StateCode ListUpdateItemsByJson(HANDLE hStbp, T_ListJsonMng *ptListMng, INT8 *strTableKey, INT8 *strJson, void **ppData, UINT32 *puiCnt)
{
	return ListUpdateItemsByJson2(hStbp, ptListMng, strTableKey, NULL, strJson, ppData, puiCnt);
}

/**********************************************************************
* �������ƣ�ListUpdateItemBySql
* ����������ͨ��sql�����±���
* ���������hStbp - ͨ�ž��
					strSql
* �����������
* �� �� ֵ��	 ״̬��
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
E_StateCode ListUpdateItemBySql(HANDLE hStbp, INT8 *strSql)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	INT32		iRet = 0;

	if ((NULL == hStbp) || (NULL == strSql))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	if (SQL_CAN_PRINT(strSql))
	{
		dbprintf("Execute sql %s\n", strSql);
	}
	
	iRet = TlaStbpExecWtSql(hStbp, strSql);
	if (0 != iRet)
	{
		if (SQL_CAN_PRINT(strSql))
		{
			SysErr("Execute sql failed, iRet = %d, [%s]\n", iRet, strSql);
		}
		return STATE_CODE_UNDEFINED_ERROR;
	}
	return eCode;
}

/**********************************************************************
* �������ƣ�ListDeleteItems
* ����������ɾ��������Ŀ
* ���������hStbp - ͨ�ž��
					strTableKey - ��������
					puiIds - �����ӵ���ĿID
					uiCnt - ��Ŀ��
* ���������pstrResJson - ����JSON
* �� �� ֵ��	 ״̬��
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
E_StateCode ListDeleteItems(HANDLE hStbp, INT8 *strTableKey, UINT32 *puiIds, UINT32 uiCnt)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	INT8		*pcJson = NULL;
	INT32		iRet = 0;

	if ((NULL == hStbp) || (NULL == strTableKey) || (NULL == puiIds))
	{
		return STATE_CODE_INVALID_HANDLE;
	}
	
	eCode = JsonMakeIds(puiIds, uiCnt, &pcJson);
	if (!STATE_OK(eCode))
	{
		SysErr("JsonMakeList failed, eCode = %d\n", eCode);
		goto cleanup;
	}
	
	/* �����ݿ���ɾ��*/
	iRet = TlaStbpDelItems(hStbp, strTableKey, (int *)puiIds, uiCnt);
	if (0 != iRet)
	{
		SysErr("TlaStbpDelItems failed, iRet = %d\n", iRet);
		eCode = STATE_CODE_UNDEFINED_ERROR;
		goto cleanup;
	}
	
	free(pcJson);
	pcJson = NULL;
	
cleanup:
	if (NULL != pcJson)
	{
		free(pcJson);
	}
	
	return eCode;
}

/**********************************************************************
* �������ƣ�ListUpdateItemsByJson
* ����������ɾ��������Ŀ
* ���������hStbp - ͨ�ž��
					strTableKey -��������
					strJson - ��������json
* ���������puiData - �����������
					puiCnt - ��Ŀ��
* �� �� ֵ��	 ״̬��
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
E_StateCode ListDeleteItemsByJson(HANDLE hStbp, INT8 *strTableKey, INT8 *strJson, UINT32 **ppuiData, UINT32 *puiCnt)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	UINT32		*puiData = NULL;
	UINT32	uiCnt = 0;
	BOOL	bKeepData = SMP_FALSE;

	if ((NULL == hStbp) || (NULL == strTableKey) || (NULL == strJson))
	{
		return STATE_CODE_INVALID_HANDLE;
	}
	
	eCode = JsonParseIds(strJson, &puiData, &uiCnt);
	if (!STATE_OK(eCode))
	{
		SysErr("JsonParseIds failed, eCode = %d\n", eCode);
		return eCode;
	}

	eCode = ListDeleteItems(hStbp, strTableKey, puiData, uiCnt);
	if (!STATE_OK(eCode))
	{
		goto cleanup;
	}

	if ((NULL != ppuiData) && (NULL != puiCnt))
	{
		*ppuiData = puiData;
		*puiCnt = uiCnt;
		bKeepData = SMP_TRUE;
	}
	
cleanup:
	if ((NULL != puiData) && (!bKeepData))
	{
		free(puiData);
	}

	return eCode;
}

/**********************************************************************
* �������ƣ�ListDeleteItemsByConditon
* ����������������ɾ��������Ŀ
* ���������hStbp - ͨ�ž��
					strTableKey -��������
					strWhere - ��������
* �����������
* �� �� ֵ��	 ״̬��
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
E_StateCode ListDeleteItemsByConditon(HANDLE hStbp, INT8 *strTableKey, INT8 *strWhere)
{
	E_StateCode 	   eCode = STATE_CODE_NO_ERROR;
	//INT32				 iRet = 0;
	INT8						*pcCur = NULL;
	INT8				*pcSql = NULL;
	UINT32				  uiLen = 0;
	
	if ((NULL == hStbp) || (NULL == strTableKey))
	{
			return STATE_CODE_INVALID_HANDLE;
	}

	uiLen = strlen(strWhere) + strlen(strTableKey) + 32;
	pcSql = malloc(uiLen);
	if (NULL == pcSql)
	{
			SysErr("malloc failed!\n");
			return STATE_CODE_ALLOCATION_FAILURE;
	}

	pcCur = strWhere;
	if (0 == strnicmp(pcCur, "where", 5))
	{
			pcCur += 6;
	}

	sprintf(pcSql, "delete from %s where %s", strTableKey, pcCur);
	
	eCode = ListUpdateItemBySql(hStbp, pcSql);

	free(pcSql);

	return eCode;
}


/**********************************************************************
* �������ƣ�ListDeleteItemsByCb
* ����������ͨ���ص�ɾ������
* ���������strJson -ɾ����ĿJSON
					pfDeleteItem - ��Ŀɾ������
* ���������
* �� �� ֵ��	 ״̬��
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
E_StateCode ListDeleteItemsByCb(INT8 *strJson, FListItemDelete pfDeleteItem)
{
	//T_MDevAgent	*ptPrivate = pPrivate;
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	UINT32		*puiId = NULL;
	UINT32		uiCnt = 0;
	INT32		iLoop = 0;

	if ((NULL == strJson) || (NULL == pfDeleteItem))
	{
		return STATE_CODE_INVALID_HANDLE;
	}
	
	eCode = JsonParseIds(strJson, &puiId, &uiCnt);
	if (!STATE_OK(eCode))
	{
		SysErr("JsonParseIds failed, eCode = %d\n", eCode);
		return eCode;
	}
	
	for (iLoop = 0; iLoop < uiCnt; iLoop++)
	{
		eCode |= pfDeleteItem(puiId[iLoop]);
	}
	
	if (NULL != puiId)
	{
		free(puiId);
	}
	
	return eCode;
}

/**********************************************************************
* �������ƣ�ListClear
* �����������������
* ���������hStbp - ͨ�ž��
					strTableKey -��������
* ���������
* �� �� ֵ��	 ״̬��
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
E_StateCode ListClear(HANDLE hStbp, INT8 *strTableKey)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	INT32		iRet = 0;

	if ((NULL == hStbp) || (NULL == strTableKey))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	iRet = TlaStbpDelAllItems(hStbp, strTableKey);
	if (0 != iRet)
	{
		SysErr("TlaStbpDelAllItems failed, iRet = %d\n", iRet);
		eCode = STATE_CODE_UNDEFINED_ERROR;
		return eCode;
	}
	
	return eCode;
}

/**********************************************************************
* �������ƣ�ListQueryItemsByJson
* ������������ѯ����
* ���������hStbp - ͨ�ž��
					strTableKey -��������
					strJson - strJson
* ���������pstrRes - ����JSON
* �� �� ֵ��	 ״̬��
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
E_StateCode ListQueryItemsByJson(HANDLE hStbp, INT8 *strTableKey, INT8 *strJson, INT8 **pstrRes)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	INT32		iRet = 0;
	TlcItemQueryParams_t	tQuery;
	TlcItemQueryResult_t	tRes;
	cJSON			*ptJson = NULL;
	
	if ((NULL == hStbp) || (NULL == strTableKey) || (NULL == strJson) || (NULL == pstrRes))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	memset(&tQuery, 0x0, sizeof(tQuery));
	memset(&tRes, 0x0, sizeof(tRes));
	TlpJsonString2Obj(g_tListQueryKey, strJson, &tQuery);
	iRet = TlaStbpQueryItems(hStbp, strTableKey, &tQuery, &tRes);
	if (0 != iRet)
	{
		SysErr("TlaStbpQueryItems failed, iRet = %d, %s\n", iRet, strJson);
		eCode = STATE_CODE_UNDEFINED_ERROR;
		goto cleanup;
	}

	ptJson = cJSON_CreateObject();
	if (NULL == ptJson)
	{
		SysErr("cJSON_CreateObject failed!\n");
		eCode = STATE_CODE_ALLOCATION_FAILURE;
		goto cleanup;
	}

	
	TlpObj2Json(g_tListResultKey, ptJson, &tRes);
	cJSON_AddItemToObject(ptJson, "items", tRes.pJsonItems);

	*pstrRes = cJSON_Print(ptJson);
	
cleanup:
	TlpJsonFreePtrMember(g_tListQueryKey, &tQuery);
	if (NULL != ptJson)
	{
		cJSON_Delete(ptJson);
	}
	else
	{
		if (NULL != tRes.pJsonItems)
		{
			cJSON_Delete(tRes.pJsonItems);
		}
	}
	return eCode;
}

/**********************************************************************
* �������ƣ�ListQueryItemsByCondition
* ������������ѯ����
* ���������hStbp - ͨ�ž��
					strSql -��ѯ���
					uiOffset - ��ѯƫ��
					uiCnt - ��ѯ��Ŀ
* ���������pstrRes - ����JSON
* �� �� ֵ��	 ״̬��
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
E_StateCode ListQueryItemsByCondition(HANDLE hStbp, INT8 *strSql, UINT32 uiOffset, UINT32 uiCnt, INT8 **pstrRes)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	INT32		iRet = 0;
	INT32		iLen = 0;
	INT8			*pcSubmit = NULL;
	cJSON		*ptJson = NULL;

	if ((NULL == hStbp) || (NULL == strSql))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	iLen = strlen(strSql);
	pcSubmit = malloc(iLen + 64);
	if (NULL == pcSubmit)
	{
		return STATE_CODE_ALLOCATION_FAILURE;
	}
	
	memset(pcSubmit, 0x0, iLen + 64);
	
	strcpy(pcSubmit, strSql);
	if (uiOffset > 0)
	{
		if (uiCnt == 0)
		{
			uiCnt = MAX_QUERY_CNT;
		}
		
		sprintf(pcSubmit + iLen, " limit %d", uiCnt);
		iLen = strlen(pcSubmit);
		sprintf(pcSubmit + iLen, " offset %d", uiOffset);
		iLen = strlen(pcSubmit);
	}
	else
	{
		if (uiCnt > 0)
		{
			sprintf(pcSubmit + iLen, " limit %d", uiCnt);
		}
	}

	//dbprintf("Execute sql %s\n", pcSubmit);
	iRet = TlaStbpExecQuerySql(hStbp, pcSubmit, &ptJson);
	if (0 != iRet)
	{
		if (SQL_CAN_PRINT(pcSubmit))
		{
			SysErr("TlaStbpExecQuerySql failed, iRet = %d, [%s]\n", iRet, pcSubmit);
		}
		eCode = STATE_CODE_UNDEFINED_ERROR;
		goto cleanup;
	}

	*pstrRes = cJSON_Print(ptJson);

	//dbprintf("\n%s\n", *pstrRes);

cleanup:
	if (NULL != pcSubmit)
	{
		free(pcSubmit);
	}

	if (NULL != ptJson)
	{
		cJSON_Delete(ptJson);
	}
	return eCode;
}


/**********************************************************************
* �������ƣ�ListQueryItemsCntByCondition
* ������������ѯ������¼����Ŀ
* ���������hStbp - ͨ�ž��
					strTable -��������
					strWhere - ����, ����Ϊ��
* �����������
* �� �� ֵ��	 ��ѯ������Ŀ
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
UINT32 ListQueryItemsCntByCondition(HANDLE hStbp, INT8 *strTable, INT8 *strWhere)
{
	//E_StateCode	eCode = STATE_CODE_NO_ERROR;
	INT32		iRet = 0;
	INT32		iLen = 0;
	INT8			*pcSubmit = NULL;
	cJSON		*ptJson = NULL;
	cJSON		*ptSubJson = NULL;
	cJSON		*ptItemJson = NULL;
	UINT32		uiCnt = 0;

	if ((NULL == hStbp) || (NULL == strTable))
	{
		return 0;
	}

	iLen = strlen(strTable);
	if (NULL != strWhere)
	{
		iLen += strlen(strWhere);
	}

	pcSubmit = malloc(iLen + 64);
	memset(pcSubmit, 0x0, iLen + 64);
	if (NULL == pcSubmit)
	{
		return 0;
	}
	sprintf(pcSubmit, "select count(*) as count from %s ", strTable);
	if (NULL != strWhere)
	{
		if (0 != strnicmp(strWhere, "where", 5))
		{
			strcat(pcSubmit, "where ");
		}
		strcat(pcSubmit, strWhere);
	}

	//dbprintf("Execute sql %s\n", pcSubmit);
	iRet = TlaStbpExecQuerySql(hStbp, pcSubmit, &ptJson);
	if (0 != iRet)
	{
		if (SQL_CAN_PRINT(pcSubmit))
		{
			SysErr("TlaStbpExecQuerySql failed, iRet = %d, [%s]\n", iRet, pcSubmit);
		}
		//eCode = STATE_CODE_UNDEFINED_ERROR;
		goto cleanup;
	}

	ptSubJson = cJSON_GetArrayItem(ptJson, 0);
	if (NULL == ptSubJson)
	{
		goto cleanup;
	}

	ptItemJson = cJSON_GetObjectItem(ptSubJson, "count");
	if (NULL != ptItemJson)
	{
		uiCnt = ptItemJson->valueint;
		dbprintf("uiCnt = %d\n", uiCnt);
	}
	//dbprintf("\n%s\n", *pstrRes);

cleanup:
	if (NULL != pcSubmit)
	{
		free(pcSubmit);
	}

	if (NULL != ptJson)
	{
		cJSON_Delete(ptJson);
	}
	return uiCnt;
}

/**********************************************************************
* �������ƣ�ListCheckExist
* ������������ѯ������¼�Ƿ����
* ���������hStbp - ͨ�ž��
					strTable -��������
					strWhere - ����, ����Ϊ��
* �����������
* �� �� ֵ��	 ��ѯ������Ŀ
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
BOOL ListCheckItemExist(HANDLE hStbp, INT8 *strTable, INT8 *strWhere)
{
	//E_StateCode	eCode = STATE_CODE_NO_ERROR;
	INT32		iRet = 0;
	INT32		iLen = 0;
	INT8			*pcSubmit = NULL;
	cJSON		*ptJson = NULL;
	cJSON		*ptSubJson = NULL;
	BOOL		bExist = SMP_FALSE;

	if ((NULL == hStbp) || (NULL == strTable))
	{
		return 0;
	}

	iLen = strlen(strTable);
	if (NULL != strWhere)
	{
		iLen += strlen(strWhere);
	}

	pcSubmit = malloc(iLen + 64);
	memset(pcSubmit, 0x0, iLen + 64);
	if (NULL == pcSubmit)
	{
		return 0;
	}
	sprintf(pcSubmit, "select * from %s ", strTable);
	if (NULL != strWhere)
	{
		if (0 != strnicmp(strWhere, "where", 5))
		{
			strcat(pcSubmit, "where ");
		}
		strcat(pcSubmit, strWhere);
	}

	strcat(pcSubmit, " limit 1");

	//dbprintf("Execute sql %s\n", pcSubmit);
	iRet = TlaStbpExecQuerySql(hStbp, pcSubmit, &ptJson);
	if (0 != iRet)
	{
		if (SQL_CAN_PRINT(pcSubmit))
		{
			SysErr("TlaStbpExecQuerySql failed, iRet = %d, [%s]\n", iRet, pcSubmit);
		}
		//eCode = STATE_CODE_UNDEFINED_ERROR;
		goto cleanup;
	}

	ptSubJson = cJSON_GetArrayItem(ptJson, 0);
	if (NULL == ptSubJson)
	{
		goto cleanup;
	}

	bExist = SMP_TRUE;

cleanup:
	if (NULL != pcSubmit)
	{
		free(pcSubmit);
	}

	if (NULL != ptJson)
	{
		cJSON_Delete(ptJson);
	}
	return bExist;
}

/**********************************************************************
* �������ƣ�ListQueryAndParseList
* ������������ѯ�������б�, �����������ݷŻش���STATE_CODE_OBJECT_NOT_EXIST
* ���������hStbp - ͨ�ž��
					ptListMng -�б������ṹ
					strTableKey 	- ��������
					strParseKey - �����ؼ���
					strSql - ��ѯ���
* ���������ppData - 
					puiCnt - 
* �� �� ֵ��	 ״̬��
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
E_StateCode ListQueryAndParseList(HANDLE hStbp, T_ListJsonMng *ptListMng, INT8 *strParseKey, INT8 *strSql, void **ppData, UINT32 *puiCnt)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	INT32		iRet = 0;
	cJSON		*ptJson = NULL;
	INT8			*pcRes = NULL;

	if ((NULL == hStbp) || (NULL == strSql) || (NULL == ppData) || (NULL == puiCnt))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	iRet = TlaStbpExecQuerySql(hStbp, strSql, &ptJson);
	if (0 != iRet)
	{
		if (SQL_CAN_PRINT(strSql))
		{
			SysErr("TlaStbpExecQuerySql failed, iRet = %d, [%s]\n", iRet, strSql);
		}
		eCode = STATE_CODE_UNDEFINED_ERROR;
		goto cleanup;
	}

	
	pcRes = cJSON_Print(ptJson);
	if (NULL != pcRes)
	{
		eCode = JsonParseList(ptListMng, strParseKey, pcRes, ppData, puiCnt);
		if (!STATE_OK(eCode))
		{
			//SysErr("JsonParseList failed, eCode = %d\n", eCode);
			return eCode;
		}

		if (0 == *puiCnt)
		{
			eCode = STATE_CODE_OBJECT_NOT_EXIST;
		}
	}
	//dbprintf("\n%s\n", *pstrRes);

cleanup:
	if (NULL != pcRes)
	{
		free(pcRes);
	}

	if (NULL != ptJson)
	{
		cJSON_Delete(ptJson);
	}
	return eCode;
}

/**********************************************************************
* �������ƣ�ListQueryAndParseList
* ������������ѯ������������¼
* ���������hStbp - ͨ�ž��
					ptListMng -�б������ṹ
					strTableKey 	- ��������
					strParseKey - �����ؼ���
					strSql - ��ѯ���
* ���������pData -
* �� �� ֵ��	 ״̬��
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
E_StateCode ListQueryAndParseElement(HANDLE hStbp, T_ListJsonMng *ptListMng, INT8 *strParseKey, INT8 *strSql, void *pData)
{
	E_StateCode	eCode = STATE_CODE_NO_ERROR;
	INT32		iRet = 0;
	cJSON		*ptJson = NULL;
	INT8			*pcRes = NULL;

	if ((NULL == hStbp) || (NULL == strSql) || (NULL == pData))
	{
		return STATE_CODE_INVALID_HANDLE;
	}

	//dbprintf("%s\n", strSql);
	iRet = TlaStbpExecQuerySql(hStbp, strSql, &ptJson);
	if (0 != iRet)
	{
		if (SQL_CAN_PRINT(strSql))
		{
			SysErr("TlaStbpExecQuerySql failed, iRet = %d, [%s]\n", iRet, strSql);
		}
		eCode = STATE_CODE_UNDEFINED_ERROR;
		goto cleanup;
	}

	
	pcRes = cJSON_Print(ptJson);
	if (NULL != pcRes)
	{
		//dbprintf("%s\n", pcRes);
		eCode = JsonParseListElement(ptListMng, strParseKey, pcRes, pData);
		if (!STATE_OK(eCode))
		{
			//SysErr("JsonParseListElement failed, eCode = %d\n", eCode);
			goto cleanup;
		}
	}
	//dbprintf("\n%s\n", *pstrRes);

cleanup:
	if (NULL != pcRes)
	{
		free(pcRes);
	}

	if (NULL != ptJson)
	{
		cJSON_Delete(ptJson);
	}
	return eCode;
}

/**********************************************************************
* �������ƣ�ListCheckTableExist
* ������������ѯ�����Ƿ����
* ���������hStbp - ͨ�ž��
					strTable 	- ��������
* ���������
* �� �� ֵ��	 BOOL
* ����˵���� 
* �޸�����        �汾��     �޸���	      �޸�����
* -----------------------------------------------
* 2019/03/13	     V1.0	           tanrp
***********************************************************************/
BOOL ListCheckTableExist(HANDLE hStbp, INT8 *strTable)
{
	INT32		iRet = 0;
	TlcTopicTableInfo_t		tInfo;

	if ((NULL == hStbp) || (NULL == strTable))
	{
		return SMP_FALSE;
	}
	
	iRet = TlaStbpGetTableInfo(hStbp, strTable, &tInfo);
	if (0 == iRet)
	{
		return SMP_TRUE;
	}

	return SMP_FALSE;
}
