#ifndef _H_SYS_CONFIG_
#define _H_SYS_CONFIG_

#ifdef __cplusplus
extern "C" {
#endif

#include "typedef.h"

E_StateCode SysConfigLoad(void);
E_StateCode SysConfigSave(void);

#ifdef __cplusplus
}
#endif

#endif
