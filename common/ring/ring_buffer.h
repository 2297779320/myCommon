/**
 * @file ring_buffer.h
 * @brief 底层环形缓冲区 -- 线程安全版本，支持超时读写
 *
 * @details
 * 基于 AWTK ring_buffer 改造，添加 pthread 互斥锁 + 条件变量实现线程安全。
 * 支持超时读写、peek（不消费读取）、定长读写等高级操作。
 *
 * @see defs.h（依赖 UINT32, UINT8, BOOL, E_StateCode）
 * @see ring_queue.h（被依赖）
 */

#ifndef TK_RING_BUFFER_H
#define TK_RING_BUFFER_H

#include "defs.h"

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <time.h>
#endif

/* 超时相关定义 */
#define RING_BUFFER_INFINITE_TIMEOUT ((UINT32)(-1))  /* 无限等待 */
#define RING_BUFFER_NO_TIMEOUT 0                      /* 不等待 */

/**
 * @class ring_buffer_t
 * 底层环形缓冲区数据结构（线程安全）。
 */
typedef struct _ring_buffer_t {
  /**
   * @property {UINT32} r
   * @annotation ["readable"]
   * 读取位置。
   */
  UINT32 r;
  /**
   * @property {UINT32} w
   * @annotation ["readable"]
   * 写入位置。
   */
  UINT32 w;
  /**
   * @property {BOOL} full
   * @annotation ["readable"]
   * 是否满。
   */
  BOOL full;
  /**
   * @property {UINT32} capacity
   * @annotation ["readable"]
   * 当前容量。
   */
  UINT32 capacity;
  /**
   * @property {UINT32} max_capacity
   * @annotation ["readable"]
   * 最大容量。
   */
  UINT32 max_capacity;
  /**
   * @property {uint8_t*} data
   * @annotation ["readable"]
   * 数据缓冲区。
   */
  UINT8* data;
  
  /* 线程安全相关 */
#ifdef WIN32
  CRITICAL_SECTION mutex;        /* 互斥锁 */
  HANDLE read_cond;              /* 读条件变量 */
  HANDLE write_cond;             /* 写条件变量 */
#else
  pthread_mutex_t mutex;         /* 互斥锁 */
  pthread_cond_t read_cond;      /* 读条件变量 */
  pthread_cond_t write_cond;     /* 写条件变量 */
#endif

} ring_buffer_t;

/* 底层环形缓冲区API - 线程安全版本 */
ring_buffer_t* ring_buffer_create(UINT32 init_capacity, UINT32 max_capacity);
E_StateCode ring_buffer_destroy(ring_buffer_t* ring_buffer);

/* 状态查询 - 无锁版本（快速查询） */
BOOL ring_buffer_is_full(ring_buffer_t* ring_buffer);
BOOL ring_buffer_is_empty(ring_buffer_t* ring_buffer);
UINT32 ring_buffer_size(ring_buffer_t* ring_buffer);
UINT32 ring_buffer_free_size(ring_buffer_t* ring_buffer);
UINT32 ring_buffer_capacity(ring_buffer_t* ring_buffer);

/* 基础读写操作 - 带超时 */
UINT32 ring_buffer_read_timeout(ring_buffer_t* ring_buffer, void* buff, UINT32 size, UINT32 timeout_ms);
UINT32 ring_buffer_peek_timeout(ring_buffer_t* ring_buffer, void* buff, UINT32 size, UINT32 timeout_ms);
UINT32 ring_buffer_write_timeout(ring_buffer_t* ring_buffer, const void* buff, UINT32 size, UINT32 timeout_ms);

/* 指定长度操作 - 带超时 */
E_StateCode ring_buffer_read_len_timeout(ring_buffer_t* ring_buffer, void* buff, UINT32 size, UINT32 timeout_ms);
E_StateCode ring_buffer_write_len_timeout(ring_buffer_t* ring_buffer, const void* buff, UINT32 size, UINT32 timeout_ms);
E_StateCode ring_buffer_skip_timeout(ring_buffer_t* ring_buffer, UINT32 size, UINT32 timeout_ms);

/* 兼容性接口 - 默认无超时 */
UINT32 ring_buffer_read(ring_buffer_t* ring_buffer, void* buff, UINT32 size);
UINT32 ring_buffer_peek(ring_buffer_t* ring_buffer, void* buff, UINT32 size);
UINT32 ring_buffer_write(ring_buffer_t* ring_buffer, const void* buff, UINT32 size);
E_StateCode ring_buffer_read_len(ring_buffer_t* ring_buffer, void* buff, UINT32 size);
E_StateCode ring_buffer_write_len(ring_buffer_t* ring_buffer, const void* buff, UINT32 size);
E_StateCode ring_buffer_skip(ring_buffer_t* ring_buffer, UINT32 size);

/* 其他操作 */
E_StateCode ring_buffer_reset(ring_buffer_t* ring_buffer);
E_StateCode ring_buffer_set_read_cursor(ring_buffer_t* ring_buffer, UINT32 r);
E_StateCode ring_buffer_set_read_cursor_delta(ring_buffer_t* ring_buffer, UINT32 r_delta);
E_StateCode ring_buffer_set_write_cursor(ring_buffer_t* ring_buffer, UINT32 w);
E_StateCode ring_buffer_set_write_cursor_delta(ring_buffer_t* ring_buffer, UINT32 w_delta);
E_StateCode ring_buffer_ensure_write_space(ring_buffer_t* ring_buffer, UINT32 size);

/* 线程安全辅助函数 */
E_StateCode ring_buffer_lock(ring_buffer_t* ring_buffer);
E_StateCode ring_buffer_unlock(ring_buffer_t* ring_buffer);
E_StateCode ring_buffer_wait_for_read(ring_buffer_t* ring_buffer, UINT32 timeout_ms);
E_StateCode ring_buffer_wait_for_write(ring_buffer_t* ring_buffer, UINT32 timeout_ms);
E_StateCode ring_buffer_signal_read(ring_buffer_t* ring_buffer);
E_StateCode ring_buffer_signal_write(ring_buffer_t* ring_buffer);

#endif /*TK_RING_BUFFER_H*/