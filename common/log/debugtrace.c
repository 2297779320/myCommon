#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <pthread.h>
#include "defs.h"
#include "common/common.h"
#include "log.h"
#include "debugtrace.h"
#include "MO/MOCtrl.h"
#include "MI/MICtrl.h"


/***********************************************************
 *						常量定义		             *
 **********************************************************/
#define DEBUG_PORT	1778
#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 1  // 只允许一个客户端连接
// #define HEARTBEAT_INTERVAL 5  // 每5秒发送一次心跳
#define TIMEOUT_INTERVAL 5  // 超过10秒没有收到心跳信号，认为客户端断开

/***********************************************************
 *				文件内部使用的宏                      *
 **********************************************************/
#define TIMEOUT_SEC 1  // 设置超时为5秒
 /***********************************************************
 *			文件内部使用的数据类型 	*
 **********************************************************/

typedef struct {
    int port;
    TSK_Handle hDebugTsk;
    BOOL bTskDone;

    // void *pCtx ;
    void *pMOCtx;
    void *pMICtx;

    int server_socket;
    int client_socket;
    T_MutexObj mutex;
    time_t last_heartbeat_time;
} DebugServer;

/***********************************************************
 *						全局变量						*
 **********************************************************/
BOOL	g_bSysStatistics = FALSE;
BOOL	g_bFullDebug = FALSE;

/***********************************************************
 *						本地变量						*
 **********************************************************/
static DebugServer *g_tDebugServer = NULL; 


typedef E_StateCode (*LJDebugCb)(int argc, char *argv[],void *pUserArg);

typedef struct _debugtrace_set_cb
{
    INT8			*pcName;		/* 配置名称*/
	INT8			*pcParamDes;
    LJDebugCb cb;
}debugtrace_set_cb;


/***********************************************************
 * 						本地函数						*
 **********************************************************/

E_StateCode get_interface_ip(char *ipdst) {
    int sockfd;
    struct ifreq ifr;
    struct sockaddr_in *ipaddr;
    char ip[INET_ADDRSTRLEN];
    E_StateCode eCode = STATE_CODE_NO_ERROR;
 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return STATE_CODE_TIME_OUT;
    }
    
    strncpy(ifr.ifr_name, "eth1", IFNAMSIZ - 1);
    
    if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
        perror("ioctl SIOCGIFADDR");
        close(sockfd);
        return STATE_CODE_TIME_OUT;
    }
    
    ipaddr = (struct sockaddr_in *)&ifr.ifr_addr;
    inet_ntop(AF_INET, &ipaddr->sin_addr, ip, sizeof(ip));
    
    syslog("Interface eth1 IP: %s\n", ip);

    SAFESTRCPY(ipdst, ip, INET_ADDRSTRLEN);
    
    close(sockfd);
    return eCode;
}

E_StateCode resetheartbeat(int argc UNUSED, char *argv[] UNUSED,void *pUserArg UNUSED)
{
    DebugServer *ptObj = NULL;
    ptObj = (DebugServer *)pUserArg;
    ptObj->last_heartbeat_time = time(NULL);

    return STATE_CODE_NO_ERROR;
}

E_StateCode debugtrace_setloglevel(int argc UNUSED, char *argv[] UNUSED,void *pUserArg UNUSED)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    DebugServer *ptObj = NULL;
    ptObj = (DebugServer *)pUserArg;
    UINT32 uiLevel = 0;

    if (argc < 2)
	{
		syslog("Too few paramter, Usage::%s chNum level.\n", argv[0]);
		return STATE_CODE_INVALID_PARAM;
	}

    uiLevel = atoi(argv[1]);
	syslog("Set debug level to %d\n", uiLevel);
	log_set_level(uiLevel);
    return eCode;
}

E_StateCode debugtrace_setstopmode(int argc UNUSED, char *argv[] UNUSED, void *pUserArg UNUSED)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    DebugServer *ptObj = NULL;
    ptObj = (DebugServer *)pUserArg;
    uapi_avplay_stop_mode mode = UAPI_AVPLAY_STOP_MODE_BLACK;

    if (argc < 2)
	{
		syslog("Too few paramter, Usage::%s chNum level.\n", argv[0]);
		return STATE_CODE_INVALID_PARAM;
	}

    mode = atoi(argv[1]);
	set_stop_mode_internal(mode, ptObj->pMOCtx);
    return eCode;
}

E_StateCode debugtrace_showinfo(int argc UNUSED, char *argv[] UNUSED,void *pUserArg UNUSED)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    DebugServer *ptObj = NULL;
    ptObj = (DebugServer *)pUserArg;
	show_info_internal(ptObj->pMOCtx);
    return eCode;
}

E_StateCode debugtrace_setvideofmt(int argc, char *argv[], void *pUserArg )
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    DebugServer *ptObj = NULL;
    BOOL bInterlaced = FALSE;
    ptObj = (DebugServer *)pUserArg;
    uapi_video_format fmt = UAPI_VIDEO_FMT_CUSTOM;

    if (argc < 3)
	{
		syslog("Too few paramter, Usage::%s chNum level.\n", argv[0]);
		return STATE_CODE_INVALID_PARAM;
	}

    fmt = atoi(argv[1]);
    bInterlaced = atoi(argv[2]);
    set_video_format_internal(fmt, bInterlaced, ptObj->pMOCtx);
    return eCode;
}

E_StateCode debugtrace_reset(int argc UNUSED, char *argv[] UNUSED,void *pUserArg UNUSED)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    DebugServer *ptObj = NULL;
    ptObj = (DebugServer *)pUserArg;
    set_reset_internal(ptObj->pMOCtx);
    return eCode;
}

E_StateCode debugtrace_setvideopts(int argc UNUSED, char *argv[] UNUSED,void *pUserArg UNUSED)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    DebugServer *ptObj = NULL;
    ptObj = (DebugServer *)pUserArg;
    int16_t pts = 0;

    if (argc < 2)
	{
		syslog("Too few paramter, Usage::%s chNum level.\n", argv[0]);
		return STATE_CODE_INVALID_PARAM;
	}

    pts = atoi(argv[1]);
    set_video_pts_internal(pts, ptObj->pMOCtx);
    return eCode;
}

E_StateCode debugtrace_setaudiopts(int argc UNUSED, char *argv[] UNUSED,void *pUserArg UNUSED)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    DebugServer *ptObj = NULL;
    ptObj = (DebugServer *)pUserArg;
    int16_t pts = 0;

    if (argc < 2)
	{
		syslog("Too few paramter, Usage::%s chNum level.\n", argv[0]);
		return STATE_CODE_INVALID_PARAM;
	}

    pts = atoi(argv[1]);
    set_delay_internal(pts, ptObj->pMOCtx);
    return eCode;
}

E_StateCode debugtrace_setsyncmode(int argc UNUSED, char *argv[] UNUSED,void *pUserArg UNUSED)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    DebugServer *ptObj = NULL;
    ptObj = (DebugServer *)pUserArg;
    uapi_sync_ref_mode mode = UAPI_SYNC_REF_MODE_NONE;

    if (argc < 2)
	{
		syslog("Too few paramter, Usage::%s chNum level.\n", argv[0]);
		return STATE_CODE_INVALID_PARAM;
	}

    mode = atoi(argv[1]);
    set_sync_ref_mode_internal(mode, ptObj->pMOCtx);
    return eCode;
}

E_StateCode debugtrace_setprog(int argc UNUSED, char *argv[] UNUSED,void *pUserArg UNUSED)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    DebugServer *ptObj = NULL;
    ptObj = (DebugServer *)pUserArg;
    uint16_t prog_id = 0;

    if (argc < 2)
	{
		syslog("Too few paramter, Usage::%s chNum level.\n", argv[0]);
		return STATE_CODE_INVALID_PARAM;
	}

    prog_id = atoi(argv[1]);
    set_progid_internal(prog_id, ptObj->pMOCtx);
    return eCode;
}

E_StateCode debugtrace_setgain(int argc UNUSED, char *argv[] UNUSED,void *pUserArg UNUSED)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    DebugServer *ptObj = NULL;
    ptObj = (DebugServer *)pUserArg;
    uint8_t audio_type = 0;
    uint8_t audio_gain = 0;
    if (argc < 3)
	{
		syslog("Too few paramter, Usage::%s chNum level.\n", argv[0]);
		return STATE_CODE_INVALID_PARAM;
	}

    audio_type = atoi(argv[1]);
    audio_gain = atoi(argv[2]);
    set_audio_gain_internal(0, audio_type, audio_gain, ptObj->pMOCtx);
    return eCode;
}

/*MI*/
E_StateCode debugtrace_setaudiobitrate(int argc UNUSED, char *argv[] UNUSED,void *pUserArg UNUSED)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    DebugServer *ptObj = NULL;
    ptObj = (DebugServer *)pUserArg;
    uint32_t bitrate = 0;

    if (argc < 2)
    {
        syslog("Too few paramter, Usage::%s chNum level.\n", argv[0]);
        return STATE_CODE_INVALID_PARAM;
    }

    bitrate = atoi(argv[1]);
    MISetAudioBitrateInternal(bitrate, ptObj->pMICtx);
    return eCode;
}

E_StateCode debugtrace_setvideobitrate(int argc UNUSED, char *argv[] UNUSED,void *pUserArg UNUSED)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    DebugServer *ptObj = NULL;
    ptObj = (DebugServer *)pUserArg;
    uint32_t bitrate = 0;

    if (argc < 2)
    {
        syslog("Too few paramter, Usage::%s chNum level.\n", argv[0]);
        return STATE_CODE_INVALID_PARAM;
    }

    bitrate = atoi(argv[1]);
    MISetVideoBitrateInternal(bitrate, ptObj->pMICtx);
    return eCode;
}
E_StateCode debugtrace_setaudiocodec(int argc UNUSED, char *argv[] UNUSED,void *pUserArg UNUSED)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    DebugServer *ptObj = NULL;
    ptObj = (DebugServer *)pUserArg;
    // String64 strCodec = {0};

    if (argc < 2)
    {
        syslog("Too few paramter, Usage::%s chNum level.\n", argv[0]);
        return STATE_CODE_INVALID_PARAM;
    }

    // audio_codec = atoi(argv[1]);
    MISetAudioCodecInternal(argv[1], ptObj->pMICtx);
    return eCode;
}
E_StateCode debugtrace_setvideocodec(int argc UNUSED, char *argv[] UNUSED,void *pUserArg UNUSED)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    DebugServer *ptObj = NULL;
    ptObj = (DebugServer *)pUserArg;
    // uint8_t video_codec = 0;

    if (argc < 2)
    {
        syslog("Too few paramter, Usage::%s chNum level.\n", argv[0]);
        return STATE_CODE_INVALID_PARAM;
    }

    // video_codec = atoi(argv[1]);
    MISetVideoCodecInternal(argv[1], ptObj->pMICtx);
    return eCode;
}

E_StateCode debugtrace_printhelp(int argc UNUSED, char *argv[] UNUSED,void *pUserArg UNUSED);

static debugtrace_set_cb g_debugtrace_cb_map[] = {
    {"HEARTBEAT",     NULL,      resetheartbeat},
    {"setdebuglevel",  "level",     debugtrace_setloglevel},
    {SET_STOP_MODE,  "mode",     debugtrace_setstopmode},
    {SHOW_INFO,  NULL,     debugtrace_showinfo},
    {SET_VIDEO_FORMAT,  "fmt binterlaced",     debugtrace_setvideofmt},
    {SET_RESET,  NULL,     debugtrace_reset},
    {SET_VIDEO_PTS,  "ms",     debugtrace_setvideopts},
    {SET_DELAY,  "ms",     debugtrace_setaudiopts},
    {SET_SYNC_MODE,  "mode",     debugtrace_setsyncmode},
    {SET_PROG,  "index",     debugtrace_setprog},
    {SET_AUDIO_GAIN,  "0~100",     debugtrace_setgain},
    /*MI*/
    {SET_AUDIO_BITRATE,  "bitrate",     debugtrace_setaudiobitrate},
    {SET_VIDEO_BITRATE,  "bitrate",     debugtrace_setvideobitrate},
    {SET_AUDIO_CODEC,  "codec",     debugtrace_setaudiocodec},
    {SET_VIDEO_CODEC,  "codec",     debugtrace_setvideocodec},

    {"?",  NULL,     debugtrace_printhelp},
};

E_StateCode debugtrace_printhelp(int argc UNUSED, char *argv[] UNUSED,void *pUserArg UNUSED)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    char buf[1024] = {0};
    size_t buf_used = 0;
    debugtrace_set_cb *pMap = g_debugtrace_cb_map;

    INT32 map_size = (INT32)(sizeof(g_debugtrace_cb_map) / sizeof(g_debugtrace_cb_map[0]));
    for (int i = 1; i < map_size-1; i++)
    {
        int written = snprintf(buf + buf_used, sizeof(buf) - buf_used, "%s  %s \n",
            pMap[i].pcName,
            pMap[i].pcParamDes ? pMap[i].pcParamDes : "");
        if (written < 0 || (size_t)written >= sizeof(buf) - buf_used) {
            break; /* 缓冲区不足，停止追加 */
        }
        buf_used += (size_t)written;
    }
    LOG_INFO("Supported commands:\n%s", buf);
    return eCode;
}

E_StateCode debugtrace_msg_handle(const char* msg, void *pUserArg)
{
    char** argv = NULL;
    int argc = 0;
    E_StateCode eCode = STATE_CODE_OBJECT_BEYOND;
    int map_size = (int)(sizeof(g_debugtrace_cb_map) / sizeof(g_debugtrace_cb_map[0]));
    
    argc = string_to_argv(msg, &argv);
    if (argc <= 0 || argv == NULL) {
        return STATE_CODE_INVALID_PARAM;
    }
    for (int i = 0; i < map_size; i++) 
    {
        /* 精确匹配命令名，避免前缀误匹配 */
        if (strcmp(argv[0], g_debugtrace_cb_map[i].pcName) == 0)
        {
            eCode = g_debugtrace_cb_map[i].cb(argc, argv, pUserArg);
            break;
     	}
    }
    free_argv(argv, argc);
    return eCode;
}

void debug_trace(const char* message, int len)
{
    DebugServer *ptObj = g_tDebugServer;

    if (ptObj == NULL) {
        return;
    }

    OSAL_MutexLock(&ptObj->mutex);
    if (ptObj->client_socket <= 0)
    {
        OSAL_MutexUnlock(&ptObj->mutex);
        return;
    }

    send(ptObj->client_socket, message, len, 0);
    OSAL_MutexUnlock(&ptObj->mutex);
}

void handle_client(DebugServer *ptObj) 
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    fd_set read_fds;
    struct timeval timeout;
    int bytes_received = 0;
    char buffer[BUFFER_SIZE] = {0};
    ptObj->last_heartbeat_time = time(NULL);

    syslog("ptObj->client_socket is %d\n", ptObj->client_socket);

    while (!ptObj->bTskDone) 
    {
        timeout.tv_sec = 1;  
        timeout.tv_usec = 0;
        memset(buffer, 0x00, sizeof(buffer));

        FD_ZERO(&read_fds);
        FD_SET(ptObj->client_socket, &read_fds);

        int select_result = select(ptObj->client_socket + 1, &read_fds, NULL, NULL, &timeout);
        if (select_result > 0) 
        {
            if (FD_ISSET(ptObj->client_socket, &read_fds))
            {
                OSAL_MutexLock(&ptObj->mutex);
                bytes_received = recv(ptObj->client_socket, buffer, sizeof(buffer) - 1, 0);
                OSAL_MutexUnlock(&ptObj->mutex);
                if (bytes_received > 0) 
                {
                    buffer[bytes_received] = '\0';
                    eCode = debugtrace_msg_handle(buffer, ptObj);
                    if(!STATE_OK(eCode))
                    {
                        syswarn("debugtrace_msg_handle failed\n");
                    }
                }
                else if(bytes_received == 0)
                {
                    syslog("Client disconnected gracefully\n");
                    break;
                }
                else 
                {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) 
                    {
                        syslog("recv error: %s\n", strerror(errno));
                        syslog("Client disconnected or error occurred? %d, %s++\n", errno, strerror(errno));
                        break;
                    }
                }
            }
        }
        else if(select_result < 0)
        {
            if (errno != EINTR) 
            {
                syslog("select error: %s\n", strerror(errno));
                break;
            }
        }

        if (difftime(time(NULL), ptObj->last_heartbeat_time) > TIMEOUT_INTERVAL) 
        {
            syslog("Client timeout, closing connection...\n");
            break;
        }
    }

    OSAL_MutexLock(&ptObj->mutex);
    close(ptObj->client_socket);
    ptObj->client_socket = -1;
    OSAL_MutexUnlock(&ptObj->mutex);
}

void* debug_trace_thread(void* arg) 
{
    DebugServer *ptObj = NULL;
    struct timeval timeout;
    fd_set read_fds;
    int ret = 0;
    int opt = 1;
    char ip[INET_ADDRSTRLEN];
    E_StateCode eCode = STATE_CODE_NO_ERROR;

    if (arg == NULL) {
        syserr("Invalid argument to debug_trace_thread\n");
        return NULL;
    }

    signal(SIGPIPE, SIG_IGN);
    ptObj = (DebugServer *)arg;

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    if ((ptObj->server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        syserr("Socket creation failed");
        return NULL;
    }

    if (setsockopt(ptObj->server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(ptObj->server_socket);
        pthread_exit(NULL); /* 不能在线程中调用 exit()，否则终止整个进程 */
    }

    server_addr.sin_family = AF_INET;

    do
    {
        eCode = get_interface_ip(ip);
        if(!STATE_OK(eCode))
        {
            TSK_sleep(5000);
        }
    } while (!STATE_OK(eCode));
    
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(ptObj->port);

    while(bind(ptObj->server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) 
    {
        syswarn("Bind failed\n");
        TSK_sleep(5000);
    }

    if (listen(ptObj->server_socket, MAX_CONNECTIONS) == -1) {
        syserr("Listen failed\n");
        close(ptObj->server_socket);
        pthread_exit(NULL);
    }

    syslog("Debug Trace Server is running on ip %s port %d...\n",ip, ptObj->port);

    while (!ptObj->bTskDone) 
    {
        int client_socket = -1;
        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = 0;

        FD_ZERO(&read_fds);
        FD_SET(ptObj->server_socket, &read_fds);

        ret = select(ptObj->server_socket + 1, &read_fds, NULL, NULL, &timeout);
        if (ret == -1) {
            perror("Select failed");
            continue;
        } 
        else if (ret == 0) 
        {
            continue;
        } 
        else 
        {
            if (FD_ISSET(ptObj->server_socket, &read_fds)) 
            {
                client_socket = accept(ptObj->server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
                if(client_socket == -1)
                {  
                    syserr("Accept failed \n");
                    continue;
                }
            }
        }

        OSAL_MutexLock(&ptObj->mutex);
        ptObj->client_socket = client_socket;
        OSAL_MutexUnlock(&ptObj->mutex);

        syslog("Client connected\n");
        handle_client(ptObj);
    }

    close(ptObj->server_socket);

    return NULL;
}

E_StateCode debug_init(void *pMOCtx, void *pMICtx) 
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;

    if (g_tDebugServer != NULL) {
        printf("Debug server already initialized.\n");
        return -1;
    }

    g_tDebugServer = (DebugServer *)malloc(sizeof(DebugServer));
    if (g_tDebugServer == NULL) {
        syserr("Failed to allocate memory for DebugServer");
        return STATE_CODE_ALLOCATION_FAILURE;
    }

    memset(g_tDebugServer, 0, sizeof(DebugServer));
    g_tDebugServer->port = DEBUG_PORT;
    g_tDebugServer->bTskDone = FALSE;

    OSAL_MutexInit(&g_tDebugServer->mutex);

    TSK_Attrs tAttr = DEFAULT_TSK_ATTR;
    tAttr.name = "debugtrace";
    tAttr.bFifo = TRUE;
    tAttr.priority = 50;
    g_tDebugServer->hDebugTsk = TSK_create(debug_trace_thread, &tAttr, g_tDebugServer);
    if (!g_tDebugServer->hDebugTsk) 
    {
        syserr("TSK_create failed\n");
        free(g_tDebugServer);
        g_tDebugServer = NULL;
        return STATE_CODE_ALLOCATION_FAILURE;
    }

    eCode =log_register_debug_trace(debug_trace, NULL);
    if(!STATE_OK(eCode))
    {
        syserr("log_register_debug_trace failed\n");
        debug_destroy();
        return eCode;
    }
    g_tDebugServer->pMOCtx = pMOCtx;
    g_tDebugServer->pMICtx = pMICtx;
    return eCode;
}

E_StateCode debug_destroy()
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;

    if (g_tDebugServer == NULL) {
        printf("Debug server not initialized.\n");
        return STATE_CODE_INVALID_HANDLE;
    }

    log_unregister_debug_trace();

    if (g_tDebugServer->hDebugTsk) {
        g_tDebugServer->bTskDone = TRUE;
        /* 关闭 server socket 以解除 accept() 阻塞 */
        if (g_tDebugServer->server_socket >= 0) {
            close(g_tDebugServer->server_socket);
            g_tDebugServer->server_socket = -1;
        }
        TSK_delete(g_tDebugServer->hDebugTsk);
        g_tDebugServer->hDebugTsk = NULL;
    }

    OSAL_MutexDestroy(&g_tDebugServer->mutex);

    free(g_tDebugServer);
    g_tDebugServer = NULL;

    return eCode;
}
