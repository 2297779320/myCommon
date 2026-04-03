/**
 * File:   ring_queue.h
 * Author: AWTK Develop Team
 * Brief:  ring_queue (基于ring_buffer的上层封装，线程安全，支持超时)
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

#ifndef TK_RING_QUEUE_H
#define TK_RING_QUEUE_H

#include "ring_buffer.h"

/* 超时相关定义（继承自ring_buffer） */
#define RING_QUEUE_INFINITE_TIMEOUT RING_BUFFER_INFINITE_TIMEOUT
#define RING_QUEUE_NO_TIMEOUT RING_BUFFER_NO_TIMEOUT

/**
 * @class ring_queue_t
 * 环形队列 - ring_buffer的上层封装，提供更易用的接口（线程安全，支持超时）。
 */
typedef struct _ring_queue_t {
  /**
   * @property {ring_buffer_t*} buffer
   * @annotation ["readable"]
   * 底层环形缓冲区。
   */
  ring_buffer_t* buffer;
  
  /**
   * @property {UINT32} element_size
   * @annotation ["readable"]
   * 每个元素的大小(字节)。
   */
  UINT32 element_size;
  
  /**
   * @property {UINT32} max_elements
   * @annotation ["readable"]
   * 最大元素数量。
   */
  UINT32 max_elements;

} ring_queue_t;

/* 环形队列API - 更易用的上层接口（支持超时） */

/**
 * @method ring_queue_create
 * @annotation ["constructor"]
 * 创建环形队列对象。
 *
 * @param {UINT32} element_size 每个元素的大小(字节)。
 * @param {UINT32} max_elements 最大元素数量。
 *
 * @return {ring_queue_t*} 环形队列对象。
 */
ring_queue_t* ring_queue_create(UINT32 element_size, UINT32 max_elements);

/**
 * @method ring_queue_destroy
 * 销毁环形队列。
 *
 * @param {ring_queue_t*} queue 环形队列对象。
 *
 * @return {E_StateCode} 返回RET_OK表示成功，否则表示失败。
 */
E_StateCode ring_queue_destroy(ring_queue_t* queue);

/* 基础操作 - 带超时 */
/**
 * @method ring_queue_push_timeout
 * 向队列尾部推入一个元素（带超时）。
 *
 * @param {ring_queue_t*} queue 环形队列对象。
 * @param {const void*} element 要推入的元素。
 * @param {UINT32} timeout_ms 超时时间（毫秒），0表示不等待，-1表示无限等待。
 *
 * @return {E_StateCode} 返回RET_OK表示成功，RET_TIMEOUT表示超时，其他表示失败。
 */
E_StateCode ring_queue_push_timeout(ring_queue_t* queue, const void* element, UINT32 timeout_ms);

/**
 * @method ring_queue_pop_timeout
 * 从队列头部弹出一个元素（带超时）。
 *
 * @param {ring_queue_t*} queue 环形队列对象。
 * @param {void*} element 接收元素的缓冲区。
 * @param {UINT32} timeout_ms 超时时间（毫秒），0表示不等待，-1表示无限等待。
 *
 * @return {E_StateCode} 返回RET_OK表示成功，RET_TIMEOUT表示超时，其他表示失败。
 */
E_StateCode ring_queue_pop_timeout(ring_queue_t* queue, void* element, UINT32 timeout_ms);

/**
 * @method ring_queue_peek_front_timeout
 * 查看队列头部元素(不移除，带超时)。
 *
 * @param {ring_queue_t*} queue 环形队列对象。
 * @param {void*} element 接收元素的缓冲区。
 * @param {UINT32} timeout_ms 超时时间（毫秒），0表示不等待，-1表示无限等待。
 *
 * @return {E_StateCode} 返回RET_OK表示成功，RET_TIMEOUT表示超时，其他表示失败。
 */
E_StateCode ring_queue_peek_front_timeout(ring_queue_t* queue, void* element, UINT32 timeout_ms);

/**
 * @method ring_queue_peek_back_timeout
 * 查看队列尾部元素(不移除，带超时)。
 *
 * @param {ring_queue_t*} queue 环形队列对象。
 * @param {void*} element 接收元素的缓冲区。
 * @param {UINT32} timeout_ms 超时时间（毫秒），0表示不等待，-1表示无限等待。
 *
 * @return {E_StateCode} 返回RET_OK表示成功，RET_TIMEOUT表示超时，其他表示失败。
 */
E_StateCode ring_queue_peek_back_timeout(ring_queue_t* queue, void* element, UINT32 timeout_ms);

/* 批量操作 - 带超时 */
/**
 * @method ring_queue_push_array_timeout
 * 推入多个元素（带超时）。
 *
 * @param {ring_queue_t*} queue 环形队列对象。
 * @param {const void*} elements 元素数组。
 * @param {UINT32} count 元素数量。
 * @param {UINT32} timeout_ms 超时时间（毫秒），0表示不等待，-1表示无限等待。
 *
 * @return {E_StateCode} 返回RET_OK表示成功，RET_TIMEOUT表示超时，其他表示失败。
 */
E_StateCode ring_queue_push_array_timeout(ring_queue_t* queue, const void* elements, UINT32 count, UINT32 timeout_ms);

/**
 * @method ring_queue_pop_array_timeout
 * 弹出多个元素（带超时）。
 *
 * @param {ring_queue_t*} queue 环形队列对象。
 * @param {void*} elements 接收元素的缓冲区。
 * @param {UINT32} count 要弹出的元素数量。
 * @param {UINT32} timeout_ms 超时时间（毫秒），0表示不等待，-1表示无限等待。
 *
 * @return {E_StateCode} 返回RET_OK表示成功，RET_TIMEOUT表示超时，其他表示失败。
 */
E_StateCode ring_queue_pop_array_timeout(ring_queue_t* queue, void* elements, UINT32 count, UINT32 timeout_ms);

/* 兼容性接口 - 默认无超时 */
/**
 * @method ring_queue_push
 * 向队列尾部推入一个元素（默认无超时）。
 *
 * @param {ring_queue_t*} queue 环形队列对象。
 * @param {const void*} element 要推入的元素。
 *
 * @return {BOOL} 成功返回TRUE，失败返回FALSE。
 */
BOOL ring_queue_push(ring_queue_t* queue, const void* element);

/**
 * @method ring_queue_pop
 * 从队列头部弹出一个元素（默认无超时）。
 *
 * @param {ring_queue_t*} queue 环形队列对象。
 * @param {void*} element 接收元素的缓冲区。
 *
 * @return {BOOL} 成功返回TRUE，失败返回FALSE。
 */
BOOL ring_queue_pop(ring_queue_t* queue, void* element);

/**
 * @method ring_queue_peek_front
 * 查看队列头部元素(不移除，默认无超时)。
 *
 * @param {ring_queue_t*} queue 环形队列对象。
 * @param {void*} element 接收元素的缓冲区。
 *
 * @return {BOOL} 成功返回TRUE，失败返回FALSE。
 */
BOOL ring_queue_peek_front(ring_queue_t* queue, void* element);

/**
 * @method ring_queue_peek_back
 * 查看队列尾部元素(不移除，默认无超时)。
 *
 * @param {ring_queue_t*} queue 环形队列对象。
 * @param {void*} element 接收元素的缓冲区。
 *
 * @return {BOOL} 成功返回TRUE，失败返回FALSE。
 */
BOOL ring_queue_peek_back(ring_queue_t* queue, void* element);

/**
 * @method ring_queue_push_array
 * 推入多个元素（默认无超时）。
 *
 * @param {ring_queue_t*} queue 环形队列对象。
 * @param {const void*} elements 元素数组。
 * @param {UINT32} count 元素数量。
 *
 * @return {UINT32} 实际推入的元素数量。
 */
UINT32 ring_queue_push_array(ring_queue_t* queue, const void* elements, UINT32 count);

/**
 * @method ring_queue_pop_array
 * 弹出多个元素（默认无超时）。
 *
 * @param {ring_queue_t*} queue 环形队列对象。
 * @param {void*} elements 接收元素的缓冲区。
 * @param {UINT32} count 要弹出的元素数量。
 *
 * @return {UINT32} 实际弹出的元素数量。
 */
UINT32 ring_queue_pop_array(ring_queue_t* queue, void* elements, UINT32 count);

/* 状态查询 - 无锁（快速查询） */
/**
 * @method ring_queue_is_full
 * 检查队列是否满。
 *
 * @param {ring_queue_t*} queue 环形队列对象。
 *
 * @return {BOOL} 是否满。
 */
BOOL ring_queue_is_full(ring_queue_t* queue);

/**
 * @method ring_queue_is_empty
 * 检查队列是否空。
 *
 * @param {ring_queue_t*} queue 环形队列对象。
 *
 * @return {BOOL} 是否空。
 */
BOOL ring_queue_is_empty(ring_queue_t* queue);

/**
 * @method ring_queue_size
 * 获取队列中元素数量。
 *
 * @param {ring_queue_t*} queue 环形队列对象。
 *
 * @return {UINT32} 元素数量。
 */
UINT32 ring_queue_size(ring_queue_t* queue);

/**
 * @method ring_queue_capacity
 * 获取队列容量(元素数量)。
 *
 * @param {ring_queue_t*} queue 环形队列对象。
 *
 * @return {UINT32} 容量。
 */
UINT32 ring_queue_capacity(ring_queue_t* queue);

/**
 * @method ring_queue_clear
 * 清空队列。
 *
 * @param {ring_queue_t*} queue 环形队列对象。
 *
 * @return {E_StateCode} 返回RET_OK表示成功，否则表示失败。
 */
E_StateCode ring_queue_clear(ring_queue_t* queue);

/* 高级功能 - 仍可访问底层ring_buffer的完整功能 */
/**
 * @method ring_queue_get_buffer
 * 获取底层ring_buffer对象(用于高级操作)。
 *
 * @param {ring_queue_t*} queue 环形队列对象。
 *
 * @return {ring_buffer_t*} 底层ring_buffer对象。
 */
ring_buffer_t* ring_queue_get_buffer(ring_queue_t* queue);

#endif /*TK_RING_QUEUE_H*/