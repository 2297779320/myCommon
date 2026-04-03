#ifndef _H_JSONPARSEPPPRIV_
#define _H_JSONPARSEPPPRIV_

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
  CJsonStructFieldDef g_stAudioParamKey[] = {
    {"bEnable", CJSON_TYPE_INT, offsetof(T_AEncInfo, bEnable), 0,NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"strCodec", CJSON_TYPE_STRING, offsetof(T_AEncInfo, strCodec), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"uiSamplerate", CJSON_TYPE_INT, offsetof(T_AEncInfo, uiSamplerate), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"uiBitRate", CJSON_TYPE_INT, offsetof(T_AEncInfo, uiBitRate), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"uiDelay", CJSON_TYPE_INT, offsetof(T_AEncInfo, uiDelay), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"strQuality", CJSON_TYPE_STRING, offsetof(T_AEncInfo, strQuality), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {NULL, 0, 0, 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX} // 结束标记
};

  CJsonStructFieldDef g_stVideoParamKey[] = {
    {"bEnable", CJSON_TYPE_INT, offsetof(T_VEncInfo, bEnable), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"strCodec", CJSON_TYPE_STRING, offsetof(T_VEncInfo, strCodec), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"strProfile", CJSON_TYPE_STRING, offsetof(T_VEncInfo, strProfile), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"strResolution", CJSON_TYPE_STRING, offsetof(T_VEncInfo, strResolution), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"uiFrameRate", CJSON_TYPE_INT, offsetof(T_VEncInfo, uiFrameRate), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"uiBitRate", CJSON_TYPE_INT, offsetof(T_VEncInfo, uiBitRate), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"uiGops", CJSON_TYPE_INT, offsetof(T_VEncInfo, uiGops), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"strGopMode", CJSON_TYPE_STRING, offsetof(T_VEncInfo, strGopMode), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"uiQpMin", CJSON_TYPE_INT, offsetof(T_VEncInfo, uiQpMin), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"uiQpMax", CJSON_TYPE_INT, offsetof(T_VEncInfo, uiQpMax), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"uiRcType", CJSON_TYPE_INT, offsetof(T_VEncInfo, uiRcType), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {NULL, 0, 0, 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX} // 结束标记
};

  CJsonStructFieldDef g_stMuxParamKey[] = {
    {"ts_id", CJSON_TYPE_INT, offsetof(T_TsMuxInfo, ts_id), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"pmt_pid", CJSON_TYPE_INT, offsetof(T_TsMuxInfo, pmt_pid), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"pcr_pid", CJSON_TYPE_INT, offsetof(T_TsMuxInfo, pcr_pid), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"video_pid", CJSON_TYPE_INT, offsetof(T_TsMuxInfo, video_pid), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"video_enable", CJSON_TYPE_INT, offsetof(T_TsMuxInfo, video_enable), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"audio_pid", CJSON_TYPE_INT, offsetof(T_TsMuxInfo, audio_pid), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"audio_enable", CJSON_TYPE_INT, offsetof(T_TsMuxInfo, audio_enable), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"ts_bitrate", CJSON_TYPE_INT, offsetof(T_TsMuxInfo, ts_bitrate), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"pat_interval", CJSON_TYPE_INT, offsetof(T_TsMuxInfo, pat_interval), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"pmt_interval", CJSON_TYPE_INT, offsetof(T_TsMuxInfo, pmt_interval), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"pcr_interval", CJSON_TYPE_INT, offsetof(T_TsMuxInfo, pcr_interval), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"sdt_interval", CJSON_TYPE_INT, offsetof(T_TsMuxInfo, sdt_interval), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {NULL, 0, 0, 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX} // 结束标记
};

  CJsonStructFieldDef g_stMuxServiceParamKey[] = {
    {"service_id", CJSON_TYPE_INT, offsetof(T_TsMuxServiceInfo, service_id), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"ServiceName", CJSON_TYPE_STRING, offsetof(T_TsMuxServiceInfo, ServiceName), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"ProviderName", CJSON_TYPE_STRING, offsetof(T_TsMuxServiceInfo, ProviderName), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"ServiceType", CJSON_TYPE_INT, offsetof(T_TsMuxServiceInfo, ServiceType), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {NULL, 0, 0, 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX} // 结束标记
};

  CJsonStructFieldDef g_stEncParamKey[] = {
    {"VEncInfo", CJSON_TYPE_STRUCT, offsetof(T_EncInfo, VEncInfo), sizeof(T_VEncInfo), g_stVideoParamKey, 0, 0, NULL, CJSON_TYPE_MAX},
    {"AEncInfo", CJSON_TYPE_STRUCT, offsetof(T_EncInfo, AEncInfo), sizeof(T_AEncInfo), g_stAudioParamKey, 0, 0, NULL, CJSON_TYPE_MAX},
    {"TsMuxInfo", CJSON_TYPE_STRUCT, offsetof(T_EncInfo, TsMuxInfo), sizeof(T_TsMuxInfo), g_stMuxParamKey, 0, 0, NULL, CJSON_TYPE_MAX},
    {"TsMuxServiceInfo", CJSON_TYPE_STRUCT, offsetof(T_EncInfo, TsMuxServiceInfo), sizeof(T_TsMuxServiceInfo), g_stMuxServiceParamKey, 0, 0, NULL, CJSON_TYPE_MAX},
    {NULL, 0, 0, 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX} // 结束标记
};

  CJsonStructFieldDef g_stProgInfoKey[] = {
    {"channel_flag", CJSON_TYPE_INT, offsetof(T_prog_info_t, channel_flag), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"vid_type", CJSON_TYPE_INT, offsetof(T_prog_info_t, vid_type), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"vid_pid", CJSON_TYPE_INT, offsetof(T_prog_info_t, vid_pid), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"aud_type", CJSON_TYPE_INT, offsetof(T_prog_info_t, aud_type), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"aud_pid", CJSON_TYPE_INT, offsetof(T_prog_info_t, aud_pid), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"pcr_pid", CJSON_TYPE_INT, offsetof(T_prog_info_t, pcr_pid), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {NULL, 0, 0, 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX} // 结束标记
};

  CJsonStructFieldDef g_stTranscodeDecodeParamKey[] = {
    {"sync", CJSON_TYPE_INT, offsetof(T_TranscodeDecodeParam, sync), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"fail", CJSON_TYPE_INT, offsetof(T_TranscodeDecodeParam, fail), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"ach1", CJSON_TYPE_INT, offsetof(T_TranscodeDecodeParam, ach1), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"ach2", CJSON_TYPE_INT, offsetof(T_TranscodeDecodeParam, ach2), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"av1", CJSON_TYPE_INT, offsetof(T_TranscodeDecodeParam, av1), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"av2", CJSON_TYPE_INT, offsetof(T_TranscodeDecodeParam, av2), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"adelay1", CJSON_TYPE_INT, offsetof(T_TranscodeDecodeParam, adelay1), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"adelay2", CJSON_TYPE_INT, offsetof(T_TranscodeDecodeParam, adelay2), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"fmt", CJSON_TYPE_INT, offsetof(T_TranscodeDecodeParam, fmt), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"sdi", CJSON_TYPE_INT, offsetof(T_TranscodeDecodeParam, sdi), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"cs", CJSON_TYPE_INT, offsetof(T_TranscodeDecodeParam, cs), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"hdr", CJSON_TYPE_INT, offsetof(T_TranscodeDecodeParam, hdr), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"br", CJSON_TYPE_INT, offsetof(T_TranscodeDecodeParam, br), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"cr", CJSON_TYPE_INT, offsetof(T_TranscodeDecodeParam, cr), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"hu", CJSON_TYPE_INT, offsetof(T_TranscodeDecodeParam, hu), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"sa", CJSON_TYPE_INT, offsetof(T_TranscodeDecodeParam, sa), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"errfmfiltervalue", CJSON_TYPE_INT, offsetof(T_TranscodeDecodeParam, errfmfiltervalue), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {NULL, 0, 0, 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX} // 结束标记
};

  CJsonStructFieldDef g_stTranscodeDecodePidKey[] = {
    {"vp", CJSON_TYPE_INT, offsetof(T_TranscodeDecodePid, vp), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"vt", CJSON_TYPE_INT, offsetof(T_TranscodeDecodePid, vt), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"pp", CJSON_TYPE_INT, offsetof(T_TranscodeDecodePid, pp), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"ap1", CJSON_TYPE_INT, offsetof(T_TranscodeDecodePid, ap1), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"ap2", CJSON_TYPE_INT, offsetof(T_TranscodeDecodePid, ap2), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"at1", CJSON_TYPE_INT, offsetof(T_TranscodeDecodePid, at1), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {"at2", CJSON_TYPE_INT, offsetof(T_TranscodeDecodePid, at2), 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX},
    {NULL, 0, 0, 0, NULL, 0, 0, NULL, CJSON_TYPE_MAX} // 结束标记
};


#ifdef		__cplusplus
}
#endif

#endif
	

