#ifndef _H_SYS_MANAGER_
#define _H_SYS_MANAGER_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "framework_v2.h"

bool SysManagerInit(ModuleHandleV2 module, void* config);
void SysManagerRun(ModuleHandleV2 module);
void SysManagerDestroy(ModuleHandleV2 module);

const T_MsgProcEntryV2* GetSysManagerMsgTable(void);
uint32_t GetSysManagerMsgTableLen(void);

#ifdef __cplusplus
}
#endif

#endif
