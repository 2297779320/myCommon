#ifndef _H_SENSOR_HUB_
#define _H_SENSOR_HUB_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "framework_def.h"

bool SensorHubInit(ModuleHandle module, void* config);
void SensorHubRun(ModuleHandle module);
void SensorHubDestroy(ModuleHandle module);

#ifdef __cplusplus
}
#endif

#endif
