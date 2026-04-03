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
void* cjson_parse_json_to_struct(const char *json_str, 
                               const CJsonStructFieldDef *fields,
                               size_t struct_size);
char* cjson_encode_struct_to_json(const void *struct_ptr,
                                 const CJsonStructFieldDef *fields);
#endif