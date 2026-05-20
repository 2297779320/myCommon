#ifndef _H_SYS_MANAGER_
#define _H_SYS_MANAGER_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "framework_def.h"

bool SysManagerInit(ModuleHandle module, void* config);
void SysManagerRun(ModuleHandle module);
void SysManagerDestroy(ModuleHandle module);

#ifdef __cplusplus
}
#endif

#endif
