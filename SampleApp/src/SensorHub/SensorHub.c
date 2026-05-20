/**************************************************************************
*  Copyright (c) 2024
*  File Name: SensorHub.c
*  Description: 传感器模块 - 使用 V2 框架
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
#include "framework_v2.h"

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
    ModuleHandleV2  hModule;
} T_SensorHub;

static T_SensorHub g_stSensorHub = {0};

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
*                    V2 Message Handlers                   *
**********************************************************/
static E_StateCode SensorHubReadHandler(
    void *pPrivate, T_FrameworkMsgV2 *ptMsg,
    char *pcResMsg, char **ppcResData, uint32_t *puiDataSize, bool *pbDelayRes)
{
    (void)pPrivate;
    (void)pcResMsg;
    (void)pbDelayRes;

    dbprintf("[SensorHub] Read request received\n");
    
    /* 返回传感器数据 */
    static char response[256];
    snprintf(response, sizeof(response),
             "{\"temperature\":%.1f,\"humidity\":%.1f}",
             g_stSensorHub.atSensor[0].fTemperature,
             g_stSensorHub.atSensor[0].fHumidity);
    *ppcResData = response;
    *puiDataSize = strlen(response);
    
    return STATE_CODE_NO_ERROR;
}

static T_MsgProcEntryV2 g_satSensorHubTable[] =
{
    {"$request.get.*.*.*.sample.v1.sensor.read", SensorHubReadHandler, NULL, true, true},
    {NULL, NULL, NULL, false, false}
};

const T_MsgProcEntryV2* GetSensorHubMsgTable(void)
{
    return g_satSensorHubTable;
}

uint32_t GetSensorHubMsgTableLen(void)
{
    return 1;
}

/***********************************************************
*                    Module Functions                      *
**********************************************************/
bool SensorHubInit(ModuleHandleV2 module, void* config)
{
    TSK_Attrs tAttr = DEFAULT_TSK_ATTR;

    (void)config;

    memset(&g_stSensorHub, 0x0, sizeof(g_stSensorHub));
    g_stSensorHub.uiSensorCnt = 2; /* Simulate 2 sensors */
    g_stSensorHub.hModule = module;

    /* Create report thread */
    tAttr.name = "SensorReport";
    g_stSensorHub.hReportTsk = TSK_create(SensorReportFxn, &tAttr, &g_stSensorHub);
    if (NULL == g_stSensorHub.hReportTsk)
    {
        SysErr("Create report thread failed!\n");
        return false;
    }

    dbprintf("[SensorHub] Initialized with %d sensors (V2).\n", g_stSensorHub.uiSensorCnt);
    return true;
}

void SensorHubRun(ModuleHandleV2 module)
{
    /* 消息处理由框架自动分发 */
}

void SensorHubDestroy(ModuleHandleV2 module)
{
    g_stSensorHub.bDone = TRUE;
    if (NULL != g_stSensorHub.hReportTsk)
    {
        TSK_delete(g_stSensorHub.hReportTsk);
    }
    dbprintf("[SensorHub] Deleted (V2).\n");
}
