/**
 * File:   ring_queue.c
 * Author: AWTK Develop Team
 * Brief:  ring_queue (基于ring_buffer的上层封装实现，线程安全，支持超时)
 *
 * Copyright (c) 2018 - 2025 Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * History:
 * ================================================================
 * 2025-12-05 Created as wrapper around ring_buffer for easier usage
 * 2025-12-05 添加超时机制和线程安全支持 (版本3)
 *
 */

#include "ring_queue.h"
#include "defs.h"
#include <string.h>

ring_queue_t* ring_queue_create(UINT32 element_size, UINT32 max_elements) {
  ring_queue_t* queue = NULL;
  UINT32 init_capacity = 0;
  UINT32 max_capacity = 0;
  
  return_value_if_fail(element_size > 0 && max_elements > 0, NULL);
  
  queue = (ring_queue_t*)malloc(sizeof(ring_queue_t));
  return_value_if_fail(queue != NULL, NULL);
  
  /* 计算底层buffer的容量(字节) */
  init_capacity = tk_max(32, element_size * (max_elements / 2));  /* 初始容量为最大容量的一半，但至少32字节 */
  max_capacity = element_size * max_elements;  /* 最大容量 */
  
  queue->buffer = ring_buffer_create(init_capacity, max_capacity);
  if (queue->buffer == NULL) {
    free(queue);
    return NULL;
  }
  
  queue->element_size = element_size;
  queue->max_elements = max_elements;
  
  return queue;
}

E_StateCode ring_queue_destroy(ring_queue_t* queue) {
  return_value_if_fail(queue != NULL, STATE_CODE_INVALID_PARAM);
  
  if (queue->buffer != NULL) {
    ring_buffer_destroy(queue->buffer);
    queue->buffer = NULL;
  }
  
  free(queue);
  return STATE_CODE_NO_ERROR;
}

/* 带超时的操作 */
E_StateCode ring_queue_push_timeout(ring_queue_t* queue, const void* element, UINT32 timeout_ms) {
  return_value_if_fail(queue != NULL && element != NULL, STATE_CODE_INVALID_PARAM);
  
  E_StateCode result = ring_buffer_write_len_timeout(queue->buffer, element, queue->element_size, timeout_ms);
  return result;
}

E_StateCode ring_queue_pop_timeout(ring_queue_t* queue, void* element, UINT32 timeout_ms) {
  return_value_if_fail(queue != NULL && element != NULL, STATE_CODE_INVALID_PARAM);
  
  E_StateCode result = ring_buffer_read_len_timeout(queue->buffer, element, queue->element_size, timeout_ms);
  return result;
}

E_StateCode ring_queue_peek_front_timeout(ring_queue_t* queue, void* element, UINT32 timeout_ms) {
  return_value_if_fail(queue != NULL && element != NULL, STATE_CODE_INVALID_PARAM);
  /* ring_buffer_peek_timeout 内部加锁，保存并恢复 r，不会修改读指针，直接使用即可 */
  UINT32 bytes_read = ring_buffer_peek_timeout(queue->buffer, element, queue->element_size, timeout_ms);
  return (bytes_read == queue->element_size) ? STATE_CODE_NO_ERROR : STATE_CODE_TIME_OUT;
}

E_StateCode ring_queue_peek_back_timeout(ring_queue_t* queue, void* element, UINT32 timeout_ms) {
  UINT32 size = 0;
  return_value_if_fail(queue != NULL && element != NULL, STATE_CODE_INVALID_PARAM);
  
  ring_buffer_t* buffer = queue->buffer;
  
  ring_buffer_lock(buffer);

  /* 等待有数据可读（内部会检查并等待） */
  if (ring_buffer_wait_for_read(buffer, timeout_ms) != STATE_CODE_NO_ERROR) {
    ring_buffer_unlock(buffer);
    return STATE_CODE_TIME_OUT;
  }
  
  size = ring_buffer_size(buffer);
  if (size < queue->element_size) {
    ring_buffer_unlock(buffer);
    return STATE_CODE_WAIT_MORE_DATA;
  }
  
  /* 在持锁状态下临时移动读指针到队尾，读取后恢复 */
  UINT32 saved_r = buffer->r;
  BOOL saved_full = buffer->full;
  buffer->r = (buffer->w - queue->element_size + buffer->capacity) % buffer->capacity;
  buffer->full = FALSE;

  /* 使用无锁内部函数直接读取（已持锁） */
  /* ring_buffer_peek_timeout 会重新加锁，改用直接 memcpy */
  uint8_t* src = buffer->data + buffer->r;
  memcpy(element, src, queue->element_size);
  UINT32 bytes_read = queue->element_size;
  
  buffer->r = saved_r;
  buffer->full = saved_full;
  ring_buffer_unlock(buffer);
  
  return (bytes_read == queue->element_size) ? STATE_CODE_NO_ERROR : STATE_CODE_INVALID_PARAM;
}

E_StateCode ring_queue_push_array_timeout(ring_queue_t* queue, const void* elements, UINT32 count, UINT32 timeout_ms) {
  UINT32 bytes_to_write = 0;
  return_value_if_fail(queue != NULL && elements != NULL, STATE_CODE_INVALID_PARAM);
  
  bytes_to_write = queue->element_size * count;
  E_StateCode result = ring_buffer_write_len_timeout(queue->buffer, elements, bytes_to_write, timeout_ms);
  return result;
}

E_StateCode ring_queue_pop_array_timeout(ring_queue_t* queue, void* elements, UINT32 count, UINT32 timeout_ms) {
  UINT32 bytes_to_read = 0;
  return_value_if_fail(queue != NULL && elements != NULL, STATE_CODE_INVALID_PARAM);
  
  bytes_to_read = queue->element_size * count;
  E_StateCode result = ring_buffer_read_len_timeout(queue->buffer, elements, bytes_to_read, timeout_ms);
  return result;
}

/* 兼容性接口 - 默认无超时 */
BOOL ring_queue_push(ring_queue_t* queue, const void* element) {
  return ring_queue_push_timeout(queue, element, RING_QUEUE_NO_TIMEOUT) == STATE_CODE_NO_ERROR;
}

BOOL ring_queue_pop(ring_queue_t* queue, void* element) {
  return ring_queue_pop_timeout(queue, element, RING_QUEUE_NO_TIMEOUT) == STATE_CODE_NO_ERROR;
}

BOOL ring_queue_peek_front(ring_queue_t* queue, void* element) {
  return ring_queue_peek_front_timeout(queue, element, RING_QUEUE_NO_TIMEOUT) == STATE_CODE_NO_ERROR;
}

BOOL ring_queue_peek_back(ring_queue_t* queue, void* element) {
  return ring_queue_peek_back_timeout(queue, element, RING_QUEUE_NO_TIMEOUT) == STATE_CODE_NO_ERROR;
}

UINT32 ring_queue_push_array(ring_queue_t* queue, const void* elements, UINT32 count) {
  if (ring_queue_push_array_timeout(queue, elements, count, RING_QUEUE_NO_TIMEOUT) == STATE_CODE_NO_ERROR) {
    return count;
  }
  return 0;
}

UINT32 ring_queue_pop_array(ring_queue_t* queue, void* elements, UINT32 count) {
  if (ring_queue_pop_array_timeout(queue, elements, count, RING_QUEUE_NO_TIMEOUT) == STATE_CODE_NO_ERROR) {
    return count;
  }
  return 0;
}

/* 状态查询 - 无锁（快速查询） */
BOOL ring_queue_is_full(ring_queue_t* queue) {
  return_value_if_fail(queue != NULL, FALSE);
  return ring_buffer_free_size(queue->buffer) < queue->element_size;
}

BOOL ring_queue_is_empty(ring_queue_t* queue) {
  return_value_if_fail(queue != NULL, FALSE);
  return ring_buffer_size(queue->buffer) < queue->element_size;
}

UINT32 ring_queue_size(ring_queue_t* queue) {
  return_value_if_fail(queue != NULL, 0);
  return ring_buffer_size(queue->buffer) / queue->element_size;
}

UINT32 ring_queue_capacity(ring_queue_t* queue) {
  return_value_if_fail(queue != NULL, 0);
  return ring_buffer_capacity(queue->buffer) / queue->element_size;
}

E_StateCode ring_queue_clear(ring_queue_t* queue) {
  return_value_if_fail(queue != NULL, STATE_CODE_INVALID_PARAM);
  return ring_buffer_reset(queue->buffer);
}

ring_buffer_t* ring_queue_get_buffer(ring_queue_t* queue) {
  return_value_if_fail(queue != NULL, NULL);
  return queue->buffer;
}