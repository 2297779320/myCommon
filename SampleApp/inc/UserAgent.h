#ifndef _H_USER_AGENT_
#define _H_USER_AGENT_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "framework_def.h"

bool UserAgentInit(ModuleHandle module, void* config);
void UserAgentRun(ModuleHandle module);
void UserAgentDestroy(ModuleHandle module);

int UserAgentCmpMsgId(const char *strFmt, const char *strTopic);

#ifdef __cplusplus
}
#endif

#endif
