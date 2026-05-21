/**
 * @file JsonParsePriv.h
 * @brief JSON 解析私有字段映射表 -- 各结构体与 JSON 字段的绑定定义
 *
 * @details
 * 定义所有媒体相关结构体（音频编码、视频编码、TS复用、转码参数等）
 * 对应的 CJsonStructFieldDef 字段映射表。仅供 JsonParse 内部使用。
 *
 * @see media.h（依赖 T_VEncInfo, T_AEncInfo, T_TsMuxInfo 等）
 * @see JsonParse.h, cjson_extension.h（依赖）
 */

#ifndef JSONPARSEPPPRIV_H
#define JSONPARSEPPPRIV_H

#ifdef		__cplusplus
extern		"C"
{
#endif

/**************************************************************************
 *                         头文件引用                                     *
 **************************************************************************/
#include "media.h"
#include "JsonParse.h"
#include "cjson_extension.h"

/**************************************************************************
 *                        常量定义                                   *
 **************************************************************************/


/**************************************************************************
 *                        宏函数定义                                 *
 **************************************************************************/

/**************************************************************************
 *                         数据类型                                    *
 **************************************************************************/

/**************************************************************************
 *                        全局函数                                *
 **************************************************************************/
  static CJsonStructFieldDef g_stAudioParamKey[] = {
    CJSON_FIELD("bEnable", CJSON_TYPE_INT, T_AEncInfo, bEnable),
    CJSON_STRING_FIELD("strCodec", T_AEncInfo, strCodec),
    CJSON_FIELD("uiSamplerate", CJSON_TYPE_INT, T_AEncInfo, uiSamplerate),
    CJSON_FIELD("uiBitRate", CJSON_TYPE_INT, T_AEncInfo, uiBitRate),
    CJSON_FIELD("uiDelay", CJSON_TYPE_INT, T_AEncInfo, uiDelay),
    CJSON_STRING_FIELD("strQuality", T_AEncInfo, strQuality),
    CJSON_FIELD_END
  };

  static CJsonStructFieldDef g_stVideoParamKey[] = {
    CJSON_FIELD("bEnable", CJSON_TYPE_INT, T_VEncInfo, bEnable),
    CJSON_STRING_FIELD("strCodec", T_VEncInfo, strCodec),
    CJSON_STRING_FIELD("strProfile", T_VEncInfo, strProfile),
    CJSON_STRING_FIELD("strResolution", T_VEncInfo, strResolution),
    CJSON_FIELD("uiFrameRate", CJSON_TYPE_INT, T_VEncInfo, uiFrameRate),
    CJSON_FIELD("uiBitRate", CJSON_TYPE_INT, T_VEncInfo, uiBitRate),
    CJSON_FIELD("uiGops", CJSON_TYPE_INT, T_VEncInfo, uiGops),
    CJSON_STRING_FIELD("strGopMode", T_VEncInfo, strGopMode),
    CJSON_FIELD("uiQpMin", CJSON_TYPE_INT, T_VEncInfo, uiQpMin),
    CJSON_FIELD("uiQpMax", CJSON_TYPE_INT, T_VEncInfo, uiQpMax),
    CJSON_FIELD("uiRcType", CJSON_TYPE_INT, T_VEncInfo, uiRcType),
    CJSON_FIELD_END
  };

  static CJsonStructFieldDef g_stMuxParamKey[] = {
    CJSON_FIELD("ts_id", CJSON_TYPE_INT, T_TsMuxInfo, ts_id),
    CJSON_FIELD("pmt_pid", CJSON_TYPE_INT, T_TsMuxInfo, pmt_pid),
    CJSON_FIELD("pcr_pid", CJSON_TYPE_INT, T_TsMuxInfo, pcr_pid),
    CJSON_FIELD("video_pid", CJSON_TYPE_INT, T_TsMuxInfo, video_pid),
    CJSON_FIELD("video_enable", CJSON_TYPE_INT, T_TsMuxInfo, video_enable),
    CJSON_FIELD("audio_pid", CJSON_TYPE_INT, T_TsMuxInfo, audio_pid),
    CJSON_FIELD("audio_enable", CJSON_TYPE_INT, T_TsMuxInfo, audio_enable),
    CJSON_FIELD("ts_bitrate", CJSON_TYPE_INT, T_TsMuxInfo, ts_bitrate),
    CJSON_FIELD("pat_interval", CJSON_TYPE_INT, T_TsMuxInfo, pat_interval),
    CJSON_FIELD("pmt_interval", CJSON_TYPE_INT, T_TsMuxInfo, pmt_interval),
    CJSON_FIELD("pcr_interval", CJSON_TYPE_INT, T_TsMuxInfo, pcr_interval),
    CJSON_FIELD("sdt_interval", CJSON_TYPE_INT, T_TsMuxInfo, sdt_interval),
    CJSON_FIELD_END
  };

  static CJsonStructFieldDef g_stMuxServiceParamKey[] = {
    CJSON_FIELD("service_id", CJSON_TYPE_INT, T_TsMuxServiceInfo, service_id),
    CJSON_STRING_FIELD("ServiceName", T_TsMuxServiceInfo, ServiceName),
    CJSON_STRING_FIELD("ProviderName", T_TsMuxServiceInfo, ProviderName),
    CJSON_FIELD("ServiceType", CJSON_TYPE_INT, T_TsMuxServiceInfo, ServiceType),
    CJSON_FIELD_END
  };

  static CJsonStructFieldDef g_stEncParamKey[] = {
    CJSON_STRUCT_FIELD("VEncInfo", T_EncInfo, VEncInfo, g_stVideoParamKey),
    CJSON_STRUCT_FIELD("AEncInfo", T_EncInfo, AEncInfo, g_stAudioParamKey),
    CJSON_STRUCT_FIELD("TsMuxInfo", T_EncInfo, TsMuxInfo, g_stMuxParamKey),
    CJSON_STRUCT_FIELD("TsMuxServiceInfo", T_EncInfo, TsMuxServiceInfo, g_stMuxServiceParamKey),
    CJSON_FIELD_END
  };

  static CJsonStructFieldDef g_stProgInfoKey[] = {
    CJSON_FIELD("channel_flag", CJSON_TYPE_INT, T_prog_info_t, channel_flag),
    CJSON_FIELD("vid_type", CJSON_TYPE_INT, T_prog_info_t, vid_type),
    CJSON_FIELD("vid_pid", CJSON_TYPE_INT, T_prog_info_t, vid_pid),
    CJSON_FIELD("aud_type", CJSON_TYPE_INT, T_prog_info_t, aud_type),
    CJSON_FIELD("aud_pid", CJSON_TYPE_INT, T_prog_info_t, aud_pid),
    CJSON_FIELD("pcr_pid", CJSON_TYPE_INT, T_prog_info_t, pcr_pid),
    CJSON_FIELD_END
  };

  static CJsonStructFieldDef g_stTranscodeDecodeParamKey[] = {
    CJSON_FIELD("sync", CJSON_TYPE_INT, T_TranscodeDecodeParam, sync),
    CJSON_FIELD("fail", CJSON_TYPE_INT, T_TranscodeDecodeParam, fail),
    CJSON_FIELD("ach1", CJSON_TYPE_INT, T_TranscodeDecodeParam, ach1),
    CJSON_FIELD("ach2", CJSON_TYPE_INT, T_TranscodeDecodeParam, ach2),
    CJSON_FIELD("av1", CJSON_TYPE_INT, T_TranscodeDecodeParam, av1),
    CJSON_FIELD("av2", CJSON_TYPE_INT, T_TranscodeDecodeParam, av2),
    CJSON_FIELD("adelay1", CJSON_TYPE_INT, T_TranscodeDecodeParam, adelay1),
    CJSON_FIELD("adelay2", CJSON_TYPE_INT, T_TranscodeDecodeParam, adelay2),
    CJSON_FIELD("fmt", CJSON_TYPE_INT, T_TranscodeDecodeParam, fmt),
    CJSON_FIELD("sdi", CJSON_TYPE_INT, T_TranscodeDecodeParam, sdi),
    CJSON_FIELD("cs", CJSON_TYPE_INT, T_TranscodeDecodeParam, cs),
    CJSON_FIELD("hdr", CJSON_TYPE_INT, T_TranscodeDecodeParam, hdr),
    CJSON_FIELD("br", CJSON_TYPE_INT, T_TranscodeDecodeParam, br),
    CJSON_FIELD("cr", CJSON_TYPE_INT, T_TranscodeDecodeParam, cr),
    CJSON_FIELD("hu", CJSON_TYPE_INT, T_TranscodeDecodeParam, hu),
    CJSON_FIELD("sa", CJSON_TYPE_INT, T_TranscodeDecodeParam, sa),
    CJSON_FIELD("errfmfiltervalue", CJSON_TYPE_INT, T_TranscodeDecodeParam, errfmfiltervalue),
    CJSON_FIELD_END
  };

  static CJsonStructFieldDef g_stTranscodeDecodePidKey[] = {
    CJSON_FIELD("vp", CJSON_TYPE_INT, T_TranscodeDecodePid, vp),
    CJSON_FIELD("vt", CJSON_TYPE_INT, T_TranscodeDecodePid, vt),
    CJSON_FIELD("pp", CJSON_TYPE_INT, T_TranscodeDecodePid, pp),
    CJSON_FIELD("ap1", CJSON_TYPE_INT, T_TranscodeDecodePid, ap1),
    CJSON_FIELD("ap2", CJSON_TYPE_INT, T_TranscodeDecodePid, ap2),
    CJSON_FIELD("at1", CJSON_TYPE_INT, T_TranscodeDecodePid, at1),
    CJSON_FIELD("at2", CJSON_TYPE_INT, T_TranscodeDecodePid, at2),
    CJSON_FIELD_END
  };


#ifdef		__cplusplus
}
#endif

#endif // JSONPARSEPPPRIV_H
