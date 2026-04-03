#include "common.h"

int string_to_argv(const char *cmd_line, char ***argv)
{
    if (!cmd_line || !argv) return 0;

    char *line = strdup(cmd_line);
    if (!line) return 0;

    int count = 0;

    /* 第一次遍历计算参数数量 */
    char *temp = strdup(cmd_line);
    if (!temp) {
        free(line);
        return 0;
    }
    char *token = strtok(temp, " ");
    while (token != NULL)
    {
        count++;
        token = strtok(NULL, " ");
    }
    free(temp);

    if (count == 0) {
        free(line);
        *argv = NULL;
        return 0;
    }

    /* 分配 argv 数组 */
    *argv = (char **)malloc((count + 1) * sizeof(char *));
    if (!*argv) {
        free(line);
        return 0;
    }

    /* 第二次遍历填充参数 */
    token = strtok(line, " ");
    for (int i = 0; i < count; i++)
    {
        if (!token) {
            /* 理论上不应发生，防御性处理 */
            (*argv)[i] = NULL;
        } else {
            (*argv)[i] = strdup(token);
            if (!(*argv)[i]) {
                /* strdup 失败，释放已分配的所有项 */
                for (int j = 0; j < i; j++) free((*argv)[j]);
                free(*argv);
                *argv = NULL;
                free(line);
                return 0;
            }
            token = strtok(NULL, " ");
        }
    }
    (*argv)[count] = NULL;

    free(line);
    return count;
}

void free_argv(char **argv, int argc)
{
    for (int i = 0; i < argc; i++)
    {
        free(argv[i]);
    }
    free(argv);
}

static const struct
{
    uapi_acodec_id acodec_id;
    INT8 *str;
} g_acodec_str_map[] = {
    {UAPI_ACODEC_ID_PCM, "pcm"},
    {UAPI_ACODEC_ID_MP2, "mp2"},
    {UAPI_ACODEC_ID_AAC, "aac"},
    {UAPI_ACODEC_ID_MP3, "mp3"},
    {UAPI_ACODEC_ID_AMRNB, "amrnb"},
    {UAPI_ACODEC_ID_AMRWB, "amrwb"},
    {UAPI_ACODEC_ID_G711, "g711"},
    {UAPI_ACODEC_ID_G726, "g726"},
    {UAPI_ACODEC_ID_ADPCM, "adpcm"},
    {UAPI_ACODEC_ID_AC3PASSTHROUGH, "ac3passthrough"},
    {UAPI_ACODEC_ID_DTSPASSTHROUGH, "dtspassthrough"},
    {UAPI_ACODEC_ID_TRUEHD, "truehd"},
    {UAPI_ACODEC_ID_DOLBY_TRUEHD, "dolby_truehd"},
    {UAPI_ACODEC_ID_DTSHD, "dtshd"},
    {UAPI_ACODEC_ID_DTSM6, "dtsm6"},
#if defined(DOLBYPLUS_HACODEC_SUPPORT)
    {UAPI_ACODEC_ID_DOLBY_PLUS, "dolby_plus"},
#endif
    {UAPI_ACODEC_ID_MS12_DDP, "ms12_ddp"},
    {UAPI_ACODEC_ID_MS12_AAC, "ms12_aac"},
    {UAPI_ACODEC_ID_VORBIS, "vorbis"},
    {UAPI_ACODEC_ID_OPUS, "opus"},
    {UAPI_ACODEC_ID_DRA, "dra"},
    {UAPI_ACODEC_ID_AVS3, "avs3a"}};

static const struct
{
    uapi_vcodec_type vcodec_id;
    INT8 *str;
} g_vcodec_str_map[] = {
    {UAPI_VCODEC_TYPE_MPEG2, "mpeg2"}, // h.262
    {UAPI_VCODEC_TYPE_MPEG4, "mpeg4"}, // MPEG-4 Part 2
    {UAPI_VCODEC_TYPE_H263, "h263"},
    {UAPI_VCODEC_TYPE_SORENSON, "sor"},
    {UAPI_VCODEC_TYPE_VP6, "vp6"},
    {UAPI_VCODEC_TYPE_VP6F, "vp6f"},
    {UAPI_VCODEC_TYPE_VP6A, "vp6a"},
    {UAPI_VCODEC_TYPE_H264, "h264"}, // MPEG-4 Part 10  AVC
    {UAPI_VCODEC_TYPE_H265, "h265"},
    {UAPI_VCODEC_TYPE_H264_MVC, "mvc"},
    {UAPI_VCODEC_TYPE_AVS, "avs"},
    {UAPI_VCODEC_TYPE_AVS2, "avs2"},
    {UAPI_VCODEC_TYPE_AVS3, "avs3"},
    {UAPI_VCODEC_TYPE_REAL8, "real8"},
    {UAPI_VCODEC_TYPE_REAL9, "real9"},
    {UAPI_VCODEC_TYPE_VC1, "vc1ap"},
    {UAPI_VCODEC_TYPE_VC1, "vc1smp5"},
    {UAPI_VCODEC_TYPE_VC1, "vc1smp8"},
    {UAPI_VCODEC_TYPE_VP8, "vp8"},
    {UAPI_VCODEC_TYPE_VP9, "vp9"},
    {UAPI_VCODEC_TYPE_AV1, "av1"},
    {UAPI_VCODEC_TYPE_DIVX3, "divx3"},
    {UAPI_VCODEC_TYPE_MJPEG, "mjpeg"},
};

INT8 *get_vcodec_str(UINT32 vcodec_id)
{
    INT32 map_size = (INT32)(sizeof(g_vcodec_str_map) / sizeof(g_vcodec_str_map[0]));

    for (int i = 0; i < map_size; i++)
    {
        if (g_vcodec_str_map[i].vcodec_id == vcodec_id)
        {
            return g_vcodec_str_map[i].str;
        }
    }

    return NULL;
}

INT8 *get_acodec_str(UINT32 acodec_id)
{
    INT32 map_size = (INT32)(sizeof(g_acodec_str_map) / sizeof(g_acodec_str_map[0]));

    for (int i = 0; i < map_size; i++)
    {
        if (g_acodec_str_map[i].acodec_id == acodec_id)
        {
            return g_acodec_str_map[i].str;
        }
    }

    return NULL;
}

INT32 get_acodec_type(INT8 *acodec_str)
{
    INT32 map_size = (INT32)(sizeof(g_acodec_str_map) / sizeof(g_acodec_str_map[0]));

    for (int i = 0; i < map_size; i++)
    {
        if (!strcasecmp(acodec_str, g_acodec_str_map[i].str))
        {
            return g_acodec_str_map[i].acodec_id;
        }
    }
    return 0xffffffff;
}

INT32 get_vcodec_type(INT8 *vcodec_str)
{
    INT32 map_size = (INT32)(sizeof(g_vcodec_str_map) / sizeof(g_vcodec_str_map[0]));

    for (int i = 0; i < map_size; i++)
    {
        if (!strcasecmp(vcodec_str, g_vcodec_str_map[i].str))
        {
            return g_vcodec_str_map[i].vcodec_id;
        }
    }
    return UAPI_VCODEC_TYPE_MAX;
}

int GetSizeFromString(const char *resolutionStr, uint32_t *width, uint32_t *height)
{
    if (!resolutionStr || !width || !height)
    {
        return -1;
    }

    *width = 0;
    *height = 0;
    int w, h;
    if (sscanf(resolutionStr, "%dx%d", &w, &h) == 2)
    {
        if (w > 0 && h > 0)
        {
            *width = w;
            *height = h;
            return 0;
        }
    }

    if (strcmp(resolutionStr, "4K") == 0 || strcmp(resolutionStr, "4k") == 0)
    {
        *width = 3840;
        *height = 2160;
        return 0;
    }
    else if (strcmp(resolutionStr, "1080P") == 0 || strcmp(resolutionStr, "1080p") == 0)
    {
        *width = 1920;
        *height = 1080;
        return 0;
    }
    else if (strcmp(resolutionStr, "720P") == 0 || strcmp(resolutionStr, "720p") == 0)
    {
        *width = 1280;
        *height = 720;
        return 0;
    }
    else if (strcmp(resolutionStr, "480P") == 0 || strcmp(resolutionStr, "480p") == 0)
    {
        *width = 854;
        *height = 480;
        return 0;
    }
    return -1;
}