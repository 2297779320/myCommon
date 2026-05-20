#ifndef _H_SAMPLE_GLOBAL_
#define _H_SAMPLE_GLOBAL_

#ifdef __cplusplus
extern "C" {
#endif

#include "typedef.h"

/**************************************************************************
 *                         STBP Constants                                 *
 **************************************************************************/
#define STBP_DEFAULT_PORT       8300
#define STBP_CONNECT_IP         "127.0.0.1"

/**************************************************************************
 *                         Topic Patterns                                 *
 **************************************************************************/
#define JSON_DEV_NAME           "sample.v1.devName"
#define JSON_LOCAL_IPADDR       "sample.v1.localIp"
#define JSON_SERVER_INFO        "sample.v1.serverInfo"

/* Sensor topics */
#define TOPIC_SENSOR_TEMP       "$report.$data.0.$l.$i.sample.v1.sensor.temp"
#define TOPIC_SENSOR_HUMI       "$report.$data.0.$l.$i.sample.v1.sensor.humi"
#define TOPIC_SENSOR_READ       "$request.$get.0.$l.$i.sample.v1.sensor.read"

/* Device control topics */
#define TOPIC_DEV_CTRL_ON       "$request.$set.0.$l.$i.sample.v1.devCtrl.on"
#define TOPIC_DEV_CTRL_OFF      "$request.$set.0.$l.$i.sample.v1.devCtrl.off"
#define TOPIC_DEV_STATE         "$report.$data.0.$l.$i.sample.v1.devCtrl.state"

/* Heartbeat */
#define TOPIC_HEARTBEAT         "$report.heartbeat.0.$l.$i.sample.v1.state"

/* System */
#define TOPIC_SYS_READY         "$report.$data.0.$l.$i.sample.v1.state"

/**************************************************************************
 *                         Global Functions                               *
 **************************************************************************/
void *GetUserClientHandle(void);
INT32 GetStbpServerPort(void);

#ifdef __cplusplus
}
#endif

#endif
