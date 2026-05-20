#ifndef _H_USER_AGENT_PRIV_
#define _H_USER_AGENT_PRIV_

#ifdef __cplusplus
extern "C" {
#endif

#include "global.h"

typedef struct tag_UserAgent
{
    HANDLE  hUser;  /* Independent STBP client - identity isolation */
} T_UserAgent;

#ifdef __cplusplus
}
#endif

#endif
