#include "msgqueue.h"

MsgQueueHandle* msg_queue_create(uint32_t max_msg_count)
{
    if (max_msg_count == 0)
    {
        return NULL;
    }

    MsgQueueHandle* handle = (MsgQueueHandle*)malloc(sizeof(MsgQueueHandle));
    if (handle == NULL)
    {
        return NULL;
    }
    memset(handle, 0, sizeof(MsgQueueHandle));

    handle->que_id = CommQue_Create(max_msg_count, sizeof(T_Msg), NULL);

    if (handle->que_id == NULL)
    {
        free(handle);
        return NULL;
    }

    return handle;
}

static E_StateCode clear_queue(MsgQueueHandle* handle)
{
    if(!handle)
    {
        return STATE_CODE_INVALID_PARAM;
    }

    /* 将所有满包中的 pcBody 释放，然后一次性清空队列 */
    T_Msg* msg = NULL;
    while (1)
    {
        msg = CommQue_GetFull(handle->que_id, OSAL_TIMEOUT_NONE);
        if (!msg)
        {
            break;
        }
        if (msg->pcBody)
        {
            free(msg->pcBody);
            msg->pcBody = NULL;
        }
        CommQue_PutEmpty(handle->que_id, msg);
    }

    return STATE_CODE_NO_ERROR;
}


void msg_queue_destroy(MsgQueueHandle* handle)
{
    if (handle == NULL)
    {
        return;
    }
    clear_queue(handle);

    if (handle->que_id)
    {
        CommQue_Delete(handle->que_id);
    }

    free(handle);
}

E_StateCode msg_queue_send(MsgQueueHandle* handle, T_Msg* pSrcmsg, int32_t timeout)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    if (handle == NULL || handle->que_id == NULL)
    {
        return STATE_CODE_INVALID_PARAM;
    }

    // 1. 从队列的消息池中获取一个空的 T_Msg 结构体
    T_Msg* msg = (T_Msg*)CommQue_GetEmpty(handle->que_id, timeout);
    if (msg == NULL)
    {
        // 超时或没有可用的空消息
        return STATE_CODE_TIME_OUT;
    }

    // 2. 填充消息内容
    // msg->pcMsg = pSrcmsg->pcMsg;
    msg->pcBody = pSrcmsg->pcBody;
    msg->uiCallId = pSrcmsg->uiCallId;
    msg->uiCommand = pSrcmsg->uiCommand;
    SAFESTRCPY(msg->strMsgId, pSrcmsg->strMsgId, sizeof(msg->strMsgId));

    // 3. 将填充好的消息放入同一个队列，使其变为“满”状态
    if (CommQue_PutFull(handle->que_id, msg) != 0)
    {
        // 放入失败，将消息结构体放回空队列
        CommQue_PutEmpty(handle->que_id, msg);
        return STATE_CODE_TIME_OUT;
    }

    return eCode;
}

T_Msg* msg_queue_receive(MsgQueueHandle* handle, int32_t timeout)
{
    if (handle == NULL || handle->que_id == NULL)
    {
        return NULL;
    }

    return (T_Msg*)CommQue_GetFull(handle->que_id, timeout);
}

E_StateCode msg_queue_release_msg(MsgQueueHandle* handle, T_Msg* msg)
{
    if (handle == NULL || handle->que_id == NULL || msg == NULL)
    {
        return STATE_CODE_INVALID_PARAM;
    }
    
    return CommQue_PutEmpty(handle->que_id, msg);
}

uint32_t msg_queue_get_pending_count(MsgQueueHandle* handle)
{
    if (handle == NULL || handle->que_id == NULL)
    {
        return 0;
    }
    return CommQue_GetFullCount(handle->que_id);
}