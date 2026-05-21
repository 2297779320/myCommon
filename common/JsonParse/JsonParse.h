/**
 * @file JsonParse.h
 * @brief JSON 解析/生成接口 -- 通用结构体与 JSON 的双向转换
 *
 * @details
 * 提供 MediaParseListElement / MediaMakeListElement 两个入口函数，
 * 根据列表类型名查找对应的字段映射表，完成结构体与 JSON 字符串的转换。
 *
 * @see defs.h（依赖 E_StateCode, INT8）
 * @see cjson_extension.h（依赖 CJsonStructFieldDef）
 */

#ifndef JSONPARSE_H
#define JSONPARSE_H

EXTERN_C_BLOCK

/**************************************************************************
 *                         ͷ�ļ�����                                     *
 **************************************************************************/
#include "defs.h"
#include "cjson_extension.h"
/**************************************************************************
 *                        ��������                                   *
 **************************************************************************/

/**************************************************************************
 *                        �꺯������                                 *
 **************************************************************************/

/**************************************************************************
 *                         ��������                                    *
 **************************************************************************/
typedef struct _tagListMng
{
	INT8				*strListType;
	CJsonStructFieldDef		*ptKey;
    UINT32 uiSize;      
}T_ListJsonMng;

/**************************************************************************
 *                        ȫ�ֺ���                                *
 **************************************************************************/


/**********************************************************************
 * �������ƣ�MediaParseListElement
 * ���������������б�Ԫ��
 * ���������strListType  - �б�Ԫ������ strjson - JSON�ַ���  pData - �б�Ԫ�����ݽṹָ��
 * �����������
 * �� �� ֵ��    ״̬��
 * ����˵����
 * �޸�����        �汾��     �޸���        �޸�����
 * -----------------------------------------------
 * 2022/12/27        V1.0              chengjiahao
 ***********************************************************************/
E_StateCode MediaParseListElement(INT8 *strListType, INT8 *strJson, void *pData);

/**********************************************************************
 * �������ƣ�MediaMakeListElement
 * ���������������б�Ԫ��
 * ���������strListType  - �б�Ԫ������ pData - �б�Ԫ�����ݽṹָ�� pstrJson - JSON�ַ���ָ��
 * �����������
 * �� �� ֵ��    ״̬��
 * ����˵����
 * �޸�����        �汾��     �޸���        �޸�����
 * -----------------------------------------------
 * 2022/12/27        V1.0              chengjiahao
 ***********************************************************************/
E_StateCode MediaMakeListElement(INT8 *strListType, void *pData, INT8 **pstrJson);



/*private*/
E_StateCode JsonParseListElement(T_ListJsonMng *ptTable, INT8 *strListType, INT8 *strJson, void *pData);
E_StateCode JsonMakeListElement(T_ListJsonMng *ptTable, INT8 *strListType, void *pData, INT8 **pstrJson);

EXTERN_C_BLOCK_END

#endif // JSONPARSE_H
	

