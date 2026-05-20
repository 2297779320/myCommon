#ifndef _H_APP_MODULE_V2_
#define _H_APP_MODULE_V2_

#ifdef __cplusplus
extern "C" {
#endif

#include "typedef.h"

/* Forward declarations */
typedef struct T_MsgProcV2 T_MsgProcV2;
typedef struct T_ModuleV2 T_ModuleV2;

/* Module state codes */
typedef enum {
    STATE_CODE_NO_ERROR = 0,
    STATE_CODE_INVALID_HANDLE = -1,
    STATE_CODE_ALLOCATION_FAILURE = -2,
    STATE_CODE_TIME_OUT = -3,
    STATE_CODE_OBJECT_NOT_EXIST = -4,
    STATE_CODE_OBJECT_EXISTED = -5,
    STATE_CODE_INIT_FAILURE = -6,
    STATE_CODE_UNDEFINED_ERROR = -7,
} E_StateCode;

#define STATE_OK(code)  ((code) == STATE_CODE_NO_ERROR)

#define SMP_TRUE    1
#define SMP_FALSE   0
#define BOOL        int
#define TRUE        1
#define FALSE       0

/* Module declaration macros */
#define DECLARE_MODULE_V2(modId, pfInit, pfDelete, pfProc, Priority, StackSize, bFifo, uiMsgPkts) \
    static T_ModuleV2 g_tModule_##modId = { \
        .id = modId, \
        .pfInit = pfInit, \
        .pfDelete = pfDelete, \
        .pfProc = pfProc, \
        .tAttrs = { .priority = Priority, .stackSize = StackSize, .bFifo = bFifo }, \
        .uiMsgPkts = uiMsgPkts \
    }

#define MODULEV2(modId)   (&g_tModule_##modId)
#define MODULE_INIT_V2(pMod, pParam)   ((pMod)->pfInit(pParam))
#define MODULE_START_V2(pMod, pParam)  ((pMod)->pfProc(pParam))
#define MODULE_DELETE_V2(pMod, pParam) ((pMod)->pfDelete(pParam))
#define MODULE_DONE_V2(pMod)           ((pMod)->bDone)

/* Default values */
#define DEFAULT_PRIORITY_V2     50
#define DEFAULT_STACK_SIZE_V2   4096

/* Module struct */
struct T_ModuleV2 {
    INT32 id;
    E_StateCode (*pfInit)(void *);
    E_StateCode (*pfDelete)(void *);
    E_StateCode (*pfProc)(void *);
    void *pPrivate;
    T_MsgProcV2 *ptMsgProcTable;
    struct {
        INT32 priority;
        INT32 stackSize;
        INT32 bFifo;
        INT8 name[64];
    } tAttrs;
    UINT32 uiMsgPkts;
    BOOL bDone;
    void *hMsgQue;
};

/* Message struct */
typedef struct {
    INT8 strMsgId[256];
    INT8 strReply[256];
    INT8 *pcBody;
    UINT32 uiBodySize;
    UINT64 u64PrivateData;
    INT8 strDescription[128];
} T_MsgV2;

/* Message handler */
typedef E_StateCode (*MsgProcFuncV2)(void *pPrivate, T_MsgV2 *ptMsg,
    INT8 *pcResMsg, INT8 **ppcResData, UINT32 *puiDataSize, BOOL *pbDelayRes);

struct T_MsgProcV2 {
    INT8 *strMsgId;
    MsgProcFuncV2 pfProc;
};

/* Common message processing */
E_StateCode CommonMsgProcessV2(T_ModuleV2 *ptModule, BOOL bBlock);
E_StateCode CommonMsgResponseV2(T_ModuleV2 *ptModule, T_MsgV2 *ptMsg, UINT32 uiTimeout);

/* Topic comparison */
BOOL IsTopicEqual(INT8 *strFmt, INT8 *strTopic);

/* String types */
typedef char INT8[256];
typedef char String64[64];
typedef char String128[128];
typedef char String256[256];

#ifdef __cplusplus
}
#endif

#endif
