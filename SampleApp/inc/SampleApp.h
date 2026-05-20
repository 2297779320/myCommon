#ifndef _H_SAMPLE_APP_
#define _H_SAMPLE_APP_

#ifdef __cplusplus
extern "C" {
#endif

#include "typedef.h"

/**************************************************************************
 *                         App Version                                    *
 **************************************************************************/
#define SAMPLE_APP_VERSION    "1.0.0"
#define SAMPLE_APP_NAME       "SampleApp"

/**************************************************************************
 *                         Debug Macros                                   *
 **************************************************************************/
#define dbprintf    printf
#define SysErr      printf
#define SysDbg      printf

/**************************************************************************
 *                         Common Constants                               *
 **************************************************************************/
#define MAX_SENSOR_CNT        4
#define MAX_DEVICE_CNT        8
#define HEARTBEAT_INTERVAL_MS 5000
#define SENSOR_REPORT_MS      2000

#ifdef __cplusplus
}
#endif

#endif
