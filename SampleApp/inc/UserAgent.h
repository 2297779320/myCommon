#ifndef _H_USER_AGENT_
#define _H_USER_AGENT_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "framework_v2.h"

bool UserAgentInit(ModuleHandleV2 module, void* config);
void UserAgentRun(ModuleHandleV2 module);
void UserAgentDestroy(ModuleHandleV2 module);

int UserAgentCmpMsgId(const char *strFmt, const char *strTopic);

const T_MsgProcEntryV2* GetUserAgentMsgTable(void);
uint32_t GetUserAgentMsgTableLen(void);

#ifdef __cplusplus
}
#endif

#endif
