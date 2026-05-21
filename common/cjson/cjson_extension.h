/**
 * @file cjson_extension.h
 * @brief cJSON 扩展 -- 结构体与 JSON 的自动映射框架
 *
 * @details
 * 提供 CJsonStructFieldDef 字段描述宏和 cjson_parse_json_to_struct /
 * cjson_encode_struct_to_json 两个核心函数，实现 C 结构体与 JSON 字符串
 * 之间的自动序列化/反序列化。支持嵌套结构体和数组。
 *
 * @see cJSON.h（依赖）
 * @see JsonParse.h, JsonParsePriv.h（被依赖）
 */

#ifndef CJSON_EXTENSION_H
#define CJSON_EXTENSION_H
#include "cJSON.h"
typedef enum {
    CJSON_TYPE_INT = 0,
    CJSON_TYPE_UINT,
    CJSON_TYPE_FLOAT,
    CJSON_TYPE_DOUBLE,
    CJSON_TYPE_STRING,
    CJSON_TYPE_BOOL,
    CJSON_TYPE_STRUCT,
    CJSON_TYPE_ARRAY,
    CJSON_TYPE_MAX,
} CJsonFieldType;
typedef struct CJsonStructFieldDef {
    /* 字段名 */
    const char *name;
    CJsonFieldType type;
    size_t offset;

    /* for CJSON_TYPE_STRUCT */
    size_t struct_size;
    const struct CJsonStructFieldDef *nested_fields;

    /* for CJSON_TYPE_ARRAY */
    size_t array_size;
    size_t element_size;
    const struct CJsonStructFieldDef *element_fields;
    CJsonFieldType element_type;

} CJsonStructFieldDef;

/**
 * @name 字段定义便捷宏
 * @{
 */
/** @brief 基本类型字段（INT/UINT/FLOAT/DOUBLE/BOOL） */
#define CJSON_FIELD(name, type, struc, field) \
    { name, type, offsetof(struc, field), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX }

/** @brief 字符串字段（自动获取缓冲区大小） */
#define CJSON_STRING_FIELD(name, struc, field) \
    { name, CJSON_TYPE_STRING, offsetof(struc, field), sizeof(((struc*)0)->field), NULL, 0, 0, NULL, CJSON_TYPE_MAX }

/** @brief 嵌套结构体字段 */
#define CJSON_STRUCT_FIELD(name, struc, field, nested) \
    { name, CJSON_TYPE_STRUCT, offsetof(struc, field), sizeof(((struc*)0)->field), nested, 0, 0, NULL, CJSON_TYPE_MAX }

/** @brief 基本类型数组字段 */
#define CJSON_ARRAY_FIELD(name, struc, field, elem_type, count) \
    { name, CJSON_TYPE_ARRAY, offsetof(struc, field), 0, NULL, count, sizeof(((struc*)0)->field[0]), NULL, elem_type }

/** @brief 结构体数组字段 */
#define CJSON_STRUCT_ARRAY_FIELD(name, struc, field, elem_fields, count) \
    { name, CJSON_TYPE_ARRAY, offsetof(struc, field), 0, NULL, count, sizeof(((struc*)0)->field[0]), elem_fields, CJSON_TYPE_MAX }

/** @brief 字段表结束标记 */
#define CJSON_FIELD_END \
    { NULL, 0, 0, 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX }
/** @} */

/**
 * @brief 从 JSON 字符串解析到结构体
 * @param[in] json_str  JSON 字符串
 * @param[in] fields    字段描述表
 * @param[in] struct_size 结构体大小
 * @return 新分配的结构体指针，调用方负责 free；失败返回 NULL
 */
void* cjson_parse_json_to_struct(const char *json_str,
                               const CJsonStructFieldDef *fields,
                               size_t struct_size);

/**
 * @brief 将结构体编码为 JSON 字符串
 * @param[in] struct_ptr 结构体指针
 * @param[in] fields     字段描述表
 * @return cJSON 分配的 JSON 字符串，调用方负责 free；失败返回 NULL
 */
char* cjson_encode_struct_to_json(const void *struct_ptr,
                                 const CJsonStructFieldDef *fields);
#endif