#ifndef _H_JSONEX_
#define _H_JSONEX_



/**************************************************************************
 *                         头文件引用                                     *
 **************************************************************************/

#include "defs.h"
#include "cJSON.h"

EXTERN_C_BLOCK
/**************************************************************************
 *                        常量定义                                   *
 **************************************************************************/

/**************************************************************************
 *                        宏函数定义                                 *
 **************************************************************************/


/**************************************************************************
 *                         数据类型                                    *
 **************************************************************************/

/**************************************************************************
 *                        全局函数                                *
 **************************************************************************/


E_StateCode JsonAddString(cJSON *ptCjson, const INT8 *strField, const INT8 *strValue);

E_StateCode JsonParseNumberArray(cJSON *ptCjson, const INT8 *strField, UINT32 *puiData, UINT32 *puiItems, UINT32 uiMaxItems);
E_StateCode JsonAddNumberArray(cJSON *ptCjson, const INT8 *strField, UINT32 *puiData, UINT32 uiItems);

E_StateCode JsonParseIds(INT8 *strJson, UINT32 **ppData, UINT32 *puiCnt);
E_StateCode JsonMakeIds(UINT32 *pData, UINT32 uiCnt, INT8 **pstrJson);

E_StateCode JsonStringStrip(INT8 *pcSrc, INT8 **ppcDst);

EXTERN_C_BLOCK_END

#endif
	

