/**************************************************************************
*  Copyright (c) 2024
*  File Name: MainApp.c
*  Description: SampleApp entry point - 使用 Framework V2
**************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "typedef.h"
#include "osal.h"
#include "global.h"
#include "SampleApp.h"
#include "SysConfig.h"
#include "framework_v2.h"

/* Module headers */
#include "UserAgent.h"
#include "SysManager.h"
#include "SensorHub.h"
#include "DeviceCtrl.h"

/***********************************************************
*                    Internal Definitions                  *
**********************************************************/
#define MAIN_ROLL_INTERVAL_MS       100
#define CONFIG_SAVE_CYCLE_MS        5000
#define MAX_MESSAGE_QUEUE_SIZE      1000

/***********************************************************
*                    Global Variables                      *
**********************************************************/
static BOOL g_bDone = FALSE;
static FrameworkHandleV2 g_hFrameworkV2 = NULL;

/***********************************************************
*                    Signal Handler                        *
**********************************************************/
static void SignalHandler(INT32 iSignal)
{
    dbprintf("Received signal %d, shutting down...\n", iSignal);
    g_bDone = TRUE;
    if (g_hFrameworkV2)
    {
        framework_v2_stop_main_loop(g_hFrameworkV2);
    }
}

/***********************************************************
*                    Main Entry                            *
**********************************************************/
INT32 main(INT32 iArgc, INT8 **ppcArgv)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;

    printf("========================================\n");
    printf("  %s v%s (Framework V2) starting...\n", SAMPLE_APP_NAME, SAMPLE_APP_VERSION);
    printf("========================================\n");

    /* Register signal handlers */
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    /* Create V2 framework with message queue */
    g_hFrameworkV2 = framework_v2_create(MAX_MESSAGE_QUEUE_SIZE);
    if (NULL == g_hFrameworkV2)
    {
        SysErr("framework_v2_create failed!\n");
        return -1;
    }

    /* Global init: create STBP client, load config */
    eCode = GlobalInit();
    if (!STATE_OK(eCode))
    {
        SysErr("GlobalInit failed, eCode = %d\n", eCode);
        framework_v2_destroy(g_hFrameworkV2);
        return -1;
    }

    /* Register V2 modules with topic-based message tables */
    /* UserAgent module */
    if (framework_v2_register_module(g_hFrameworkV2, 
            UserAgentInit, UserAgentRun, UserAgentDestroy,
            GetUserAgentMsgTable(), GetUserAgentMsgTableLen(), NULL) == 0)
    {
        SysErr("UserAgent module registration failed\n");
        goto cleanup;
    }

    /* SysManager module */
    if (framework_v2_register_module(g_hFrameworkV2,
            SysManagerInit, SysManagerRun, SysManagerDestroy,
            GetSysManagerMsgTable(), GetSysManagerMsgTableLen(), NULL) == 0)
    {
        SysErr("SysManager module registration failed\n");
        goto cleanup;
    }

    /* SensorHub module */
    if (framework_v2_register_module(g_hFrameworkV2,
            SensorHubInit, SensorHubRun, SensorHubDestroy,
            GetSensorHubMsgTable(), GetSensorHubMsgTableLen(), NULL) == 0)
    {
        SysErr("SensorHub module registration failed\n");
        goto cleanup;
    }

    /* DeviceCtrl module */
    if (framework_v2_register_module(g_hFrameworkV2,
            DeviceCtrlInit, DeviceCtrlRun, DeviceCtrlDestroy,
            GetDeviceCtrlMsgTable(), GetDeviceCtrlMsgTableLen(), NULL) == 0)
    {
        SysErr("DeviceCtrl module registration failed\n");
        goto cleanup;
    }

    dbprintf("All %u modules registered (V2).\n", 
             framework_v2_get_module_count(g_hFrameworkV2));

    /* Start V2 main loop (this blocks until framework_v2_stop_main_loop is called) */
    dbprintf("Starting V2 main loop with topic-based routing...\n");
    framework_v2_start_main_loop(g_hFrameworkV2, MAIN_ROLL_INTERVAL_MS);

    dbprintf("Main loop exited. Cleaning up...\n");

cleanup:
    GlobalDestroy();
    framework_v2_destroy(g_hFrameworkV2);

    printf("SampleApp exited.\n");
    return 0;
}
