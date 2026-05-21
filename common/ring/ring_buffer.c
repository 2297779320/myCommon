/**
 * File:   ring_buffer.c
 * Author: AWTK Develop Team
 * Brief:  ring_buffer (底层环形缓冲区实现，线程安全版本)
 *
 * Copyright (c) 2019 - 2025 Guangzhou ZHIYUAN Electronics Co.,Ltd.
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
 * 2019-10-06 Li XianJing <xianjimli@hotmail.com> created
 * 2025-12-05 作为ring_queue的底层实现
 * 2025-12-05 添加线程安全机制和超时支持 (版本3)
 *
 */

#include "ring_buffer.h"
#include "defs.h"
#include "time.h"
#include <string.h>

/* AWTK 兼容宏映射到 defs.h 中的定义 */
#ifndef tk_max
#define tk_max(a, b) lj_max(a, b)
#endif
#ifndef tk_min
#define tk_min(a, b) lj_min(a, b)
#endif
#ifndef TKMEM_ALLOC
#define TKMEM_ALLOC(size) LJMEM_ALLOC(size)
#endif
#ifndef TKMEM_FREE
#define TKMEM_FREE(ptr) LJMEM_FREE(ptr)
#endif

/* 平台相关的等待函数 */
#ifdef WIN32
static DWORD get_current_time_ms() {
  return GetTickCount();
}

static E_StateCode wait_condition(HANDLE cond, HANDLE mutex, UINT32 timeout_ms) {
  DWORD result;
  
  /* 释放互斥锁 */
  LeaveCriticalSection(mutex);
  
  if (timeout_ms == RING_BUFFER_INFINITE_TIMEOUT) {
    result = WaitForSingleObject(cond, INFINITE);
  } else if (timeout_ms == RING_BUFFER_NO_TIMEOUT) {
    result = WAIT_TIMEOUT;
  } else {
    result = WaitForSingleObject(cond, timeout_ms);
  }
  
  /* 重新获取互斥锁 */
  EnterCriticalSection(mutex);
  
  return (result == WAIT_OBJECT_0) ? STATE_CODE_NO_ERROR : STATE_CODE_TIME_OUT;
}

static void signal_condition(HANDLE cond) {
  SetEvent(cond);
}
#else
static UINT32 get_current_time_ms() {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return (UINT32)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static E_StateCode wait_condition(pthread_cond_t* cond, pthread_mutex_t* mutex, UINT32 timeout_ms) {
  int result;
  
  if (timeout_ms == RING_BUFFER_INFINITE_TIMEOUT) {
    result = pthread_cond_wait(cond, mutex);
  } else if (timeout_ms == RING_BUFFER_NO_TIMEOUT) {
    return STATE_CODE_TIME_OUT;
  } else {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
      ts.tv_sec += ts.tv_nsec / 1000000000;
      ts.tv_nsec %= 1000000000;
    }
    
    result = pthread_cond_timedwait(cond, mutex, &ts);
  }
  
  return (result == 0) ? STATE_CODE_NO_ERROR : STATE_CODE_TIME_OUT;
}

static void signal_condition(pthread_cond_t* cond) {
  pthread_cond_signal(cond);
}
#endif

/* 线程安全辅助函数 */
E_StateCode ring_buffer_lock(ring_buffer_t* ring_buffer) {
  return_value_if_fail(ring_buffer != NULL, STATE_CODE_INVALID_PARAM);
  
#ifdef WIN32
  EnterCriticalSection(&ring_buffer->mutex);
#else
  pthread_mutex_lock(&ring_buffer->mutex);
#endif
  
  return STATE_CODE_NO_ERROR;
}

E_StateCode ring_buffer_unlock(ring_buffer_t* ring_buffer) {
  return_value_if_fail(ring_buffer != NULL, STATE_CODE_INVALID_PARAM);
  
#ifdef WIN32
  LeaveCriticalSection(&ring_buffer->mutex);
#else
  pthread_mutex_unlock(&ring_buffer->mutex);
#endif
  
  return STATE_CODE_NO_ERROR;
}

E_StateCode ring_buffer_wait_for_read(ring_buffer_t* ring_buffer, UINT32 timeout_ms) {
  return_value_if_fail(ring_buffer != NULL, STATE_CODE_INVALID_PARAM);
  
  if (ring_buffer_is_empty(ring_buffer)) {
    return wait_condition(&ring_buffer->read_cond, &ring_buffer->mutex, timeout_ms);
  }
  
  return STATE_CODE_NO_ERROR;
}

E_StateCode ring_buffer_wait_for_write(ring_buffer_t* ring_buffer, UINT32 timeout_ms) {
  return_value_if_fail(ring_buffer != NULL, STATE_CODE_INVALID_PARAM);
  
  if (ring_buffer_is_full(ring_buffer)) {
    return wait_condition(&ring_buffer->write_cond, &ring_buffer->mutex, timeout_ms);
  }
  
  return STATE_CODE_NO_ERROR;
}

E_StateCode ring_buffer_signal_read(ring_buffer_t* ring_buffer) {
  return_value_if_fail(ring_buffer != NULL, STATE_CODE_INVALID_PARAM);
  
  signal_condition(&ring_buffer->read_cond);
  return STATE_CODE_NO_ERROR;
}

E_StateCode ring_buffer_signal_write(ring_buffer_t* ring_buffer) {
  return_value_if_fail(ring_buffer != NULL, STATE_CODE_INVALID_PARAM);
  
  signal_condition(&ring_buffer->write_cond);
  return STATE_CODE_NO_ERROR;
}

ring_buffer_t* ring_buffer_create(UINT32 init_capacity, UINT32 max_capacity) {
  ring_buffer_t* ring_buffer = NULL;
  return_value_if_fail(init_capacity >= 32, NULL);

  ring_buffer = (ring_buffer_t*)malloc(sizeof(ring_buffer_t));
  return_value_if_fail(ring_buffer != NULL, NULL);

  ring_buffer->data = (uint8_t*)malloc(init_capacity);
  if (ring_buffer->data != NULL) {
    ring_buffer->capacity = init_capacity;
    memset(ring_buffer->data, 0x00, init_capacity);
    ring_buffer->max_capacity = tk_max(init_capacity, max_capacity);
    ring_buffer->r = 0;
    ring_buffer->w = 0;
    ring_buffer->full = FALSE;
    
    /* 初始化线程同步对象 */
#ifdef WIN32
    InitializeCriticalSection(&ring_buffer->mutex);
    ring_buffer->read_cond = CreateEvent(NULL, FALSE, FALSE, NULL);
    ring_buffer->write_cond = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (ring_buffer->read_cond == NULL || ring_buffer->write_cond == NULL) {
      /* 创建失败，清理资源 */
      if (ring_buffer->read_cond) CloseHandle(ring_buffer->read_cond);
      if (ring_buffer->write_cond) CloseHandle(ring_buffer->write_cond);
      DeleteCriticalSection(&ring_buffer->mutex);
      free(ring_buffer->data);
      free(ring_buffer);
      return NULL;
    }
#else
    if (pthread_mutex_init(&ring_buffer->mutex, NULL) != 0) {
      free(ring_buffer->data);
      free(ring_buffer);
      return NULL;
    }
    if (pthread_cond_init(&ring_buffer->read_cond, NULL) != 0) {
      pthread_mutex_destroy(&ring_buffer->mutex);
      free(ring_buffer->data);
      free(ring_buffer);
      return NULL;
    }
    if (pthread_cond_init(&ring_buffer->write_cond, NULL) != 0) {
      pthread_cond_destroy(&ring_buffer->read_cond);
      pthread_mutex_destroy(&ring_buffer->mutex);
      free(ring_buffer->data);
      free(ring_buffer);
      return NULL;
    }
#endif
    
  } else {
    free(ring_buffer);
    ring_buffer = NULL;
  }

  return ring_buffer;
}

E_StateCode ring_buffer_destroy(ring_buffer_t* ring_buffer) {
  return_value_if_fail(ring_buffer != NULL, STATE_CODE_INVALID_PARAM);

  /* 销毁线程同步对象 */
#ifdef WIN32
  DeleteCriticalSection(&ring_buffer->mutex);
  if (ring_buffer->read_cond) CloseHandle(ring_buffer->read_cond);
  if (ring_buffer->write_cond) CloseHandle(ring_buffer->write_cond);
#else
  pthread_mutex_destroy(&ring_buffer->mutex);
  pthread_cond_destroy(&ring_buffer->read_cond);
  pthread_cond_destroy(&ring_buffer->write_cond);
#endif

  TKMEM_FREE(ring_buffer->data);
  memset(ring_buffer, 0x00, sizeof(ring_buffer_t));
  TKMEM_FREE(ring_buffer);

  return STATE_CODE_NO_ERROR;
}

BOOL ring_buffer_is_full(ring_buffer_t* ring_buffer) {
  return_value_if_fail(ring_buffer != NULL, FALSE);
  return ring_buffer->full;
}

BOOL ring_buffer_is_empty(ring_buffer_t* ring_buffer) {
  return_value_if_fail(ring_buffer != NULL, FALSE);
  return (ring_buffer->full == FALSE) && (ring_buffer->r == ring_buffer->w);
}

UINT32 ring_buffer_size(ring_buffer_t* ring_buffer) {
  return_value_if_fail(ring_buffer != NULL, 0);

  if (ring_buffer->full) {
    return ring_buffer->capacity;
  } else {
    if (ring_buffer->w >= ring_buffer->r) {
      return ring_buffer->w - ring_buffer->r;
    } else {
      return ring_buffer->capacity - (ring_buffer->r - ring_buffer->w);
    }
  }
}

UINT32 ring_buffer_free_size(ring_buffer_t* ring_buffer) {
  return_value_if_fail(ring_buffer != NULL, 0);
  return ring_buffer->capacity - ring_buffer_size(ring_buffer);
}

UINT32 ring_buffer_capacity(ring_buffer_t* ring_buffer) {
  return_value_if_fail(ring_buffer != NULL, 0);
  return ring_buffer->capacity;
}

E_StateCode ring_buffer_reset(ring_buffer_t* ring_buffer) {
  return_value_if_fail(ring_buffer != NULL, STATE_CODE_INVALID_PARAM);

  ring_buffer_lock(ring_buffer);
  ring_buffer->full = FALSE;
  ring_buffer->r = ring_buffer->w;
  memset(ring_buffer->data, 0x00, ring_buffer->capacity);
  ring_buffer_signal_write(ring_buffer);
  ring_buffer_unlock(ring_buffer);

  return STATE_CODE_NO_ERROR;
}

E_StateCode ring_buffer_set_read_cursor(ring_buffer_t* ring_buffer, UINT32 r) {
  return_value_if_fail(ring_buffer != NULL, STATE_CODE_INVALID_PARAM);

  ring_buffer_lock(ring_buffer);
  ring_buffer->r = r % ring_buffer->capacity;
  ring_buffer->full = FALSE;
  ring_buffer_signal_write(ring_buffer);
  ring_buffer_unlock(ring_buffer);

  return STATE_CODE_NO_ERROR;
}

E_StateCode ring_buffer_set_read_cursor_delta(ring_buffer_t* ring_buffer, UINT32 r_delta) {
  return_value_if_fail(ring_buffer != NULL, STATE_CODE_INVALID_PARAM);
  /* 在锁内读取 r，然后传给 set_read_cursor（它内部也会加锁，使用 r % capacity） */
  ring_buffer_lock(ring_buffer);
  UINT32 new_r = ring_buffer->r + r_delta;
  ring_buffer_unlock(ring_buffer);
  return ring_buffer_set_read_cursor(ring_buffer, new_r);
}

E_StateCode ring_buffer_set_write_cursor(ring_buffer_t* ring_buffer, UINT32 w) {
  return_value_if_fail(ring_buffer != NULL, STATE_CODE_INVALID_PARAM);

  ring_buffer_lock(ring_buffer);
  ring_buffer->w = w % ring_buffer->capacity;
  if (ring_buffer->r == ring_buffer->w) {
    ring_buffer->full = TRUE;
  }
  ring_buffer_signal_read(ring_buffer);
  ring_buffer_unlock(ring_buffer);

  return STATE_CODE_NO_ERROR;
}

E_StateCode ring_buffer_set_write_cursor_delta(ring_buffer_t* ring_buffer, UINT32 w_delta) {
  return_value_if_fail(ring_buffer != NULL, STATE_CODE_INVALID_PARAM);
  /* 在锁内读取 w，然后传给 set_write_cursor */
  ring_buffer_lock(ring_buffer);
  UINT32 new_w = ring_buffer->w + w_delta;
  ring_buffer_unlock(ring_buffer);
  return ring_buffer_set_write_cursor(ring_buffer, new_w);
}

/* 内部读写函数（无锁，需要在锁内调用） */
static UINT32 ring_buffer_read_internal(ring_buffer_t* ring_buffer, void* buff, UINT32 size) {
  if (size == 0) {
    return 0;
  }

  if (ring_buffer->r != ring_buffer->w || ring_buffer->full) {
    UINT32 ret = 0;
    UINT32 rsize = 0;
    uint8_t* d = (uint8_t*)buff;
    uint8_t* s = ring_buffer->data + ring_buffer->r;

    ring_buffer->full = FALSE;
    if (ring_buffer->r < ring_buffer->w) {
      rsize = ring_buffer->w - ring_buffer->r;
      rsize = tk_min(rsize, size);

      ret = rsize;
      memcpy(d, s, rsize);
      ring_buffer->r = (ring_buffer->r + rsize) % ring_buffer->capacity;
    } else {
      rsize = ring_buffer->capacity - ring_buffer->r;
      rsize = tk_min(rsize, size);

      ret = rsize;
      memcpy(d, s, rsize);
      ring_buffer->r = (ring_buffer->r + rsize) % ring_buffer->capacity;

      if (rsize < size) {
        size -= rsize;
        d += rsize;
        s = ring_buffer->data;

        rsize = tk_min(ring_buffer->w, size);
        if (rsize > 0) {
          memcpy(d, s, rsize);
          ret += rsize;
          ring_buffer->r = (ring_buffer->r + rsize) % ring_buffer->capacity;
        }
      }
    }

    return ret;
  }

  return 0;
}

static UINT32 ring_buffer_write_internal(ring_buffer_t* ring_buffer, const void* buff, UINT32 size) {
  if (size == 0) {
    return 0;
  }

  if (ring_buffer->r != ring_buffer->w || !ring_buffer->full) {
    UINT32 ret = 0;
    UINT32 rsize = 0;
    uint8_t* s = (uint8_t*)buff;
    uint8_t* d = ring_buffer->data + ring_buffer->w;

    if (ring_buffer->w < ring_buffer->r) {
      rsize = ring_buffer->r - ring_buffer->w;
      rsize = tk_min(rsize, size);

      ret = rsize;
      memcpy(d, s, rsize);
      ring_buffer->w = (ring_buffer->w + rsize) % ring_buffer->capacity;
    } else {
      rsize = ring_buffer->capacity - ring_buffer->w;
      rsize = tk_min(rsize, size);

      ret = rsize;
      memcpy(d, s, rsize);
      ring_buffer->w = (ring_buffer->w + rsize) % ring_buffer->capacity;

      if (rsize < size) {
        size -= rsize;
        s += rsize;
        d = ring_buffer->data;

        rsize = tk_min(ring_buffer->r, size);
        if (rsize > 0) {
          memcpy(d, s, rsize);
          ret += rsize;
          ring_buffer->w = (ring_buffer->w + rsize) % ring_buffer->capacity;
        }
      }
    }

    return ret;
  }

  return 0;
}

/* 带超时的读写操作 */
UINT32 ring_buffer_read_timeout(ring_buffer_t* ring_buffer, void* buff, UINT32 size, UINT32 timeout_ms) {
  UINT32 ret = 0;
  return_value_if_fail(ring_buffer != NULL && buff != NULL, 0);

  ring_buffer_lock(ring_buffer);
  
  if (ring_buffer_wait_for_read(ring_buffer, timeout_ms) != STATE_CODE_NO_ERROR) {
    ring_buffer_unlock(ring_buffer);
    return 0;
  }
  
  ret = ring_buffer_read_internal(ring_buffer, buff, size);
  ring_buffer_signal_write(ring_buffer);
  ring_buffer_unlock(ring_buffer);

  return ret;
}

UINT32 ring_buffer_peek_timeout(ring_buffer_t* ring_buffer, void* buff, UINT32 size, UINT32 timeout_ms) {
  UINT32 ret = 0;
  return_value_if_fail(ring_buffer != NULL && buff != NULL, 0);

  ring_buffer_lock(ring_buffer);
  
  if (ring_buffer_wait_for_read(ring_buffer, timeout_ms) != STATE_CODE_NO_ERROR) {
    ring_buffer_unlock(ring_buffer);
    return 0;
  }
  
  /* 保存原始读取位置 */
  UINT32 saved_r = ring_buffer->r;
  BOOL saved_full = ring_buffer->full;
  
  ret = ring_buffer_read_internal(ring_buffer, buff, size);
  
  /* 恢复原始读取位置 */
  ring_buffer->r = saved_r;
  ring_buffer->full = saved_full;
  
  ring_buffer_unlock(ring_buffer);

  return ret;
}

UINT32 ring_buffer_write_timeout(ring_buffer_t* ring_buffer, const void* buff, UINT32 size, UINT32 timeout_ms) {
  UINT32 ret = 0;
  return_value_if_fail(ring_buffer != NULL && buff != NULL, 0);

  ring_buffer_lock(ring_buffer);
  
  if (ring_buffer_wait_for_write(ring_buffer, timeout_ms) != STATE_CODE_NO_ERROR) {
    ring_buffer_unlock(ring_buffer);
    return 0;
  }
  
  ret = ring_buffer_write_internal(ring_buffer, buff, size);
  ring_buffer_signal_read(ring_buffer);
  ring_buffer_unlock(ring_buffer);

  return ret;
}

E_StateCode ring_buffer_read_len_timeout(ring_buffer_t* ring_buffer, void* buff, UINT32 size, UINT32 timeout_ms) {
  return_value_if_fail(ring_buffer != NULL && buff != NULL, STATE_CODE_INVALID_PARAM);

  ring_buffer_lock(ring_buffer);
  
  if (ring_buffer_wait_for_read(ring_buffer, timeout_ms) != STATE_CODE_NO_ERROR) {
    ring_buffer_unlock(ring_buffer);
    return STATE_CODE_TIME_OUT;
  }
  
  if (ring_buffer_size(ring_buffer) >= size) {
    UINT32 ret = ring_buffer_read_internal(ring_buffer, buff, size);
    ring_buffer_signal_write(ring_buffer);
    ring_buffer_unlock(ring_buffer);
    return (ret == size) ? STATE_CODE_NO_ERROR : STATE_CODE_WAIT_MORE_DATA;
  } else {
    ring_buffer_unlock(ring_buffer);
    return STATE_CODE_WAIT_MORE_DATA;
  }
}

E_StateCode ring_buffer_write_len_timeout(ring_buffer_t* ring_buffer, const void* buff, UINT32 size, UINT32 timeout_ms) {
  return_value_if_fail(ring_buffer != NULL && buff != NULL, STATE_CODE_INVALID_PARAM);

  ring_buffer_lock(ring_buffer);
  
  if (ring_buffer_wait_for_write(ring_buffer, timeout_ms) != STATE_CODE_NO_ERROR) {
    ring_buffer_unlock(ring_buffer);
    return STATE_CODE_TIME_OUT;
  }
  
  if (ring_buffer_ensure_write_space(ring_buffer, size) == STATE_CODE_NO_ERROR) {
    UINT32 ret = ring_buffer_write_internal(ring_buffer, buff, size);
    ring_buffer_signal_read(ring_buffer);
    ring_buffer_unlock(ring_buffer);
    return (ret == size) ? STATE_CODE_NO_ERROR : STATE_CODE_INVALID_PARAM;
  } else {
    ring_buffer_unlock(ring_buffer);
    return STATE_CODE_INVALID_PARAM;
  }
}

E_StateCode ring_buffer_skip_timeout(ring_buffer_t* ring_buffer, UINT32 size, UINT32 timeout_ms) {
  return_value_if_fail(ring_buffer != NULL, STATE_CODE_INVALID_PARAM);

  ring_buffer_lock(ring_buffer);
  
  if (ring_buffer_wait_for_read(ring_buffer, timeout_ms) != STATE_CODE_NO_ERROR) {
    ring_buffer_unlock(ring_buffer);
    return STATE_CODE_TIME_OUT;
  }
  
  if (ring_buffer_size(ring_buffer) >= size && size > 0) {
    ring_buffer->full = FALSE;
    ring_buffer->r = (ring_buffer->r + size) % ring_buffer->capacity;
    ring_buffer_signal_write(ring_buffer);
    ring_buffer_unlock(ring_buffer);
    return STATE_CODE_NO_ERROR;
  } else {
    ring_buffer_unlock(ring_buffer);
    return STATE_CODE_WAIT_MORE_DATA;
  }
}

/* 兼容性接口 */
UINT32 ring_buffer_read(ring_buffer_t* ring_buffer, void* buff, UINT32 size) {
  return ring_buffer_read_timeout(ring_buffer, buff, size, RING_BUFFER_NO_TIMEOUT);
}

UINT32 ring_buffer_peek(ring_buffer_t* ring_buffer, void* buff, UINT32 size) {
  return ring_buffer_peek_timeout(ring_buffer, buff, size, RING_BUFFER_NO_TIMEOUT);
}

UINT32 ring_buffer_write(ring_buffer_t* ring_buffer, const void* buff, UINT32 size) {
  return ring_buffer_write_timeout(ring_buffer, buff, size, RING_BUFFER_NO_TIMEOUT);
}

E_StateCode ring_buffer_read_len(ring_buffer_t* ring_buffer, void* buff, UINT32 size) {
  return ring_buffer_read_len_timeout(ring_buffer, buff, size, RING_BUFFER_NO_TIMEOUT);
}

E_StateCode ring_buffer_write_len(ring_buffer_t* ring_buffer, const void* buff, UINT32 size) {
  return ring_buffer_write_len_timeout(ring_buffer, buff, size, RING_BUFFER_NO_TIMEOUT);
}

E_StateCode ring_buffer_skip(ring_buffer_t* ring_buffer, UINT32 size) {
  return ring_buffer_skip_timeout(ring_buffer, size, RING_BUFFER_NO_TIMEOUT);
}

E_StateCode ring_buffer_ensure_write_space(ring_buffer_t* ring_buffer, UINT32 size) {
  UINT32 free_size = ring_buffer_free_size(ring_buffer);
  if (free_size >= size) {
    return STATE_CODE_NO_ERROR;
  } else if (ring_buffer->capacity == ring_buffer->max_capacity) {
    return STATE_CODE_OBJECT_EXISTED;
  } else {
    uint8_t* data = NULL;
    UINT32 old_size = ring_buffer_size(ring_buffer);
    UINT32 capacity = ring_buffer->capacity + (size - free_size);
    return_value_if_fail(capacity <= ring_buffer->max_capacity, STATE_CODE_INVALID_PARAM);

    data = (uint8_t*)TKMEM_ALLOC(capacity);
    return_value_if_fail(data != NULL, STATE_CODE_ALLOCATION_FAILURE);
    
    /* 读取现有数据 */
    UINT32 read_ret = ring_buffer_read_internal(ring_buffer, data, old_size);
    if (read_ret != old_size) {
      TKMEM_FREE(data);
      return STATE_CODE_INVALID_PARAM;
    }

    TKMEM_FREE(ring_buffer->data);
    ring_buffer->r = 0;
    ring_buffer->w = old_size;
    ring_buffer->data = data;
    ring_buffer->capacity = capacity;

    return STATE_CODE_NO_ERROR;
  }
}