#ifndef _H_SENSOR_HUB_
#define _H_SENSOR_HUB_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "framework_v2.h"

bool SensorHubInit(ModuleHandleV2 module, void* config);
void SensorHubRun(ModuleHandleV2 module);
void SensorHubDestroy(ModuleHandleV2 module);

const T_MsgProcEntryV2* GetSensorHubMsgTable(void);
uint32_t GetSensorHubMsgTableLen(void);

#ifdef __cplusplus
}
#endif

#endif
