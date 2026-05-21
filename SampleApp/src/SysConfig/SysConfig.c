/**************************************************************************
*  Copyright (c) 2024
*  File Name: SysConfig.c
*  Description: System configuration loading/saving using JsonFile module
**************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "typedef.h"
#include "osal.h"
#include "global.h"
#include "SampleApp.h"
#include "SysConfig.h"
#include "JsonFile.h"
#include "cJSON.h"

static BOOL g_bConfigLoaded = SMP_FALSE;

/***********************************************************
*                    Config Structure                      *
**********************************************************/
typedef struct {
    INT8    strConfigPath[256];
    INT8    strServerIp[64];
    INT32   iServerPort;
    INT8    strDeviceName[64];
    BOOL    bDebugMode;
    INT32   iHeartbeatInterval;
} T_SysConfig;

static T_SysConfig g_stSysConfig = {0};

/***********************************************************
*                    Internal Functions                    *
**********************************************************/

/**
 * @brief 从 JSON 对象中解析配置字段
 */
static void SysConfigParseJson(cJSON *ptJson)
{
    cJSON *ptItem = NULL;

    if (NULL == ptJson) return;

    /* 解析 server.ip */
    ptItem = cJSON_GetObjectItem(ptJson, "server");
    if (ptItem)
    {
        cJSON *ptSub = cJSON_GetObjectItem(ptItem, "ip");
        if (ptSub && cJSON_IsString(ptSub))
        {
            strncpy(g_stSysConfig.strServerIp, ptSub->valuestring, sizeof(g_stSysConfig.strServerIp) - 1);
        }

        ptSub = cJSON_GetObjectItem(ptItem, "port");
        if (ptSub && cJSON_IsNumber(ptSub))
        {
            g_stSysConfig.iServerPort = ptSub->valueint;
        }
    }

    /* 解析 device.name */
    ptItem = cJSON_GetObjectItem(ptJson, "device");
    if (ptItem)
    {
        cJSON *ptSub = cJSON_GetObjectItem(ptItem, "name");
        if (ptSub && cJSON_IsString(ptSub))
        {
            strncpy(g_stSysConfig.strDeviceName, ptSub->valuestring, sizeof(g_stSysConfig.strDeviceName) - 1);
        }
    }

    /* 解析 debug.enable */
    ptItem = cJSON_GetObjectItem(ptJson, "debug");
    if (ptItem)
    {
        cJSON *ptSub = cJSON_GetObjectItem(ptItem, "enable");
        if (ptSub && cJSON_IsBool(ptSub))
        {
            g_stSysConfig.bDebugMode = ptSub->valueint ? TRUE : FALSE;
        }
    }

    /* 解析 heartbeat.interval */
    ptItem = cJSON_GetObjectItem(ptJson, "heartbeat");
    if (ptItem)
    {
        cJSON *ptSub = cJSON_GetObjectItem(ptItem, "interval");
        if (ptSub && cJSON_IsNumber(ptSub))
        {
            g_stSysConfig.iHeartbeatInterval = ptSub->valueint;
        }
    }
}

/**
 * @brief 将配置字段构建为 JSON 对象
 */
static cJSON* SysConfigBuildJson(void)
{
    cJSON *ptJson = NULL;
    cJSON *ptSub = NULL;

    ptJson = cJSON_CreateObject();
    if (NULL == ptJson) return NULL;

    /* server 对象 */
    ptSub = cJSON_CreateObject();
    cJSON_AddStringToObject(ptSub, "ip", g_stSysConfig.strServerIp);
    cJSON_AddNumberToObject(ptSub, "port", g_stSysConfig.iServerPort);
    cJSON_AddItemToObject(ptJson, "server", ptSub);

    /* device 对象 */
    ptSub = cJSON_CreateObject();
    cJSON_AddStringToObject(ptSub, "name", g_stSysConfig.strDeviceName);
    cJSON_AddItemToObject(ptJson, "device", ptSub);

    /* debug 对象 */
    ptSub = cJSON_CreateObject();
    cJSON_AddBoolToObject(ptSub, "enable", g_stSysConfig.bDebugMode ? 1 : 0);
    cJSON_AddItemToObject(ptJson, "debug", ptSub);

    /* heartbeat 对象 */
    ptSub = cJSON_CreateObject();
    cJSON_AddNumberToObject(ptSub, "interval", g_stSysConfig.iHeartbeatInterval);
    cJSON_AddItemToObject(ptJson, "heartbeat", ptSub);

    return ptJson;
}

/**
 * @brief 初始化默认配置
 */
static void SysConfigInitDefaults(void)
{
    strncpy(g_stSysConfig.strConfigPath, "configs/topic_data/sample.v1/sys.json",
            sizeof(g_stSysConfig.strConfigPath) - 1);
    strncpy(g_stSysConfig.strServerIp, "127.0.0.1", sizeof(g_stSysConfig.strServerIp) - 1);
    g_stSysConfig.iServerPort = 18300;
    strncpy(g_stSysConfig.strDeviceName, "sample-device", sizeof(g_stSysConfig.strDeviceName) - 1);
    g_stSysConfig.bDebugMode = FALSE;
    g_stSysConfig.iHeartbeatInterval = 30000;  /* 30 seconds */
}

/***********************************************************
*                    Public Functions                      *
**********************************************************/

E_StateCode SysConfigLoad(void)
{
    cJSON *ptJson = NULL;

    /* 初始化默认配置 */
    SysConfigInitDefaults();

    /* 检查配置文件是否存在 */
    if (!JsonFile_Exists(g_stSysConfig.strConfigPath))
    {
        dbprintf("[SysConfig] Config file %s not found, using defaults.\n",
                 g_stSysConfig.strConfigPath);
        g_bConfigLoaded = SMP_TRUE;
        return STATE_CODE_NO_ERROR;
    }

    /* 加载 JSON 文件 */
    ptJson = JsonFile_LoadJson(g_stSysConfig.strConfigPath);
    if (NULL == ptJson)
    {
        SysErr("[SysConfig] JsonFile_LoadJson failed, using defaults.\n");
        return STATE_CODE_NO_ERROR;  /* 使用默认配置 */
    }

    /* 解析配置字段 */
    SysConfigParseJson(ptJson);
    cJSON_Delete(ptJson);

    g_bConfigLoaded = SMP_TRUE;
    dbprintf("[SysConfig] Loaded config from %s\n", g_stSysConfig.strConfigPath);

    return STATE_CODE_NO_ERROR;
}

E_StateCode SysConfigSave(void)
{
    cJSON *ptJson = NULL;
    E_StateCode eCode = STATE_CODE_NO_ERROR;

    if (!g_bConfigLoaded)
    {
        SysErr("[SysConfig] Config not loaded yet.\n");
        return STATE_CODE_OBJECT_NOT_EXIST;
    }

    /* 构建 JSON 对象 */
    ptJson = SysConfigBuildJson();
    if (NULL == ptJson)
    {
        SysErr("[SysConfig] SysConfigBuildJson failed.\n");
        return STATE_CODE_ALLOCATION_FAILURE;
    }

    /* 保存到文件 */
    eCode = JsonFile_SaveJson(g_stSysConfig.strConfigPath, ptJson);
    cJSON_Delete(ptJson);

    if (STATE_OK(eCode))
    {
        dbprintf("[SysConfig] Config saved to %s\n", g_stSysConfig.strConfigPath);
    }
    else
    {
        SysErr("[SysConfig] JsonFile_SaveJson failed: %d\n", eCode);
    }

    return eCode;
}

/**
 * @brief 获取配置项（便捷接口）
 */
E_StateCode SysConfigGet(const INT8 *strSection, const INT8 *strKey, void *pValue, INT32 iValueSize)
{
    cJSON *ptJson = NULL;
    const cJSON *ptValue = NULL;
    E_StateCode eCode = STATE_CODE_NO_ERROR;

    if (NULL == strSection || NULL == strKey || NULL == pValue)
    {
        return STATE_CODE_INVALID_HANDLE;
    }

    if (!g_bConfigLoaded)
    {
        return STATE_CODE_OBJECT_NOT_EXIST;
    }

    /* 加载 JSON 文件 */
    ptJson = JsonFile_LoadJson(g_stSysConfig.strConfigPath);
    if (NULL == ptJson)
    {
        return STATE_CODE_INVALID_HANDLE;
    }

    /* 获取字段 */
    ptValue = cJSON_GetObjectItem(ptJson, strKey);
    if (NULL == ptValue)
    {
        eCode = STATE_CODE_OBJECT_NOT_EXIST;
        goto cleanup;
    }

    /* 根据类型复制值 */
    if (cJSON_IsString(ptValue) && iValueSize > 0)
    {
        strncpy((INT8*)pValue, ptValue->valuestring, iValueSize - 1);
    }
    else if (cJSON_IsNumber(ptValue) && iValueSize == sizeof(INT32))
    {
        *(INT32*)pValue = (INT32)ptValue->valueint;
    }

cleanup:
    cJSON_Delete(ptJson);
    return eCode;
}

/**
 * @brief 设置配置项（便捷接口）
 */
E_StateCode SysConfigSet(const INT8 *strSection, const INT8 *strKey, const void *pValue, INT32 iValueType)
{
    cJSON *ptJson = NULL;
    cJSON *ptValue = NULL;
    E_StateCode eCode = STATE_CODE_NO_ERROR;

    if (NULL == strSection || NULL == strKey || NULL == pValue)
    {
        return STATE_CODE_INVALID_HANDLE;
    }

    /* 加载现有文件（如果存在） */
    if (JsonFile_Exists(g_stSysConfig.strConfigPath))
    {
        ptJson = JsonFile_LoadJson(g_stSysConfig.strConfigPath);
    }

    if (NULL == ptJson)
    {
        ptJson = cJSON_CreateObject();
        if (NULL == ptJson)
        {
            return STATE_CODE_ALLOCATION_FAILURE;
        }
    }

    /* 根据类型创建 cJSON 对象 */
    if (iValueType == 0)  /* 字符串类型 */
    {
        ptValue = cJSON_CreateString((const INT8*)pValue);
    }
    else  /* 数值类型 */
    {
        ptValue = cJSON_CreateNumber(*(const INT32*)pValue);
    }

    if (NULL == ptValue)
    {
        cJSON_Delete(ptJson);
        return STATE_CODE_ALLOCATION_FAILURE;
    }

    /* 替换或添加字段 */
    cJSON *ptOld = cJSON_DetachItemFromObject(ptJson, strKey);
    if (ptOld) cJSON_Delete(ptOld);
    cJSON_AddItemToObject(ptJson, strKey, ptValue);

    /* 保存回文件 */
    eCode = JsonFile_SaveJson(g_stSysConfig.strConfigPath, ptJson);
    cJSON_Delete(ptJson);

    return eCode;
}
