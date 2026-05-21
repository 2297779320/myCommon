#ifndef _H_SYS_CONFIG_
#define _H_SYS_CONFIG_

#ifdef __cplusplus
extern "C" {
#endif

#include "typedef.h"

E_StateCode SysConfigLoad(void);
E_StateCode SysConfigSave(void);

/**
 * @brief 获取配置项
 * @param strSection   配置段（暂未使用，保留扩展）
 * @param strKey       配置键名
 * @param pValue       输出值缓冲区
 * @param iValueSize   缓冲区大小
 * @return E_StateCode 状态码
 */
E_StateCode SysConfigGet(const INT8 *strSection, const INT8 *strKey, void *pValue, INT32 iValueSize);

/**
 * @brief 设置配置项
 * @param strSection   配置段（暂未使用，保留扩展）
 * @param strKey       配置键名
 * @param pValue       值指针（字符串或 INT32）
 * @param iValueType   值类型：0=字符串, 1=INT32
 * @return E_StateCode 状态码
 */
E_StateCode SysConfigSet(const INT8 *strSection, const INT8 *strKey, const void *pValue, INT32 iValueType);

#ifdef __cplusplus
}
#endif

#endif
