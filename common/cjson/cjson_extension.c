#include "cjson_extension.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "defs.h"

static E_StateCode cjson_parse_json_to_struct_internal(cJSON *json_item,
                                                       const CJsonStructFieldDef *fields,
                                                       void *struct_ptr,
                                                       size_t struct_size);

static cJSON *encode_struct_to_json_internal(const void *struct_ptr,
                                             const CJsonStructFieldDef *fields);

static E_StateCode parse_json_field(cJSON *json_item, void *field_ptr,
                                    const CJsonStructFieldDef *field_def)
{

    E_StateCode eCode = STATE_CODE_NO_ERROR;
    if (!json_item || !field_ptr || !field_def)
        return STATE_CODE_INVALID_PARAM;

    /* 调用方传入的 field_ptr 已是字段地址，不要再次加 offset */
    switch (field_def->type)
    {
    case CJSON_TYPE_INT:
        *(int *)field_ptr = json_item->valueint;
        break;
    case CJSON_TYPE_UINT:
        *(unsigned int *)field_ptr = (unsigned int)json_item->valuedouble;
        break;
    case CJSON_TYPE_FLOAT:
        *(float *)field_ptr = (float)json_item->valuedouble;
        break;
    case CJSON_TYPE_DOUBLE:
        *(double *)field_ptr = json_item->valuedouble;
        break;
    case CJSON_TYPE_STRING:
    {
        // 自动计算字符数组大小
        size_t array_size = field_def->struct_size;
        if (array_size == 0)
        {
            // 如果没有指定大小，尝试通过下一个字段的偏移量计算
            const CJsonStructFieldDef *next_field = field_def + 1;
            if (next_field && next_field->name)
            {
                array_size = next_field->offset - field_def->offset;
            }
        }
        char *dest = (char *)field_ptr;
        if (array_size == 0)
        {
            dest[0] = '\0';
            break;
        }
        strncpy(dest, json_item->valuestring, array_size - 1);
        dest[array_size - 1] = '\0'; // 确保以NULL结尾
        break;
    }
    case CJSON_TYPE_BOOL:
        *(int *)field_ptr = cJSON_IsTrue(json_item);
        break;
    case CJSON_TYPE_STRUCT:
    {
        if (!json_item->child)
            return STATE_CODE_INVALID_PARAM;
        void *nested_struct = (void *)field_ptr;
        eCode = cjson_parse_json_to_struct_internal(json_item,
                                                    field_def->nested_fields,
                                                    nested_struct,
                                                    field_def->struct_size);
        return eCode;
    }
    case CJSON_TYPE_ARRAY:
    {
        if (!json_item->child)
            return STATE_CODE_INVALID_PARAM;
        cJSON *array_item = json_item->child;
        size_t count = 0;
        while (array_item && count < field_def->array_size)
        {
            void *element_ptr = (char *)field_ptr + count * field_def->element_size;
            if (field_def->element_fields)
            {
                eCode = cjson_parse_json_to_struct_internal(array_item,
                                                            field_def->element_fields,
                                                            element_ptr,
                                                            field_def->element_size);
                if (!STATE_OK(eCode))
                    return eCode;
            }
            else
            {
                switch (field_def->element_type)
                {
                case CJSON_TYPE_INT:
                    *(int *)element_ptr = array_item->valueint;
                    break;
                case CJSON_TYPE_UINT:
                    *(unsigned int *)element_ptr = (unsigned int)array_item->valuedouble;
                    break;
                case CJSON_TYPE_FLOAT:
                    *(float *)element_ptr = (float)array_item->valuedouble;
                    break;
                case CJSON_TYPE_DOUBLE:
                    *(double *)element_ptr = array_item->valuedouble;
                    break;
                case CJSON_TYPE_BOOL:
                    *(int *)element_ptr = cJSON_IsTrue(array_item);
                    break;
                default:
                    eCode = STATE_CODE_INVALID_PARAM;
                    break;
                }
            }
            array_item = array_item->next;
            count++;
        }
        break;
    }
    default:
        return STATE_CODE_INVALID_PARAM;
    }
    return eCode;
}

static cJSON *encode_field_to_json(const void *struct_ptr,
                                   const CJsonStructFieldDef *field_def)
{
    if (!struct_ptr || !field_def)
        return NULL;

    void *field_ptr = (void *)struct_ptr;
    cJSON *json_item = NULL;

    switch (field_def->type)
    {
    case CJSON_TYPE_INT:
        json_item = cJSON_CreateNumber(*(int *)field_ptr);
        break;
    case CJSON_TYPE_UINT:
        json_item = cJSON_CreateNumber(*(unsigned int *)field_ptr);
        break;
    case CJSON_TYPE_FLOAT:
        json_item = cJSON_CreateNumber(*(float *)field_ptr);
        break;
    case CJSON_TYPE_DOUBLE:
        json_item = cJSON_CreateNumber(*(double *)field_ptr);
        break;
    case CJSON_TYPE_STRING:
    {
        char *str = (char *)field_ptr;
        json_item = cJSON_CreateString(str);
        break;
    }
    case CJSON_TYPE_BOOL:
        json_item = cJSON_CreateBool(*(int *)field_ptr);
        break;
    case CJSON_TYPE_STRUCT:
    {
        json_item = cJSON_CreateObject();
        if (json_item && field_def->nested_fields)
        {
            cJSON *nested_json = encode_struct_to_json_internal(field_ptr, field_def->nested_fields);
            if (nested_json)
            {
                cJSON_Delete(json_item);
                json_item = nested_json;
            }
        }
        break;
    }
    case CJSON_TYPE_ARRAY:
    {
        size_t i = 0;
        json_item = cJSON_CreateArray();
        for (i = 0; i < field_def->array_size; i++)
        {
            void *element_ptr = (char *)field_ptr + i * field_def->element_size;
            cJSON *element_json = NULL;
            if (field_def->element_fields)
            {
                element_json = encode_struct_to_json_internal(element_ptr, field_def->element_fields);
            }
            else
            {
                switch (field_def->element_type)
                {
                case CJSON_TYPE_INT:
                    element_json = cJSON_CreateNumber(*(int *)element_ptr);
                    break;
                case CJSON_TYPE_UINT:
                    element_json = cJSON_CreateNumber(*(unsigned int *)element_ptr);
                    break;
                case CJSON_TYPE_FLOAT:
                    element_json = cJSON_CreateNumber(*(float *)element_ptr);
                    break;
                case CJSON_TYPE_DOUBLE:
                    element_json = cJSON_CreateNumber(*(double *)element_ptr);
                    break;
                case CJSON_TYPE_STRING:
                    element_json = cJSON_CreateString((char *)element_ptr);
                    break;
                case CJSON_TYPE_BOOL:
                    element_json = cJSON_CreateBool(*(int *)element_ptr);
                    break;
                default:
                    break;
                }
            }
            if (element_json)
            {
                cJSON_AddItemToArray(json_item, element_json);
            }
        }
        break;
    }
    default:
        return NULL;
    }
    return json_item;
}
static E_StateCode cjson_parse_json_to_struct_internal(cJSON *json_item,
                                                       const CJsonStructFieldDef *fields,
                                                       void *struct_ptr,
                                                       size_t struct_size)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    cJSON *root = json_item;
    if (!root)
        return STATE_CODE_INVALID_PARAM;

    memset(struct_ptr, 0, struct_size);
    const CJsonStructFieldDef *field = fields;

    while (field->name)
    {
        cJSON *item = cJSON_GetObjectItemCaseSensitive(root, field->name);
        if (item)
        {
            void *field_ptr = (char *)struct_ptr + field->offset;
            eCode = parse_json_field(item, field_ptr, field);
            if (!STATE_OK(eCode))
            {
                return eCode;
            }
        }
        field++;
    }

    return eCode;
}
static cJSON *encode_struct_to_json_internal(const void *struct_ptr,
                                             const CJsonStructFieldDef *fields)
{
    cJSON *root = cJSON_CreateObject();
    if (!root)
        return NULL;

    const CJsonStructFieldDef *field = fields;
    while (field->name)
    {
        cJSON *item = encode_field_to_json((const char *)struct_ptr + field->offset, field);
        if (item)
        {
            cJSON_AddItemToObject(root, field->name, item);
        }
        field++;
    }

    return root;
}
void* cjson_parse_json_to_struct(const char *json_str,
                                 const CJsonStructFieldDef *fields,
                                 size_t struct_size)
{
    cJSON *ptJson = NULL;
    void *struct_ptr = NULL;
    E_StateCode eCode = STATE_CODE_NO_ERROR;

    ptJson = cJSON_Parse(json_str);
    if (NULL == ptJson)
    {
        syserr("cJSON_Parse failed, %s.\n", json_str);
        return NULL;
    }

    struct_ptr = malloc(struct_size);
    if (!struct_ptr)
        goto end;

    eCode = cjson_parse_json_to_struct_internal(ptJson, fields, struct_ptr, struct_size);
    if (!STATE_OK(eCode))
    {
        free(struct_ptr);
        struct_ptr = NULL;
    }

end:
    cJSON_Delete(ptJson);
    return struct_ptr;
}
char *cjson_encode_struct_to_json(const void *struct_ptr,
                                  const CJsonStructFieldDef *fields)
{
    cJSON *root = encode_struct_to_json_internal(struct_ptr, fields);
    if (!root)
        return NULL;

    char *result = cJSON_Print(root);
    cJSON_Delete(root);
    return result;
}