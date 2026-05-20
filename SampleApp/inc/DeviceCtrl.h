#ifndef _H_DEVICE_CTRL_
#define _H_DEVICE_CTRL_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "framework_v2.h"

bool DeviceCtrlInit(ModuleHandleV2 module, void* config);
void DeviceCtrlRun(ModuleHandleV2 module);
void DeviceCtrlDestroy(ModuleHandleV2 module);

/* V2 消息表导出 */
const T_MsgProcEntryV2* GetDeviceCtrlMsgTable(void);
uint32_t GetDeviceCtrlMsgTableLen(void);

#ifdef __cplusplus
}
#endif

#endif
