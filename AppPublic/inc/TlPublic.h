#ifndef _TL_PUBLIC_H_
#define _TL_PUBLIC_H_

#include "typedef.h"
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>
#include "cJSON.h"

/* 状态码定义 */
typedef enum
{
    STATE_CODE_NO_ERROR = 0,
    STATE_CODE_INVALID_HANDLE = -1,
    STATE_CODE_ALLOCATION_FAILURE = -2,
    STATE_CODE_UNDEFINED_ERROR = -3,
    STATE_CODE_OBJECT_NOT_EXIST = -4,
    STATE_CODE_UNABLE_TO_OPEN_FILE = -5,
} E_StateCode;

#define STATE_OK(code) ((code) == STATE_CODE_NO_ERROR)

/* 日志宏定义 */
#define SysErr(fmt, ...)   printf("[ERROR] " fmt, ##__VA_ARGS__)
#define SysLog(fmt, ...)   printf("[LOG] " fmt, ##__VA_ARGS__)
#define dbprintf(fmt, ...) printf("[DEBUG] " fmt, ##__VA_ARGS__)

/* TL Key 类型枚举 */
typedef enum
{
    TL_KEY_TYPE_Invalid = 0,
    TL_KEY_TYPE_U8,
    TL_KEY_TYPE_U16,
    TL_KEY_TYPE_U32,
    TL_KEY_TYPE_S8,
    TL_KEY_TYPE_S16,
    TL_KEY_TYPE_S32,
    TL_KEY_TYPE_String,
    TL_KEY_TYPE_StringPtr,
    TL_KEY_TYPE_Bool,
    TL_KEY_TYPE_Object,
    TL_KEY_TYPE_Array,
} TL_KEY_TYPE_E;

/* TL Key 选项枚举 */
typedef enum
{
    TL_KEY_OptIgnor = 0,
    TL_KEY_OptTrue,
    TL_KEY_OptFalse,
    TL_KEY_OptRequired,
} TL_KEY_OPT_E;

/* TL Key 信息结构 */
typedef struct
{
    TL_KEY_OPT_E    eOpt;
    UINT32          uiStructSize;
    TL_KEY_TYPE_E   eType;
    const INT8      *strKey;
    UINT32          uiOffset;
    UINT32          csize;              /* 字符串大小或结构体大小 */
    INT8            strField[64];
    void            *pReserved;
} TlKeyInfo_t;

/* TL Key 宏定义 */
#define TL_MAKE_OPT_KEY_INFO(opt, type, keytype, key, field, reserved) \
    { \
        .eOpt = (opt), \
        .uiStructSize = sizeof(type), \
        .eType = (keytype), \
        .strKey = (key), \
        .uiOffset = offsetof(type, field), \
        .csize = sizeof(((type *)0)->field), \
        .strField = "", \
        .pReserved = (reserved) \
    }

#define TL_MAKE_END_INFO() \
    { \
        .eOpt = TL_KEY_OptIgnor, \
        .uiStructSize = 0, \
        .eType = TL_KEY_TYPE_Invalid, \
        .strKey = NULL, \
        .uiOffset = 0, \
        .csize = 0, \
        .strField = "", \
        .pReserved = NULL \
    }

/* Forward declaration */
typedef struct _tagListMng T_ListJsonMng;

/* STBP 查询参数结构 */
typedef struct
{
    INT8 *pColumns;
    INT8 *pWhere;
    INT8 *pOrderBy;
    INT32 limit;
    INT32 offset;
    BOOL withTotal;
} TlcItemQueryParams_t;

/* STBP 查询结果结构 */
typedef struct
{
    INT32 total;
    INT32 limit;
    INT32 offset;
    cJSON *pJsonItems;
} TlcItemQueryResult_t;

/* STBP 表信息结构 */
typedef struct
{
    INT8 strTableName[128];
    void *pReserved;
} TlcTopicTableInfo_t;

/* Json 函数声明 (在 JsonEx.c 中实现) */
E_StateCode JsonParseObject(cJSON *ptCjson, const INT8 *strField, TlKeyInfo_t *ptKey, void *pObj);
E_StateCode JsonAddObject(cJSON *ptCjson, const INT8 *strField, TlKeyInfo_t *ptKey, void *pObj);
E_StateCode JsonParseList(T_ListJsonMng *ptTable, INT8 *strListType, INT8 *strJson, void **ppData, UINT32 *puiCnt);
E_StateCode JsonMakeList(T_ListJsonMng *ptTable, INT8 *strListType, void *pData, UINT32 uiCnt, INT8 **pstrJson);
UINT32 JsonGetListElementSize(T_ListJsonMng *ptTable, INT8 *strListType);
E_StateCode JsonParseListElement(T_ListJsonMng *ptTable, INT8 *strListType, INT8 *strJson, void *pData);
E_StateCode JsonParseIds(INT8 *strJson, UINT32 **ppData, UINT32 *puiCnt);
E_StateCode JsonMakeIds(UINT32 *pData, UINT32 uiCnt, INT8 **pstrJson);
void TlpJsonString2Json(TlKeyInfo_t *ptKey, INT8 *strJson, void *pObj);
void TlpObj2Json(TlKeyInfo_t *ptKey, cJSON *ptJson, void *pObj);
void TlpJsonFreePtrMember(TlKeyInfo_t *ptKey, void *pObj);

#endif /* _TL_PUBLIC_H_ */
