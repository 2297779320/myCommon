/**
 * @file common.h
 * @brief 公共工具库总入口 -- 字符串处理、编解码查询、分辨率解析
 *
 * @details
 * 本文件是公共库的伞形头文件，聚合了 defs.h、comm_que.h、ctos.h、osal.h、
 * tsk.h、xlist.h、uthash.h 等核心模块。用户只需 @c \#include "common.h" 即可
 * 获得完整的公共库功能。
 *
 * @see defs.h, comm_que.h, ctos.h, osal.h, tsk.h, xlist.h, uthash.h（依赖）
 */

#ifndef COMMON_H
#define COMMON_H

#include "defs.h"
#include "comm_que.h"
#include "ctos.h"
#include "osal.h"
#include "tsk.h"
#include "xlist.h"
#include "uthash.h"

EXTERN_C_BLOCK

int string_to_argv(const char *cmd_line, char ***argv);

void free_argv(char **argv, int argc);


INT8* get_vcodec_str(UINT32 vcodec_id);

INT8* get_acodec_str(UINT32 acodec_id);

INT32 get_vcodec_type(const INT8* vcodec_str);

INT32 get_acodec_type(const INT8* acodec_str);

int GetSizeFromString(const char *resolutionStr, uint32_t *width, uint32_t *height);

EXTERN_C_BLOCK_END
#endif // COMMON_H


