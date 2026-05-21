/**
 * @file JsonFile.c
 * @brief JSON 配置文件读写实现 -- 仅负责文件 I/O
 *
 * @details
 * 实现文件级别的原始读写：
 *   - 读取整个文件到字符串缓冲区
 *   - 将字符串写入文件
 *   - 文件存在性检查与删除
 */

#include "JsonFile.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/***********************************************************
*                    内部辅助函数                          *
**********************************************************/

/**
 * @brief 打开文件并检查状态
 */
static BOOL JsonFile_Open(const INT8 *strFilePath, const INT8 *strMode, FILE **ppFp)
{
    if (NULL == strFilePath)
    {
        SysErr("Invalid file path.\n");
        return FALSE;
    }

    *ppFp = fopen(strFilePath, strMode);
    if (NULL == *ppFp)
    {
        SysErr("Failed to open file: %s\n", strFilePath);
        return FALSE;
    }

    return TRUE;
}

/***********************************************************
*                    文件读写                              *
**********************************************************/

E_StateCode JsonFile_Read(const INT8 *strFilePath, INT8 **ppBuf)
{
    FILE *fp = NULL;
    INT8 *pBuf = NULL;
    size_t file_size = 0;

    if (NULL == strFilePath || NULL == ppBuf)
    {
        return STATE_CODE_INVALID_HANDLE;
    }

    if (!JsonFile_Open(strFilePath, "r", &fp))
    {
        return STATE_CODE_UNABLE_TO_OPEN_FILE;
    }

    /* 获取文件大小 */
    fseek(fp, 0, SEEK_END);
    file_size = (size_t)ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size > JSON_FILE_MAX_SIZE)
    {
        SysErr("File too large: %zu bytes (max: %d)\n", file_size, JSON_FILE_MAX_SIZE);
        fclose(fp);
        return STATE_CODE_INVALID_PARAM;
    }

    /* 分配缓冲区 */
    pBuf = (INT8 *)malloc(file_size + 1);
    if (NULL == pBuf)
    {
        SysErr("malloc failed for %zu bytes.\n", file_size);
        fclose(fp);
        return STATE_CODE_ALLOCATION_FAILURE;
    }

    /* 读取文件 */
    size_t nRead = fread(pBuf, 1, file_size, fp);
    pBuf[nRead] = '\0';

    fclose(fp);

    *ppBuf = pBuf;
    return STATE_CODE_NO_ERROR;
}

E_StateCode JsonFile_Write(const INT8 *strFilePath, const INT8 *strBuf)
{
    FILE *fp = NULL;

    if (NULL == strFilePath || NULL == strBuf)
    {
        return STATE_CODE_INVALID_HANDLE;
    }

    if (!JsonFile_Open(strFilePath, "w", &fp))
    {
        return STATE_CODE_UNABLE_TO_OPEN_FILE;
    }

    size_t len = strlen(strBuf);
    size_t nWritten = fwrite(strBuf, 1, len, fp);

    fclose(fp);

    if (nWritten != len)
    {
        SysErr("fwrite failed: expected %zu, got %zu\n", len, nWritten);
        return STATE_CODE_ALLOCATION_FAILURE;
    }

    return STATE_CODE_NO_ERROR;
}

/***********************************************************
*                    文件操作                              *
**********************************************************/

BOOL JsonFile_Exists(const INT8 *strFilePath)
{
    if (NULL == strFilePath)
    {
        return FALSE;
    }

    struct stat st;
    return (stat(strFilePath, &st) == 0) ? TRUE : FALSE;
}

E_StateCode JsonFile_Delete(const INT8 *strFilePath)
{
    if (NULL == strFilePath)
    {
        return STATE_CODE_INVALID_HANDLE;
    }

    if (remove(strFilePath) != 0)
    {
        SysErr("Failed to delete file: %s\n", strFilePath);
        return STATE_CODE_UNABLE_TO_OPEN_FILE;
    }

    return STATE_CODE_NO_ERROR;
}

/***********************************************************
*                    JSON 加载/保存                        *
**********************************************************/

cJSON* JsonFile_LoadJson(const INT8 *strFilePath)
{
    INT8 *strBuf = NULL;
    cJSON *ptJson = NULL;

    if (NULL == strFilePath)
    {
        return NULL;
    }

    /* 读取文件 */
    E_StateCode eCode = JsonFile_Read(strFilePath, &strBuf);
    if (!STATE_OK(eCode))
    {
        return NULL;
    }

    /* 解析 JSON */
    ptJson = cJSON_Parse(strBuf);
    if (NULL == ptJson)
    {
        SysErr("cJSON_Parse failed for file: %s\n", strFilePath);
    }

    free(strBuf);
    return ptJson;
}

E_StateCode JsonFile_SaveJson(const INT8 *strFilePath, const cJSON *ptJson)
{
    INT8 *strOutput = NULL;
    E_StateCode eCode = STATE_CODE_NO_ERROR;

    if (NULL == strFilePath || NULL == ptJson)
    {
        return STATE_CODE_INVALID_HANDLE;
    }

    /* 格式化 */
    strOutput = cJSON_Print(ptJson);
    if (NULL == strOutput)
    {
        SysErr("cJSON_Print failed.\n");
        return STATE_CODE_ALLOCATION_FAILURE;
    }

    /* 写入文件 */
    eCode = JsonFile_Write(strFilePath, strOutput);
    cJSON_free(strOutput);

    return eCode;
}

E_StateCode JsonFile_SaveJsonCompact(const INT8 *strFilePath, const cJSON *ptJson)
{
    INT8 *strOutput = NULL;
    E_StateCode eCode = STATE_CODE_NO_ERROR;

    if (NULL == strFilePath || NULL == ptJson)
    {
        return STATE_CODE_INVALID_HANDLE;
    }

    /* 紧凑格式化 */
    strOutput = cJSON_PrintUnformatted(ptJson);
    if (NULL == strOutput)
    {
        SysErr("cJSON_PrintUnformatted failed.\n");
        return STATE_CODE_ALLOCATION_FAILURE;
    }

    /* 写入文件 */
    eCode = JsonFile_Write(strFilePath, strOutput);
    cJSON_free(strOutput);

    return eCode;
}
