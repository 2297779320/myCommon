/**************************************************************************
*  Copyright (c) 2024
*  File Name: SensorHub.c
*  Description: Sensor module - simulates temperature/humidity data reporting
**************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include "typedef.h"
#include "osal.h"
#include "tsk.h"
#include "global.h"
#include "SampleApp.h"
#include "StbpClient.h"
#include "SensorHub.h"
#include "framework_def.h"

/***********************************************************
*                    Private Types                         *
**********************************************************/
typedef struct {
    FLOAT   fTemperature;
    FLOAT   fHumidity;
    UINT32  uiReportCnt;
} T_SensorData;

typedef struct {
    BOOL            bDone;
    TSK_Handle      hReportTsk;
    T_SensorData    atSensor[MAX_SENSOR_CNT];
    UINT32          uiSensorCnt;
    ModuleHandle    hModule;
} T_SensorHub;

/***********************************************************
*                    Internal Functions                    *
**********************************************************/
static void SensorReportFxn(void *param)
{
    T_SensorHub *ptObj = (T_SensorHub *)param;
    HANDLE hUser = NULL;
    INT8 strPayload[128];
    INT32 i;

    if (NULL == ptObj) return;

    dbprintf("[SensorHub] Report thread started.\n");

    /* Initialize simulated sensor data */
    for (i = 0; i < MAX_SENSOR_CNT; i++)
    {
        ptObj->atSensor[i].fTemperature = 22.0f + (FLOAT)(i * 2);
        ptObj->atSensor[i].fHumidity = 50.0f + (FLOAT)(i * 5);
        ptObj->atSensor[i].uiReportCnt = 0;
    }

    while (!ptObj->bDone)
    {
        hUser = GetUserClientHandle();
        if (NULL != hUser && StbpClientConnected(hUser))
        {
            for (i = 0; i < ptObj->uiSensorCnt; i++)
            {
                /* Simulate slight data changes */
                ptObj->atSensor[i].fTemperature += 0.1f * (((FLOAT)rand() / RAND_MAX) - 0.5f);
                ptObj->atSensor[i].fHumidity += 0.5f * (((FLOAT)rand() / RAND_MAX) - 0.5f);
                ptObj->atSensor[i].uiReportCnt++;

                /* Publish temperature */
                snprintf(strPayload, sizeof(strPayload),
                    "{\"sensorId\":%d,\"temperature\":%.1f,\"unit\":\"C\"}",
                    i, ptObj->atSensor[i].fTemperature);
                StbpClientPublish(hUser, TOPIC_SENSOR_TEMP, strPayload);

                /* Publish humidity */
                snprintf(strPayload, sizeof(strPayload),
                    "{\"sensorId\":%d,\"humidity\":%.1f,\"unit\":\"%%\"}",
                    i, ptObj->atSensor[i].fHumidity);
                StbpClientPublish(hUser, TOPIC_SENSOR_HUMI, strPayload);

                dbprintf("[SensorHub] Sensor %d: %.1f C, %.1f%% (report #%u)\n",
                    i, ptObj->atSensor[i].fTemperature,
                    ptObj->atSensor[i].fHumidity,
                    ptObj->atSensor[i].uiReportCnt);
            }
        }

        OSAL_Sleep(SENSOR_REPORT_MS);
    }

    dbprintf("[SensorHub] Report thread stopped.\n");
}

/***********************************************************
*                    Message Handlers                      *
**********************************************************/
static void SensorHubMessageHandler(ModuleHandle module, const T_ModuleMsg* msg)
{
    if (NULL == module || NULL == msg)
    {
        return;
    }

    dbprintf("[SensorHub] Received message type: %u\n", msg->type);
    /* Process module-specific messages here if needed */
}

/***********************************************************
*                    Module Functions                      *
**********************************************************/
bool SensorHubInit(ModuleHandle module, void* config)
{
    T_SensorHub *ptPrivate = NULL;
    TSK_Attrs tAttr = DEFAULT_TSK_ATTR;

    (void)config;

    ptPrivate = (T_SensorHub *)malloc(sizeof(T_SensorHub));
    if (NULL == ptPrivate)
    {
        SysErr("malloc failed!\n");
        return false;
    }
    memset(ptPrivate, 0x0, sizeof(T_SensorHub));

    ptPrivate->uiSensorCnt = 2; /* Simulate 2 sensors */
    ptPrivate->hModule = module;

    /* Register message handler */
    module_register_handler(module, 0, SensorHubMessageHandler);

    /* Create report thread */
    tAttr.name = "SensorReport";
    ptPrivate->hReportTsk = TSK_create(SensorReportFxn, &tAttr, ptPrivate);
    if (NULL == ptPrivate->hReportTsk)
    {
        SysErr("Create report thread failed!\n");
        free(ptPrivate);
        return false;
    }

    /* Store private data in module */
    module->private_data = ptPrivate;

    dbprintf("[SensorHub] Initialized with %d sensors.\n", ptPrivate->uiSensorCnt);
    return true;
}

void SensorHubRun(ModuleHandle module)
{
    T_SensorHub *ptPrivate = NULL;

    if (NULL == module)
    {
        return;
    }

    ptPrivate = (T_SensorHub *)module->private_data;
    if (NULL == ptPrivate)
    {
        return;
    }

    /* Module run logic here - report thread handles the work */
    /* This function is called periodically by the framework */
}

void SensorHubDestroy(ModuleHandle module)
{
    T_SensorHub *ptPrivate = NULL;

    if (NULL == module)
    {
        return;
    }

    ptPrivate = (T_SensorHub *)module->private_data;
    if (NULL == ptPrivate)
    {
        return;
    }

    ptPrivate->bDone = TRUE;
    if (NULL != ptPrivate->hReportTsk)
    {
        TSK_delete(ptPrivate->hReportTsk);
    }

    free(ptPrivate);
    module->private_data = NULL;

    dbprintf("[SensorHub] Deleted.\n");
}
