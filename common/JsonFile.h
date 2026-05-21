/**
 * @file JsonFile.h
 * @brief JSON 配置文件读写工具 -- 仅负责文件 I/O
 *
 * @details
 * 仅提供文件级别的原始读写操作：
 *   - 读取文件内容到字符串缓冲区
 *   - 将字符串写入文件
 *   - 文件存在性检查与删除
 *
 * JSON 解析/格式化交由 JsonParse 模块处理。
 *
 * @see JsonParse.h（JSON 解析工具）
 */

#ifndef JSON_FILE_H
#define JSON_FILE_H

#include "defs.h"
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************
 *                         常量定义                                       *
 **************************************************************************/

/** @brief JSON 文件最大大小（1MB） */
#define JSON_FILE_MAX_SIZE          (1024 * 1024)

/**************************************************************************
 *                         文件 I/O（原始字符串）                          *
 **************************************************************************/

/**
 * @brief 读取整个文件内容到字符串
 * @param strFilePath   文件路径
 * @param ppBuf         输出：字符串指针（需调用者 free）
 * @return E_StateCode  状态码
 */
E_StateCode JsonFile_Read(const INT8 *strFilePath, INT8 **ppBuf);

/**
 * @brief 将字符串写入文件
 * @param strFilePath   文件路径
 * @param strBuf        字符串内容
 * @return E_StateCode  状态码
 */
E_StateCode JsonFile_Write(const INT8 *strFilePath, const INT8 *strBuf);

/**
 * @brief 检查文件是否存在
 */
BOOL JsonFile_Exists(const INT8 *strFilePath);

/**
 * @brief 删除文件
 */
E_StateCode JsonFile_Delete(const INT8 *strFilePath);

/**************************************************************************
 *                         JSON 加载/保存（解析 + I/O）                    *
 **************************************************************************/

/**
 * @brief 从文件加载并解析 JSON
 * @details 内部调用 JsonFile_Read + cJSON_Parse
 * @param strFilePath   文件路径
 * @return cJSON*       解析后的 cJSON 对象，失败返回 NULL（需调用者 cJSON_Delete）
 */
cJSON* JsonFile_LoadJson(const INT8 *strFilePath);

/**
 * @brief 将 cJSON 对象格式化并保存到文件
 * @details 内部调用 cJSON_Print + JsonFile_Write
 * @param strFilePath   文件路径
 * @param ptJson        cJSON 对象
 * @return E_StateCode  状态码
 */
E_StateCode JsonFile_SaveJson(const INT8 *strFilePath, const cJSON *ptJson);

/**
 * @brief 将 cJSON 对象紧凑格式化并保存到文件
 * @details 内部调用 cJSON_PrintUnformatted + JsonFile_Write
 * @param strFilePath   文件路径
 * @param ptJson        cJSON 对象
 * @return E_StateCode  状态码
 */
E_StateCode JsonFile_SaveJsonCompact(const INT8 *strFilePath, const cJSON *ptJson);

#ifdef __cplusplus
}
#endif

#endif /* JSON_FILE_H */
