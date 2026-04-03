#ifndef MEDIA_H_
#define MEDIA_H_
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "defs.h"

EXTERN_C_BLOCK

#define MAX_WIDTH 3840
#define MAX_HEIGHT 2160

#define TS_READ_SIZE (188 * 100)
#define TS_READ_TIMEOUT 100
#define WAIT_BUF_TIMEOUT 1000

#define MAX_AUDIO_DEV 2 // 声卡节点数量

#define MAX_AUDIO_TRACK 2 // 音轨数量

#define MAX_AUDIO_OUTCHANEL_NUM 2 // 网页配置数量

#define MAX_MO_OUTCHANEL_NUM 2 // 数量

typedef E_StateCode (*MsgProcFxn)(void *pPrivate, void *ptMsg);

typedef struct _T_frame
{
    AVFrame *frame; // 真内存
    void *pUserData;
} T_frame;

typedef struct _T_packet
{
    AVPacket *packet; // 真内存
    void *pUserData;
} T_packet;

typedef struct _T_video_params
{
    INT32 width;
    INT32 height;
    enum AVPixelFormat pix_fmt;
    AVRational time_base;
    INT32 fps;
    INT32 gop_size;
    INT32 bitrate;

    String64 codec_name;
} T_video_params;

typedef struct _T_audio_params
{
    INT32 sample_rate;
    INT32 channels;
    enum AVSampleFormat sample_fmt;
    INT32 bitrate;
    INT32 block_align;
    INT32 frame_size;

    String64 codec_name;
    String64 profiles_name;
} T_audio_params;

typedef enum
{
    FFMPEG_SOFT_CODEC,
    FFMPEG_HAL_CODEC
} E_CodecType; // 解码类型

typedef struct _T_MsgV2
{
    String128 strMethod;
    void *pcBody;
    UINT32 uiCallId;
} T_MsgV2;

typedef struct _T_MsgProcV2
{
    INT8 *pcMsg;
    MsgProcFxn pfMsgProcFxn;
} T_MsgProcV2;

typedef struct _T_DemuxParam
{
    uint16_t vpid;
    uint16_t apid0;
    uint16_t apid1;

    uint8_t *udp_addr;
    uint8_t *file_path;
} T_DemuxParam;

typedef struct _T_AudioConfigureParam
{
    uint8_t audio;
    uint8_t audio_mode;

    uint8_t gain_type;
    uint8_t audio_gain;
} T_AudioConfigureParam;

typedef struct _T_VideoConfigureParam
{
    uint8_t fmt;
    uint8_t hdr;
    uint8_t stop_mode;
    uint8_t aspect_ratio;
    uint8_t aspect_convert;

    uint16_t brightness;
    uint16_t contrast;
    uint16_t hue;
    uint16_t saturation;
} T_VideoConfigureParam;

typedef struct _T_SyncConfigureParam
{
    uint8_t mode;
    uint16_t av_delay;
} T_SyncConfigureParam;

typedef struct _T_Msg
{
    UINT32 uiCommand;
    String32 strMsgId;
    // INT8*	  	pcMsg;
    void *pcBody;
    UINT32 uiCallId;
} T_Msg;

/* 编码类型*/
#define CODEC_NAME_H264 "H264"
#define CODEC_NAME_H265 "H265"

#define CODEC_NAME_AAC_LC "AAC_LC"
#define CODEC_NAME_AAC_HE "AAC_HE"
#define CODEC_NAME_AAC_HE_PLUS "AAC_HE_PLUS"

#define CODEC_NAME_G711A "G711A"
#define CODEC_NAME_G711U "G711U"
#define CODEC_NAME_G722 "G722"

#define GOP_MODE_NORMALP "IP"
#define GOP_MODE_DUALP "IPP"
#define GOP_MODE_SMARTP "IIP"

#define PROFILE_MODE_HIGH "High"
#define PROFILE_MODE_MAIN "Main"

#define PROFILE_AUDIO_MODE_LOW "Low"
#define PROFILE_AUDIO_MODE_HIGH "High"
#define PROFILE_AUDIO_MODE_EXCELLENT "Excellent"

typedef struct
{
    UINT32 channel_flag;
    UINT32 vid_type;
    UINT32 vid_pid;
    UINT32 aud_type;
    UINT32 aud_pid;
    UINT32 pcr_pid;
} T_prog_info_t;

typedef struct
{
    BOOL bEnable;
    String16 strCodec;      //"H264"
    String16 strProfile;    //"High"
    String32 strResolution; //"1920x1080"
    UINT32 uiFrameRate;
    UINT32 uiBitRate;
    UINT32 uiGops;
    String16 strGopMode; //"IP"
    UINT32 uiQpMin;
    UINT32 uiQpMax;
    UINT32 uiRcType;
} T_VEncInfo;

typedef struct
{
    BOOL bEnable;
    String16 strCodec;   //"AAC_LC"
    UINT32 uiSamplerate; // 采样率
    UINT32 uiBitRate;    // 码率
    UINT32 uiDelay;
    String32 strQuality; //"High"
} T_AEncInfo;

typedef struct
{
    UINT32 ts_id;
    UINT32 pmt_pid;
    UINT32 pcr_pid;

    UINT32 video_pid;
    UINT32 video_enable;

    UINT32 audio_pid;
    UINT32 audio_enable;

    UINT32 ts_bitrate;
    UINT32 pat_interval; // ms
    UINT32 pmt_interval; // ms
    UINT32 pcr_interval; // ms
    UINT32 sdt_interval; // ms
} T_TsMuxInfo;

typedef struct
{
    UINT32 service_id;
    String64 ServiceName; // SDT表服务名
    String64 ProviderName;
    UINT8 ServiceType;
} T_TsMuxServiceInfo;

typedef struct
{
    T_VEncInfo VEncInfo;
    T_AEncInfo AEncInfo;
    T_TsMuxInfo TsMuxInfo;
    T_TsMuxServiceInfo TsMuxServiceInfo;
} T_EncInfo;

typedef struct
{
    unsigned char *buf;
    long long timestamp;
    int len; // this node valid size
    void *ptSrc;
    BOOL BNeedFree;
} T_StreamInfo;

typedef struct
{
    UINT8 *PPS;
    UINT32 PPSSize;
    UINT8 *SPS;
    UINT32 SPSSize;
    UINT8 *data;
    UINT32 dataSize;
} T_NewStreamInfo;

typedef enum
{
    E_TRANSCODE_RUNNING,
    E_TRANSCODE_STOPPED,
    E_TRANSCODE_RESETING,
} E_TransCodeState; // 转换状态

typedef struct
{
    UINT32 uiWidth;
    UINT32 uiHeight;
    UINT32 uiFrameRate;
    BOOL bInterlaced;

    UINT32 uiProfile;
    UINT32 uiLevel;
    UINT32 uiBitRate;
} T_VideoParam;

typedef struct
{
    UINT32 uiSampleRate;
    UINT32 uiChannels;
    UINT32 uiBitRate;
} T_AudioParam;

typedef struct
{
    UINT8 *pBuf;
    UINT32 uiSize;
    // BOOL bFullFrame;
    INT64 timestamp;
} T_PesPacket;

#define MAX_NALUS_PER_FRAME 16

typedef struct
{
    void *pesInfo[MAX_NALUS_PER_FRAME];
    int count;
} T_PesMuxer;

typedef enum
{
    ENC_CONFIG_AUD = 0x01,
    ENC_CONFIG_VID = 0x02,
    ENC_CONFIG_MUX = 0x04,
    ENC_CONFIG_DECODE_PID = 0x08,
    ENC_CONFIG_DECODE_PARAM = 0x10,

    ENC_CONFIG_MAX = 0x8
} enc_media_config;

typedef struct
{
    UINT32 vp;
    UINT32 vt;
    UINT32 pp;
    UINT32 ap1;
    UINT32 ap2;
    UINT32 at1;
    UINT32 at2;
} T_TranscodeDecodePid; // 适配旧接口

typedef struct
{
    UINT32 sync;
    UINT32 fail;
    UINT32 ach1;
    UINT32 ach2;
    UINT32 av1;
    UINT32 av2;
    UINT32 adelay1;
    UINT32 adelay2;
    UINT32 fmt;
    UINT32 sdi;
    UINT32 cs;
    UINT32 hdr;
    UINT32 br;
    UINT32 cr;
    UINT32 hu;
    UINT32 sa;
    UINT32 errfmfiltervalue;
} T_TranscodeDecodeParam; // 适配旧接口

typedef struct
{
    UINT32 vp;
} T_TranscodeDecodeInfo; // 适配旧接口

typedef struct
{
    UINT32 vp;
    UINT32 vt;
} T_TranscodeEncodeInfo; // 适配旧接口

typedef enum
{
    NO_INPUT,
    TSI_INPUT,
    LAN_INPUT,
    FILE_INPUT,
    URL_INPUT,
} demux_input_type;

typedef struct
{
    UINT32 video_delay;
    UINT32 video_delay_mode;
    UINT32 video_delay_pid;

} video_config;

typedef struct
{
    UINT32 aud_volume; // 0
    UINT32 aud_mode;   // uapi_snd_output_mode
    UINT32 aud_pid;
    UINT32 aud_ac3_drc_mode;

    INT64 video_pts;
    INT64 audio_pts;
} audio_config;

typedef struct
{
    ui_handle avplay; // 播放控制句柄
    UINT32 avplay_count;

    HANDLE hdecode; // 私有解码解复用句柄
    INT8 url[PATH_MAX_SIZE];
} lj_play_ctx;

typedef struct
{
    HANDLE hdecode; // 私有解码解复用句柄
    INT8 url[PATH_MAX_SIZE];
    void *userCtx; // havplay
} lj_play_es_ctx;

typedef struct
{
    /*多个audio_pid配置*/
    UINT32 aud_pid[PROG_MAX_AUDIO];
    uapi_avplay_multi_audio_attr multi_aud_attr;
} multi_audio_ctx;

typedef struct
{
    uapi_avplay_event_type evt;
    uapi_avplay_event_callback_fn callback;
} avplay_cb_ctx;

typedef struct
{
    E_AudioSampleRate uiSampleRate;
    E_AudioSoundMode enSoundmode;
    E_AudioBitWidth enBitwidth; /* audio frame bitwidth */
    UINT32 u32Seq;              /* audio frame seq */
    UINT64 u64TimeStamp;        /* audio frame timestamp,单位:微秒 */
    short int *sample;
    UINT32 uibufferSize;
} T_Audio;

EXTERN_C_BLOCK_END
#endif