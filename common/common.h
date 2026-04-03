#ifndef _COMMON_H_
#define _COMMON_H_

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

INT32 get_vcodec_type(INT8* vcodec_str);

INT32 get_acodec_type(INT8* acodec_str);

int GetSizeFromString(const char *resolutionStr, uint32_t *width, uint32_t *height);

EXTERN_C_BLOCK_END
#endif


