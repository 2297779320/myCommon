/**
 * @file defs.h
 * @brief 项目核心定义头文件 -- 类型、宏、状态码、工具宏
 *
 * @details
 * 本文件是项目中被依赖最广泛的头文件，几乎所有非叶子模块都直接或间接依赖它。
 * 主要内容：
 *   1. 系统头文件聚合（stdio, stdlib, pthread, socket 等）
 *   2. 平台 SDK 头文件（soc_errno, securec, uapi_avplay 等）
 *   3. 基本数据类型宏定义（INT8~INT64, UINT8~UINT64, BOOL 等）
 *   4. 状态码枚举 E_StateCode
 *   5. 工具宏（do_func, return_if_fail, SAFESTRCPY, lj_min/lj_max 等）
 *   6. 音频相关枚举（采样率、位宽、声道模式、样本格式）
 *
 * @note BOOL 在本文件中以 #define 宏定义，会覆盖 ctdef.h 中的 typedef。
 *       TRUE/FALSE 同时在 ctdef.h 和本文件中定义，值相同 (1/0)，通过 #ifndef 避免冲突。
 *
 * @see log.h (syserr/syswarn 宏依赖)
 * @see common.h, ctos.h, osal.h, tsk.h, comm_que.h, framework_def.h, media.h,
 *      JsonEx.h, JsonParse.h, debugtrace.h, ring_buffer.h, share_mem_queue.h,
 *      jsonrpc.h, jsonrpcService.h, uart.h（被依赖）
 */

#ifndef DEFS_H
#define DEFS_H
#ifdef		__cplusplus
extern		"C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <features.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <soc_errno.h>
#include <stddef.h>

#include <assert.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>

#include "securec.h"
#include "uapi_avplay.h"
#include "uapi_sound.h"
#include "uapi_win.h"
#include "uapi_demux.h"
#include "adp_uapi_ext.h"

#include "log.h"
#include "LJMediaMsg.h"

#define PATH_MAX_SIZE           512   
#define HDMITX_MAX_NUM          2

#define APP_DECODE_UHD_WIDTH    3840
#define APP_DECODE_UHD_HEIGHT   2160
#define APP_DECODE_FHD_WIDTH    1920
#define APP_DECODE_FHD_HEIGHT   1080
#define APP_DECODE_HD_WIDTH     1280
#define APP_DECODE_HD_HEIGHT    720

#ifdef __cplusplus
    #define EXTERN_C_BLOCK extern "C" {
    #define EXTERN_C_BLOCK_END }
#else
    #define EXTERN_C_BLOCK
    #define EXTERN_C_BLOCK_END
#endif


#ifndef UNUSED
#define UNUSED __attribute__((unused))
#endif

#ifndef VOID
#define VOID void
#endif

#ifndef INT32
#define INT32 int
#endif

#ifndef INT16
#define INT16 short
#endif

#ifndef INT8
#define INT8 char
#endif

#ifndef UINT64
#define UINT64 unsigned long long
#endif

#ifndef INT64
#define INT64 long long
#endif

#ifndef UINT32
#define UINT32 unsigned int
#endif

#ifndef UINT16
#define UINT16 unsigned short
#endif

#ifndef UINT8
#define UINT8 unsigned char
#endif

#ifndef BYTE
#define BYTE unsigned char
#endif

#ifndef LONG
#define LONG long
#endif

#ifndef ULONG
#define ULONG unsigned long
#endif

#ifndef DOUBLE
#define DOUBLE double
#endif

#ifndef FLOAT
#define FLOAT float
#endif

#ifdef BOOL
#undef BOOL
#endif

#define BOOL int

#ifndef SOCKET
#define SOCKET int
#endif

#ifndef STATIC
#define STATIC static
#endif

#ifndef CONST
#define CONST const
#endif

#ifndef WIN32
#ifndef HANDLE
#define HANDLE void *
#endif
#endif

/* NULL is provided by <stddef.h> */

#ifndef LJFrame
#define LJFrame void *
#endif

#ifndef LJPacket
#define LJPacket void *
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE (1)
#endif

#define LJ_SUCCESS          0
#define LJ_FAILURE          (-1)

#define LJ_INVALID_HANDLE   (0xffffffff)

typedef UINT32            ui_handle;

typedef enum
{
    LJ_FALSE = 0,
    LJ_TRUE = 1,
} lj_bool;

typedef INT8 String8[8];
typedef INT8 String16[16];
typedef INT8 String32[32];
typedef INT8 String64[64];
typedef INT8 String128[128];
typedef INT8 String256[256];
typedef INT8 String512[512];

/* 状态码常量定义*/
typedef enum
{
  STATE_CODE_NO_ERROR = 0,
  STATE_CODE_INIT_FAILURE = -1,
  STATE_CODE_INVALID_HANDLE = -2,
  STATE_CODE_UNABLE_TO_OPEN_FILE = -3,
  STATE_CODE_CONFIG_ERROR = -4,
  STATE_CODE_ALLOCATION_FAILURE = -5,
  STATE_CODE_FREE_MEMORY_FAILURE = -6,
  STATE_CODE_SOCKET_CREATE_FAILURE = -7,
  STATE_CODE_SOCKET_CLOSE_FAILURE = -8,
  STATE_CODE_SOCKET_BIND_FAILURE = -9,
  STATE_CODE_FAILED_TO_PROCEED_COMMAND = -10,
  STATE_CODE_INVALID_COMMAND = -11,
  STATE_CODE_INVALID_PARAM = -12,
  STATE_CODE_TIME_OUT = -13,
  STATE_CODE_OBJECT_EXISTED = -14,
  STATE_CODE_OBJECT_NOT_EXIST = -15,
  STATE_CODE_OBJECT_BEYOND = -16, /* 超出范围*/
  STATE_CODE_OBJECT_BUSY = -17,
  STATE_CODE_WAIT_MORE_DATA = -18,
  STATE_CODE_AUTHORIZATION_FAILED = -19,
  STATE_CODE_OBJECT_LOCKED = -20,
  STATE_CODE_USER_NOT_FOUND = -21,
  STATE_CODE_STREAM_NOT_FOUND = -22,
  STATE_CODE_OBJECT_NOT_SUPPORT = -23,

  STATE_CODE_UNDEFINED_ERROR = -999,

  STATE_CODE_ERROR_MAX
} E_StateCode;

#define STATE_OK(eCode) (STATE_CODE_NO_ERROR == (eCode))

#define do_func(func)                                                         \
  do                                                             \
  {                                                              \
        INT32 _ret = func;                                                   \
    if (_ret != LJ_SUCCESS)                                      \
    {                                                            \
            syserr("Do [%s] failed, errorcode = 0x%x\n", #func, _ret);       \
            return _ret;                                                      \
        }                                                                     \
    } while (0)


#define goto_error_if_fail_ex(p, sentence)                                   \
  do                                                                        \
  {                                                                         \
    if (!(p))                                                               \
    {                                                                       \
      syswarn("%s:%d condition(" #p ") failed!\n", __FUNCTION__, __LINE__); \
      sentence;                                                              \
      goto error;                                                            \
    }                                                                        \
  } while (0)

#define break_if_fail(p)                                                     \
  {                                                                          \
    if (!(p))                                                               \
    {                                                                       \
      syswarn("%s:%d condition(" #p ") failed!\n", __FUNCTION__, __LINE__); \
      break;                                                                 \
    }                                                                        \
  }

#define return_if_fail(p)                                                    \
  do                                                                        \
  {                                                                         \
    if (!(p))                                                               \
    {                                                                       \
      syswarn("%s:%d condition(" #p ") failed!\n", __FUNCTION__, __LINE__); \
      return;                                                                \
    }                                                                        \
  } while (0)

#define return_value_if_fail(p, value)                                       \
  do                                                                        \
  {                                                                         \
    if (!(p))                                                               \
    {                                                                       \
      syswarn("%s:%d condition(" #p ") failed!\n", __FUNCTION__, __LINE__); \
      return (value);                                                        \
    }                                                                        \
  } while (0)

#define return_value_if_equal(p, value) \
  do                                    \
  {                                     \
    if ((p) == value)                   \
    {                                   \
      return (value);                   \
    }                                   \
  } while (0)

#define lj_min(a, b) ((a) < (b) ? (a) : (b))
#define lj_abs(a) ((a) < (0) ? (-(a)) : (a))
#define lj_max(a, b) ((a) > (b) ? (a) : (b))

#define LJMEM_ALLOC(size) malloc(size)

#define LJ_SAFE_MALLOC(ptr, size) \
  do                                                            \
  {                                                             \
        (ptr) = malloc((size)); \
    if ((ptr) == NULL)                                          \
    {                                                           \
            syserr("[ERROR] Memory allocation failed  (size: %zu)\n", \
                    (size_t)(size)); \
    }                                                           \
    else                                                        \
    {                                                           \
        memset((ptr), 0, (size));                               \
    }                                                           \
    } while(0)


#define LJMEM_FREE(p) \
  do                   \
  {                    \
    if (p != NULL)     \
    {                  \
      free((void*)p); \
      p = NULL;       \
    }                 \
  } while (0)


#define SAFESTRCPY(dest, src, dest_size) \
  do                                                         \
  {                                                          \
    if ((dest) == NULL || (src) == NULL || (dest_size) == 0) \
    {                                                        \
      if ((dest) != NULL && (dest_size) > 0)                 \
      {                                                      \
                (dest)[0] = '\0'; \
            } \
            break; \
        } \
        size_t __src_len = strlen(src); \
    if (__src_len >= (dest_size))                            \
    {                                                        \
            /* 截断处理 */ \
            memcpy((dest), (src), (dest_size) - 1); \
            (dest)[(dest_size) - 1] = '\0'; \
    }                                                        \
    else                                                     \
    {                                                        \
            strcpy((dest), (src)); \
        } \
    } while(0)

#define LJSAFE_MEMCPY(dest, src, n)                  \
    ({                                              \
        void *result = NULL;                        \
    if ((dest) == NULL || (src) == NULL)                     \
    {                                                        \
            result = NULL;                          \
    }                                                        \
    else if ((n) == 0)                                       \
    {                                                        \
            result = (dest);                        \
    }                                                        \
    else if ((src < dest && (src + (n)) > dest) ||           \
             (dest < src && (dest + (n)) > src))             \
    {                                                        \
            result = NULL;                          \
    }                                                        \
    else                                                     \
    {                                                        \
            unsigned char *d = (unsigned char *)(dest); \
            const unsigned char *s = (const unsigned char *)(src); \
      for (size_t l = 0; l < (n); ++l)                       \
      {                                                      \
                d[l] = s[l];                        \
            }                                       \
            result = (dest);                        \
        }                                           \
        result;                                     \
    })


#define LJ_STR_CASE_CMP(s1, s2) strcasecmp((s1), (s2))
#define LJ_STR_EQUAL_IGNORE_CASE(s1, s2) (LJ_STR_CASE_CMP(s1, s2) == 0)

#define LJ_UNUSED(x) ((x) = (x))

typedef enum tagE_AudioBitWidth
{
    AudioBitWidth_8    = 0,   /* 8bit width */
    AudioBitWidth_16   = 1,   /* 16bit width */
    AudioBitWidth_24   = 2,   /* 24bit width */
    AudioBitWidth_32   = 3,   /* 32bit width */
    AudioBitWidth_Butt,
} E_AudioBitWidth;

typedef enum tagE_AudioSoundMode
{
    AudioSoundMode_Mono     = 0, /* mono */
    AudioSoundMode_Stereo   = 1, /* stereo */
    AudioSoundMode_4        = 4, 
    AudioSoundMode_8        = 8,
    AudioSoundMode_Butt,
} E_AudioSoundMode;

/** @enum 样本格式,用于描述样本的位宽、含义 */
typedef enum tagE_AudioSampleFormat
{
    E_SAMPLE_FMT_NONE = -1,
    E_SAMPLE_FMT_U8,
    E_SAMPLE_FMT_S16,
    E_SAMPLE_FMT_S32,
    E_SAMPLE_FMT_FLT,
    E_SAMPLE_FMT_U8P,
    E_SAMPLE_FMT_S16P,
    E_SAMPLE_FMT_S32P,
    E_SAMPLE_FMT_FLTP,
    E_SAMPLE_FMT_G711A,
    E_SAMPLE_FMT_G711U,
    E_SAMPLE_FMT_LAST = E_SAMPLE_FMT_G711U
} E_AudioSampleFormat;

typedef enum tagE_AudioSampleRate
{
    AudioSampleRate_8000   = 8000,    /* 8kHz sample rate */
    AudioSampleRate_12000  = 12000,   /* 12kHz sample rate */
    AudioSampleRate_11025  = 11025,   /* 11.025kHz sample rate */
    AudioSampleRate_16000  = 16000,   /* 16kHz sample rate */
    AudioSampleRate_22050  = 22050,   /* 22.05kHz sample rate */
    AudioSampleRate_24000  = 24000,   /* 24kHz sample rate */
    AudioSampleRate_32000  = 32000,   /* 32kHz sample rate */
    AudioSampleRate_44100  = 44100,   /* 44.1kHz sample rate */
    AudioSampleRate_48000  = 48000,   /* 48kHz sample rate */
    AudioSampleRate_64000  = 64000,   /* 64kHz sample rate */
    AudioSampleRate_96000  = 96000,   /* 96kHz sample rate */

    AudioSampleRate_Butt,
} E_AudioSampleRate;

#ifdef		__cplusplus
}
#endif

#endif