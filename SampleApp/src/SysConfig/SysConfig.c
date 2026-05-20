/**************************************************************************
*  Copyright (c) 2024
*  File Name: SysConfig.c
*  Description: System configuration loading/saving
**************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "typedef.h"
#include "osal.h"
#include "global.h"
#include "SampleApp.h"
#include "SysConfig.h"

static BOOL g_bConfigLoaded = SMP_FALSE;

E_StateCode SysConfigLoad(void)
{
    INT8 strCfgPath[256];
    FILE *fp = NULL;

    /* Try to load system config */
    snprintf(strCfgPath, sizeof(strCfgPath), "configs/topic_data/sample.v1/sys.json");

    fp = fopen(strCfgPath, "r");
    if (NULL != fp)
    {
        INT8 strBuf[1024];
        size_t nRead = fread(strBuf, 1, sizeof(strBuf) - 1, fp);
        if (nRead > 0)
        {
            strBuf[nRead] = '\0';
            dbprintf("[SysConfig] Loaded config from %s:\n%s\n", strCfgPath, strBuf);
        }
        fclose(fp);
        g_bConfigLoaded = SMP_TRUE;
    }
    else
    {
        dbprintf("[SysConfig] Config file %s not found, using defaults.\n", strCfgPath);
    }

    return STATE_CODE_NO_ERROR;
}

E_StateCode SysConfigSave(void)
{
    /* Placeholder for config save */
    return STATE_CODE_NO_ERROR;
}
