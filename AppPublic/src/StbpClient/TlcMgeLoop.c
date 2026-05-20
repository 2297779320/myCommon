/**
 * @file TlcMgeLoop.c
 * @brief 事件循环管理器实现 -- 为StbpClient提供事件循环支持
 *
 * @details
 * 实现 TlcMgeLoopCreate/Start/Stop/Destroy 等函数，
 * 为 STBP 客户端提供事件循环（消息泵）功能。
 * 基于 pthread 实现异步事件处理。
 */

#include "typedef.h"
#include "osal.h"
#include "tsk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/***********************************************************
*                    数据结构定义                          *
**********************************************************/

/**
 * @brief 事件循环对象
 */
typedef struct {
    BOOL            bRunning;          /**< 运行标志 */
    pthread_t       hThread;           /**< 线程句柄 */
    String128       strName;           /**< 循环名称 */
    void           *pUserData;         /**< 用户数据 */
} T_TlcMgeLoop;

/***********************************************************
*                    内部函数                              *
**********************************************************/

/**
 * @brief 事件循环线程函数
 */
static void* TlcMgeLoopThreadFunc(void* param)
{
    T_TlcMgeLoop *ptLoop = (T_TlcMgeLoop*)param;
    
    if (NULL == ptLoop) return NULL;
    
    dbprintf("[TlcMgeLoop] Thread '%s' started.\n", ptLoop->strName);
    
    /* 事件循环 */
    while (ptLoop->bRunning)
    {
        /* 处理事件（此处简化为休眠，实际应处理事件队列） */
        OSAL_Sleep(10);  /* 10ms */
    }
    
    dbprintf("[TlcMgeLoop] Thread '%s' stopped.\n", ptLoop->strName);
    return NULL;
}

/***********************************************************
*                    公开API                               *
**********************************************************/

/**
 * @brief 创建事件循环
 * @param phLoop 输出：事件循环句柄
 * @return 0成功，其他失败
 */
INT32 TlcMgeLoopCreate(tlHdl_t* phLoop)
{
    T_TlcMgeLoop *ptLoop = NULL;
    
    if (NULL == phLoop)
    {
        SysErr("[TlcMgeLoop] Invalid parameter\n");
        return -1;
    }
    
    /* 分配内存 */
    ptLoop = (T_TlcMgeLoop*)malloc(sizeof(T_TlcMgeLoop));
    if (NULL == ptLoop)
    {
        SysErr("[TlcMgeLoop] Memory allocation failed\n");
        return -2;
    }
    
    memset(ptLoop, 0, sizeof(T_TlcMgeLoop));
    ptLoop->bRunning = FALSE;
    strcpy(ptLoop->strName, "DefaultLoop");
    
    *phLoop = (tlHdl_t)ptLoop;
    
    dbprintf("[TlcMgeLoop] Created successfully\n");
    return 0;
}

/**
 * @brief 启动事件循环
 * @param hLoop 事件循环句柄
 * @param strName 循环名称
 * @return 0成功，其他失败
 */
INT32 TlcMgeLoopStart(tlHdl_t hLoop, const INT8* strName)
{
    T_TlcMgeLoop *ptLoop = (T_TlcMgeLoop*)hLoop;
    
    if (NULL == ptLoop)
    {
        SysErr("[TlcMgeLoop] Invalid handle\n");
        return -1;
    }
    
    if (ptLoop->bRunning)
    {
        SysErr("[TlcMgeLoop] Already running\n");
        return -2;
    }
    
    /* 设置名称 */
    if (NULL != strName)
    {
        strncpy(ptLoop->strName, strName, sizeof(ptLoop->strName) - 1);
    }
    
    /* 启动线程 */
    ptLoop->bRunning = TRUE;
    
    pthread_attr_t tAttr;
    pthread_attr_init(&tAttr);
    pthread_attr_setdetachstate(&tAttr, PTHREAD_CREATE_JOINABLE);
    
    INT32 iRet = pthread_create(&ptLoop->hThread, &tAttr, TlcMgeLoopThreadFunc, ptLoop);
    pthread_attr_destroy(&tAttr);
    
    if (0 != iRet)
    {
        SysErr("[TlcMgeLoop] Failed to create thread, ret=%d\n", iRet);
        ptLoop->bRunning = FALSE;
        return iRet;
    }
    
    dbprintf("[TlcMgeLoop] Started '%s'\n", ptLoop->strName);
    return 0;
}

/**
 * @brief 停止事件循环
 * @param hLoop 事件循环句柄
 * @return 0成功，其他失败
 */
INT32 TlcMgeLoopStop(tlHdl_t hLoop)
{
    T_TlcMgeLoop *ptLoop = (T_TlcMgeLoop*)hLoop;
    
    if (NULL == ptLoop)
    {
        SysErr("[TlcMgeLoop] Invalid handle\n");
        return -1;
    }
    
    if (!ptLoop->bRunning)
    {
        return 0;  /* 已经停止 */
    }
    
    /* 设置停止标志 */
    ptLoop->bRunning = FALSE;
    
    /* 等待线程结束 */
    if (ptLoop->hThread != 0)
    {
        pthread_join(ptLoop->hThread, NULL);
        ptLoop->hThread = 0;
    }
    
    dbprintf("[TlcMgeLoop] Stopped '%s'\n", ptLoop->strName);
    return 0;
}

/**
 * @brief 销毁事件循环
 * @param hLoop 事件循环句柄
 * @return 0成功，其他失败
 */
INT32 TlcMgeLoopDestroy(tlHdl_t hLoop)
{
    T_TlcMgeLoop *ptLoop = (T_TlcMgeLoop*)hLoop;
    
    if (NULL == ptLoop)
    {
        SysErr("[TlcMgeLoop] Invalid handle\n");
        return -1;
    }
    
    if (ptLoop->bRunning)
    {
        SysErr("[TlcMgeLoop] Must stop before destroy\n");
        return -2;
    }
    
    dbprintf("[TlcMgeLoop] Destroyed '%s'\n", ptLoop->strName);
    
    free(ptLoop);
    return 0;
}
