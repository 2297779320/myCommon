/**************************************************************************
*  Copyright (c) 2024
*  File Name: MainApp.c
*  Description: SampleApp entry point - module declaration and startup
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
#include "framework_def.h"

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
static FrameworkHandle g_hFramework = NULL;

/***********************************************************
*                    Signal Handler                        *
**********************************************************/
static void SignalHandler(INT32 iSignal)
{
    dbprintf("Received signal %d, shutting down...\n", iSignal);
    g_bDone = TRUE;
    if (g_hFramework)
    {
        framework_stop_main_loop(g_hFramework);
    }
}

/***********************************************************
*                    Main Entry                            *
**********************************************************/
INT32 main(INT32 iArgc, INT8 **ppcArgv)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;

    printf("========================================\n");
    printf("  %s v%s starting...\n", SAMPLE_APP_NAME, SAMPLE_APP_VERSION);
    printf("========================================\n");

    /* Register signal handlers */
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    /* Create framework with message queue */
    g_hFramework = framework_create(MAX_MESSAGE_QUEUE_SIZE);
    if (NULL == g_hFramework)
    {
        SysErr("framework_create failed!\n");
        return -1;
    }

    /* Global init: create STBP client, load config */
    eCode = GlobalInit();
    if (!STATE_OK(eCode))
    {
        SysErr("GlobalInit failed, eCode = %d\n", eCode);
        framework_destroy(g_hFramework);
        return -1;
    }

    /* Register modules with framework */
    /* UserAgent module */
    if (!framework_register_module(g_hFramework, UserAgentInit, UserAgentRun, UserAgentDestroy, NULL))
    {
        SysErr("UserAgent module registration failed\n");
        goto cleanup;
    }

    /* SysManager module */
    if (!framework_register_module(g_hFramework, SysManagerInit, SysManagerRun, SysManagerDestroy, NULL))
    {
        SysErr("SysManager module registration failed\n");
        goto cleanup;
    }

    /* SensorHub module */
    if (!framework_register_module(g_hFramework, SensorHubInit, SensorHubRun, SensorHubDestroy, NULL))
    {
        SysErr("SensorHub module registration failed\n");
        goto cleanup;
    }

    /* DeviceCtrl module */
    if (!framework_register_module(g_hFramework, DeviceCtrlInit, DeviceCtrlRun, DeviceCtrlDestroy, NULL))
    {
        SysErr("DeviceCtrl module registration failed\n");
        goto cleanup;
    }

    dbprintf("All %d modules registered.\n", framework_get_module_count(g_hFramework));

    /* Start main loop (this blocks until framework_stop_main_loop is called) */
    dbprintf("Starting main loop...\n");
    framework_start_main_loop(g_hFramework, MAIN_ROLL_INTERVAL_MS);

    dbprintf("Main loop exited. Cleaning up...\n");

cleanup:
    GlobalDestroy();
    framework_destroy(g_hFramework);

    printf("SampleApp exited.\n");
    return 0;
}
