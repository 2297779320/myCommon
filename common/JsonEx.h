/**
 * @file JsonEx.h
 * @brief JSON 扩展工具函数 -- 字符串/数值/数组/ID 的 JSON 序列化与反序列化
 *
 * @details
 * 提供 cJSON 的便捷封装：添加字符串、解析/创建数值数组、解析/生成 ID 列表、
 * JSON 字符串去转义等。
 *
 * @see defs.h（依赖 E_StateCode, INT8, UINT32）
 * @see cJSON.h（依赖）
 * @see JsonParse.h, JsonParsePriv.h（被依赖）
 */

#ifndef JSONEX_H
#define JSONEX_H


/**************************************************************************
 *                         头文件引用                                     *
 **************************************************************************/

#include "defs.h"
#include "cJSON.h"

EXTERN_C_BLOCK
/**************************************************************************
 *                        ��������                                   *
 **************************************************************************/

/**************************************************************************
 *                        �꺯������                                 *
 **************************************************************************/


/**************************************************************************
 *                         ��������                                    *
 **************************************************************************/

/**************************************************************************
 *                        ȫ�ֺ���                                *
 **************************************************************************/


E_StateCode JsonAddString(cJSON *ptCjson, const INT8 *strField, const INT8 *strValue);

E_StateCode JsonParseNumberArray(cJSON *ptCjson, const INT8 *strField, UINT32 *puiData, UINT32 *puiItems, UINT32 uiMaxItems);
E_StateCode JsonAddNumberArray(cJSON *ptCjson, const INT8 *strField, UINT32 *puiData, UINT32 uiItems);

E_StateCode JsonParseIds(INT8 *strJson, UINT32 **ppData, UINT32 *puiCnt);
E_StateCode JsonMakeIds(UINT32 *pData, UINT32 uiCnt, INT8 **pstrJson);

E_StateCode JsonStringStrip(INT8 *pcSrc, INT8 **ppcDst);

EXTERN_C_BLOCK_END

#endif // JSONEX_H
	

