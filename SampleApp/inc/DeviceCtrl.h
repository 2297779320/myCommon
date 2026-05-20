#ifndef _H_DEVICE_CTRL_
#define _H_DEVICE_CTRL_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "framework_def.h"

bool DeviceCtrlInit(ModuleHandle module, void* config);
void DeviceCtrlRun(ModuleHandle module);
void DeviceCtrlDestroy(ModuleHandle module);

#ifdef __cplusplus
}
#endif

#endif
