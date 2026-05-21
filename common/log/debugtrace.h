/**
 * @file debugtrace.h
 * @brief 调试跟踪模块接口
 *
 * @details
 * 提供调试跟踪服务的初始化和销毁功能。
 * 调试跟踪服务用于远程调试命令处理、心跳检测等。
 *
 * @see defs.h（依赖 E_StateCode）
 */

#ifndef DEBUGTRACE_H
#define DEBUGTRACE_H


#include <stdio.h>
#include "defs.h"


// extern BOOL	g_bSysStatistics;
// extern BOOL	g_bFullDebug;

/**********************************************************************
* 函数名称：debug_init
* 功能描述：创建debugtrace句柄
* 输入参数：
* 输出参数：无
* 返 回 值：      状态码
* 其它说明：
* 修改日期     版本号   修改人           修改内容
* -----------------------------------------------
* 2025/11/10       V1.0              chengjiahao
***********************************************************************/
E_StateCode debug_init(void *pMOCtx, void *pMICtx);

/**********************************************************************
* 函数名称：debug_destroy
* 功能描述：销毁debugtrace句柄
* 输入参数：
* 输出参数：无
* 返 回 值：      状态码
* 其它说明：
* 修改日期     版本号   修改人           修改内容
* -----------------------------------------------
* 2025/11/10       V1.0              chengjiahao
***********************************************************************/
E_StateCode debug_destroy();

#endif // DEBUGTRACE_H