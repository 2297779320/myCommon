/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

/* cJSON version 1.7.15 - Simplified implementation for ListOpt */

#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Internal structure */
typedef struct {
    const unsigned char *content;
    size_t length;
    size_t offset;
} parse_buffer;

typedef struct {
    char *buffer;
    size_t length;
    size_t offset;
    size_t depth;
} print_buffer;

/* Static hooks */
static cJSON_Hooks global_hooks = {
    .malloc_fn = malloc,
    .free_fn = free
};

void cJSON_InitHooks(cJSON_Hooks* hooks)
{
    if (hooks == NULL)
    {
        global_hooks.malloc_fn = malloc;
        global_hooks.free_fn = free;
        return;
    }
    
    if (hooks->malloc_fn)
        global_hooks.malloc_fn = hooks->malloc_fn;
    if (hooks->free_fn)
        global_hooks.free_fn = hooks->free_fn;
}

void *cJSON_malloc(size_t size)
{
    return global_hooks.malloc_fn(size);
}

void cJSON_free(void *ptr)
{
    global_hooks.free_fn(ptr);
}

cJSON *cJSON_CreateNull(void)
{
    cJSON *item = (cJSON *)cJSON_malloc(sizeof(cJSON));
    if (item)
    {
        memset(item, 0, sizeof(cJSON));
        item->type = cJSON_NULL;
    }
    return item;
}

cJSON *cJSON_CreateTrue(void)
{
    cJSON *item = (cJSON *)cJSON_malloc(sizeof(cJSON));
    if (item)
    {
        memset(item, 0, sizeof(cJSON));
        item->type = cJSON_True;
    }
    return item;
}

cJSON *cJSON_CreateFalse(void)
{
    cJSON *item = (cJSON *)cJSON_malloc(sizeof(cJSON));
    if (item)
    {
        memset(item, 0, sizeof(cJSON));
        item->type = cJSON_False;
    }
    return item;
}

cJSON *cJSON_CreateBool(cJSON_bool boolean)
{
    return boolean ? cJSON_CreateTrue() : cJSON_CreateFalse();
}

cJSON *cJSON_CreateNumber(double num)
{
    cJSON *item = (cJSON *)cJSON_malloc(sizeof(cJSON));
    if (item)
    {
        memset(item, 0, sizeof(cJSON));
        item->type = cJSON_Number;
        item->valuedouble = num;
        item->valueint = (int)num;
    }
    return item;
}

cJSON *cJSON_CreateString(const char *string)
{
    cJSON *item = (cJSON *)cJSON_malloc(sizeof(cJSON));
    if (item)
    {
        memset(item, 0, sizeof(cJSON));
        item->type = cJSON_String;
        if (string)
        {
            item->valuestring = (char *)cJSON_malloc(strlen(string) + 1);
            if (item->valuestring)
                strcpy(item->valuestring, string);
        }
    }
    return item;
}

cJSON *cJSON_CreateArray(void)
{
    cJSON *item = (cJSON *)cJSON_malloc(sizeof(cJSON));
    if (item)
    {
        memset(item, 0, sizeof(cJSON));
        item->type = cJSON_Array;
    }
    return item;
}

cJSON *cJSON_CreateObject(void)
{
    cJSON *item = (cJSON *)cJSON_malloc(sizeof(cJSON));
    if (item)
    {
        memset(item, 0, sizeof(cJSON));
        item->type = cJSON_Object;
    }
    return item;
}

void cJSON_Delete(cJSON *item)
{
    if (item == NULL)
        return;
    
    if (item->child)
    {
        cJSON *child = item->child;
        cJSON *next = child->next;
        while (child)
        {
            next = child->next;
            cJSON_Delete(child);
            child = next;
        }
    }
    
    if (item->valuestring)
        cJSON_free(item->valuestring);
    if (item->string)
        cJSON_free(item->string);
    
    cJSON_free(item);
}

void cJSON_AddItemToArray(cJSON *array, cJSON *item)
{
    if (array == NULL || item == NULL)
        return;
    
    cJSON *child = array->child;
    if (child == NULL)
    {
        array->child = item;
        item->prev = item;
        item->next = item;
    }
    else
    {
        item->prev = child->prev;
        item->next = child;
        child->prev->next = item;
        child->prev = item;
    }
}

void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item)
{
    if (item)
    {
        item->string = (char *)string;
        cJSON_AddItemToArray(object, item);
    }
}

cJSON *cJSON_GetObjectItem(const cJSON * const object, const char * const string)
{
    if (object == NULL || string == NULL || object->type != cJSON_Object)
        return NULL;
    
    cJSON *child = object->child;
    if (child == NULL)
        return NULL;
    
    cJSON *current = child;
    do
    {
        if (current->string && strcmp(current->string, string) == 0)
            return current;
        current = current->next;
    } while (current != child);
    
    return NULL;
}

cJSON *cJSON_GetArrayItem(const cJSON *array, int index)
{
    if (array == NULL || array->type != cJSON_Array || array->child == NULL)
        return NULL;
    
    cJSON *child = array->child;
    for (int i = 0; i < index && child->next != array->child; i++)
    {
        child = child->next;
    }
    
    return (index >= 0) ? child : NULL;
}

/* Simple JSON printer */
static int print_value(const cJSON *item, char *buffer, int buffer_len, int indent_level);

static int print_string(const char *str, char *buffer, int buffer_len)
{
    if (str == NULL)
        return snprintf(buffer, buffer_len, "null");
    
    int len = 2; /* quotes */
    len += strlen(str);
    
    if (buffer && buffer_len > 0)
    {
        snprintf(buffer, buffer_len, "\"%s\"", str);
    }
    
    return len;
}

static int print_value(const cJSON *item, char *buffer, int buffer_len, int indent_level)
{
    if (item == NULL)
        return 0;
    
    switch (item->type & 0xFF)
    {
        case cJSON_NULL:
            return snprintf(buffer, buffer_len, "null");
        
        case cJSON_True:
            return snprintf(buffer, buffer_len, "true");
        
        case cJSON_False:
            return snprintf(buffer, buffer_len, "false");
        
        case cJSON_Number:
            if (item->valuedouble == (int)item->valuedouble)
                return snprintf(buffer, buffer_len, "%d", item->valueint);
            else
                return snprintf(buffer, buffer_len, "%f", item->valuedouble);
        
        case cJSON_String:
            return print_string(item->valuestring, buffer, buffer_len);
        
        case cJSON_Array:
        {
            int len = 1; /* [ */
            if (buffer && buffer_len > 0)
                snprintf(buffer, buffer_len, "[");
            
            cJSON *child = item->child;
            if (child)
            {
                cJSON *start = child;
                do
                {
                    if (child != start)
                    {
                        len += snprintf(buffer ? buffer + len : NULL, 
                                       buffer_len > len ? buffer_len - len : 0, ",");
                    }
                    len += print_value(child, buffer ? buffer + len : NULL,
                                      buffer_len > len ? buffer_len - len : 0, indent_level + 1);
                    child = child->next;
                } while (child != start);
            }
            
            len += snprintf(buffer ? buffer + len : NULL,
                           buffer_len > len ? buffer_len - len : 0, "]");
            return len;
        }
        
        case cJSON_Object:
        {
            int len = 1; /* { */
            if (buffer && buffer_len > 0)
                snprintf(buffer, buffer_len, "{");
            
            cJSON *child = item->child;
            if (child)
            {
                cJSON *start = child;
                do
                {
                    if (child != start)
                    {
                        len += snprintf(buffer ? buffer + len : NULL,
                                       buffer_len > len ? buffer_len - len : 0, ",");
                    }
                    len += print_string(child->string, buffer ? buffer + len : NULL,
                                       buffer_len > len ? buffer_len - len : 0);
                    len += snprintf(buffer ? buffer + len : NULL,
                                   buffer_len > len ? buffer_len - len : 0, ":");
                    len += print_value(child, buffer ? buffer + len : NULL,
                                      buffer_len > len ? buffer_len - len : 0, indent_level + 1);
                    child = child->next;
                } while (child != start);
            }
            
            len += snprintf(buffer ? buffer + len : NULL,
                           buffer_len > len ? buffer_len - len : 0, "}");
            return len;
        }
        
        default:
            return 0;
    }
}

char *cJSON_Print(const cJSON *item)
{
    if (item == NULL)
        return NULL;
    
    /* First pass: calculate length */
    int len = print_value(item, NULL, 0, 0) + 1;
    
    char *buffer = (char *)cJSON_malloc(len);
    if (buffer == NULL)
        return NULL;
    
    /* Second pass: print to buffer */
    print_value(item, buffer, len, 0);
    buffer[len - 1] = '\0';
    
    return buffer;
}

/* Simple JSON parser */
static const unsigned char *skip_whitespace(const unsigned char *in)
{
    while (in && *in && (*in == ' ' || *in == '\t' || *in == '\n' || *in == '\r'))
        in++;
    return in;
}

static const unsigned char *parse_string(cJSON *item, const unsigned char *input)
{
    if (*input != '\"')
        return NULL;
    
    input++; /* skip opening quote */
    const unsigned char *start = input;
    
    /* Find closing quote */
    while (*input && *input != '\"')
    {
        if (*input == '\\')
            input++; /* skip escaped char */
        input++;
    }
    
    if (*input != '\"')
        return NULL;
    
    size_t len = input - start;
    item->valuestring = (char *)cJSON_malloc(len + 1);
    if (item->valuestring == NULL)
        return NULL;
    
    strncpy(item->valuestring, (const char *)start, len);
    item->valuestring[len] = '\0';
    item->type = cJSON_String;
    
    return input + 1; /* skip closing quote */
}

static const unsigned char *parse_number(cJSON *item, const unsigned char *input)
{
    char *end;
    item->valuedouble = strtod((const char *)input, &end);
    item->valueint = (int)item->valuedouble;
    item->type = cJSON_Number;
    return (const unsigned char *)end;
}

static const unsigned char *parse_value(cJSON *item, const unsigned char *input);

static const unsigned char *parse_array(cJSON *item, const unsigned char *input)
{
    item->type = cJSON_Array;
    input++; /* skip [ */
    input = skip_whitespace(input);
    
    if (*input == ']')
        return input + 1; /* empty array */
    
    do
    {
        cJSON *child = (cJSON *)cJSON_malloc(sizeof(cJSON));
        if (child == NULL)
            return NULL;
        memset(child, 0, sizeof(cJSON));
        
        input = parse_value(child, input);
        if (input == NULL)
        {
            cJSON_Delete(child);
            return NULL;
        }
        
        cJSON_AddItemToArray(item, child);
        input = skip_whitespace(input);
        
        if (*input == ',')
        {
            input++;
            input = skip_whitespace(input);
        }
    } while (*input != ']');
    
    return input + 1; /* skip ] */
}

static const unsigned char *parse_object(cJSON *item, const unsigned char *input)
{
    item->type = cJSON_Object;
    input++; /* skip { */
    input = skip_whitespace(input);
    
    if (*input == '}')
        return input + 1; /* empty object */
    
    do
    {
        /* Parse key */
        if (*input != '\"')
            return NULL;
        
        cJSON *child = (cJSON *)cJSON_malloc(sizeof(cJSON));
        if (child == NULL)
            return NULL;
        memset(child, 0, sizeof(cJSON));
        
        input = parse_string(child, input);
        if (input == NULL)
        {
            cJSON_Delete(child);
            return NULL;
        }
        
        child->string = child->valuestring;
        child->valuestring = NULL;
        
        input = skip_whitespace(input);
        if (*input != ':')
        {
            cJSON_Delete(child);
            return NULL;
        }
        input++; /* skip : */
        input = skip_whitespace(input);
        
        /* Parse value */
        input = parse_value(child, input);
        if (input == NULL)
        {
            cJSON_Delete(child);
            return NULL;
        }
        
        cJSON_AddItemToArray(item, child);
        input = skip_whitespace(input);
        
        if (*input == ',')
        {
            input++;
            input = skip_whitespace(input);
        }
    } while (*input != '}');
    
    return input + 1; /* skip } */
}

static const unsigned char *parse_value(cJSON *item, const unsigned char *input)
{
    input = skip_whitespace(input);
    
    if (*input == '\"')
        return parse_string(item, input);
    
    if (*input == '-' || (*input >= '0' && *input <= '9'))
        return parse_number(item, input);
    
    if (*input == '[')
        return parse_array(item, input);
    
    if (*input == '{')
        return parse_object(item, input);
    
    if (strncmp((const char *)input, "true", 4) == 0)
    {
        item->type = cJSON_True;
        return input + 4;
    }
    
    if (strncmp((const char *)input, "false", 5) == 0)
    {
        item->type = cJSON_False;
        return input + 5;
    }
    
    if (strncmp((const char *)input, "null", 4) == 0)
    {
        item->type = cJSON_NULL;
        return input + 4;
    }
    
    return NULL;
}

cJSON *cJSON_Parse(const char *value)
{
    if (value == NULL)
        return NULL;
    
    cJSON *item = (cJSON *)cJSON_malloc(sizeof(cJSON));
    if (item == NULL)
        return NULL;
    
    memset(item, 0, sizeof(cJSON));
    
    const unsigned char *input = (const unsigned char *)value;
    input = skip_whitespace(input);
    
    if (parse_value(item, input) == NULL)
    {
        cJSON_Delete(item);
        return NULL;
    }
    
    return item;
}

cJSON_bool cJSON_IsString(const cJSON * const item)
{
    return (item != NULL && (item->type & cJSON_String));
}

cJSON_bool cJSON_IsNumber(const cJSON * const item)
{
    return (item != NULL && (item->type & cJSON_Number));
}

cJSON_bool cJSON_IsArray(const cJSON * const item)
{
    return (item != NULL && (item->type & cJSON_Array));
}

cJSON_bool cJSON_IsObject(const cJSON * const item)
{
    return (item != NULL && (item->type & cJSON_Object));
}

cJSON_bool cJSON_IsNull(const cJSON * const item)
{
    return (item != NULL && (item->type & cJSON_NULL));
}

cJSON_bool cJSON_IsTrue(const cJSON * const item)
{
    return (item != NULL && (item->type & cJSON_True));
}

cJSON_bool cJSON_IsFalse(const cJSON * const item)
{
    return (item != NULL && (item->type & cJSON_False));
}

int cJSON_GetArraySize(const cJSON *array)
{
    if (array == NULL || array->child == NULL)
        return 0;
    
    int count = 1;
    cJSON *child = array->child->next;
    while (child != array->child)
    {
        count++;
        child = child->next;
    }
    return count;
}
