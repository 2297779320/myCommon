#ifndef _TLA_STBP_CFG_CT_PUBLIC_H_
#define _TLA_STBP_CFG_CT_PUBLIC_H_

#include "TlPublic.h"
#include <sqlite3.h>

#ifdef __cplusplus
extern "C" {
#endif

/* STBP 句柄结构 */
typedef struct
{
    sqlite3 *pdb;              /* SQLite 数据库句柄 */
    char db_path[512];         /* 数据库路径 */
    int table_count;           /* 表数量 */
    void *pTables;             /* 表信息链表 */
    int initialized;           /* 是否已初始化 */
} TlaStbpHandle_t;

/* 表字段定义 */
typedef struct
{
    char name[128];            /* 字段名 */
    char type[64];             /* 字段类型 (TEXT, INTEGER, REAL) */
    int is_primary_key;        /* 是否主键 */
    int is_not_null;           /* 是否非空 */
    int is_indexed;            /* 是否建索引 */
} TlaStbpField_t;

/* 表信息结构 */
typedef struct TlaStbpTable
{
    char table_name[128];      /* 表名 */
    int field_count;           /* 字段数 */
    TlaStbpField_t fields[64]; /* 字段定义 */
    int has_id;                /* 是否有自增ID */
    struct TlaStbpTable *next; /* 链表指针 */
} TlaStbpTable_t;

/* STBP 查询参数结构 (完善版) */
typedef struct
{
    INT8 *pColumns;            /* 查询列，NULL表示* */
    INT8 *pWhere;              /* WHERE条件 */
    INT8 *pOrderBy;            /* 排序字段 */
    INT32 limit;               /* 限制数量 */
    INT32 offset;              /* 偏移量 */
    BOOL withTotal;            /* 是否返回总数 */
} TlcItemQueryParams_t;

/* STBP 查询结果结构 (完善版) */
typedef struct
{
    INT32 total;               /* 总记录数 */
    INT32 limit;               /* 限制数 */
    INT32 offset;              /* 偏移量 */
    cJSON *pJsonItems;         /* JSON格式结果 */
} TlcItemQueryResult_t;

/* STBP 表信息结构 (完善版) */
typedef struct
{
    INT8 strTableName[128];    /* 表名 */
    INT32 iFieldCnt;           /* 字段数 */
    void *pFields;             /* 字段信息 */
} TlcTopicTableInfo_t;

/**
 * 初始化 STBP 句柄
 * @param db_path 数据库文件路径
 * @return STBP句柄，NULL表示失败
 */
HANDLE TlaStbpInit(const INT8 *db_path);

/**
 * 关闭 STBP 句柄
 * @param hStbp STBP句柄
 */
void TlaStbpClose(HANDLE hStbp);

/**
 * 添加表定义
 * @param hStbp STBP句柄
 * @param ptConf 表配置信息
 * @return 0成功，非0失败
 */
INT32 TlaStbpAddTable(HANDLE hStbp, TlcTopicTableInfo_t *ptConf);

/**
 * 删除表
 * @param hStbp STBP句柄
 * @param strTableKey 表名
 * @return 0成功，非0失败
 */
INT32 TlaStbpDelTable(HANDLE hStbp, INT8 *strTableKey);

/**
 * 添加字符串格式的数据项
 * @param hStbp STBP句柄
 * @param strTableKey 表名
 * @param strJson JSON格式数据
 * @param uiCnt 数据项数量
 * @param puiItemId 返回的项ID数组
 * @return 0成功，非0失败
 */
INT32 TlaStbpAddStrItems(HANDLE hStbp, INT8 *strTableKey, INT8 *strJson, UINT32 uiCnt, INT32 *puiItemId);

/**
 * 更新字符串格式的数据项
 * @param hStbp STBP句柄
 * @param strTableKey 表名
 * @param strJson JSON格式数据
 * @return 0成功，非0失败
 */
INT32 TlaStbpUpdateStrItems(HANDLE hStbp, INT8 *strTableKey, INT8 *strJson);

/**
 * 删除数据项
 * @param hStbp STBP句柄
 * @param strTableKey 表名
 * @param puiIds 要删除的项ID数组
 * @param uiCnt 删除的数量
 * @return 0成功，非0失败
 */
INT32 TlaStbpDelItems(HANDLE hStbp, INT8 *strTableKey, INT32 *puiIds, UINT32 uiCnt);

/**
 * 删除表中所有数据
 * @param hStbp STBP句柄
 * @param strTableKey 表名
 * @return 0成功，非0失败
 */
INT32 TlaStbpDelAllItems(HANDLE hStbp, INT8 *strTableKey);

/**
 * 执行写操作SQL
 * @param hStbp STBP句柄
 * @param strSql SQL语句
 * @return 0成功，非0失败
 */
INT32 TlaStbpExecWtSql(HANDLE hStbp, INT8 *strSql);

/**
 * 执行查询SQL
 * @param hStbp STBP句柄
 * @param strSql SQL语句
 * @param pptJson 返回查询结果的JSON对象
 * @return 0成功，非0失败
 */
INT32 TlaStbpExecQuerySql(HANDLE hStbp, INT8 *strSql, cJSON **pptJson);

/**
 * 查询数据项
 * @param hStbp STBP句柄
 * @param strTableKey 表名
 * @param ptQuery 查询参数
 * @param ptRes 查询结果
 * @return 0成功，非0失败
 */
INT32 TlaStbpQueryItems(HANDLE hStbp, INT8 *strTableKey, TlcItemQueryParams_t *ptQuery, TlcItemQueryResult_t *ptRes);

/**
 * 获取表信息
 * @param hStbp STBP句柄
 * @param strTable 表名
 * @param ptInfo 返回表信息
 * @return 0成功，非0失败
 */
INT32 TlaStbpGetTableInfo(HANDLE hStbp, INT8 *strTable, TlcTopicTableInfo_t *ptInfo);

/**
 * 检查表是否存在
 * @param hStbp STBP句柄
 * @param strTable 表名
 * @return 1存在，0不存在
 */
INT32 TlaStbpTableExists(HANDLE hStbp, INT8 *strTable);

/**
 * 获取最后插入的ROWID
 * @param hStbp STBP句柄
 * @return ROWID
 */
INT64 TlaStbpLastInsertRowid(HANDLE hStbp);

/**
 * 通过STBP消息从远端加载配置
 * @param hStbp STBP连接句柄(StbpClientGetConnectionHandle返回)
 * @param strTopicId Topic标识
 * @param ppcConfig 返回的配置字符串指针
 * @return 0成功，非0失败
 */
INT32 TlaStbpLoadConfig(void *hStbp, const INT8 *strTopicId, INT8 **ppcConfig);

/**
 * 通过STBP消息将配置保存到远端
 * @param hStbp STBP连接句柄(StbpClientGetConnectionHandle返回)
 * @param strTopicId Topic标识
 * @param pcConfig 配置字符串
 * @return 0成功，非0失败
 */
INT32 TlaStbpSaveConfig(void *hStbp, const INT8 *strTopicId, const INT8 *pcConfig);

#ifdef __cplusplus
}
#endif

#endif /* _TLA_STBP_CFG_CT_PUBLIC_H_ */
