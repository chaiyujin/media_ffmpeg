#pragma once
#ifndef __YK_MEDIA_H__
#define __YK_MEDIA_H__

#include "ffmpeg.h"
#include "audio.h"
#include <string>

class Media {
private:
    // instance
    static Media *instance;

    // needed for decode
    AVFormatContext		*p_fmt_ctx;
    AVCodecContext		*p_video_codec_ctx,
        				*p_audio_codec_ctx;
    AVCodec				*p_video_codec,
                        *p_audio_codec;
    int                 video_stream_idx,
                        audio_stream_idx;
    std::string         pcm_file;

    // cannot ctor
    Media()
        : p_fmt_ctx(NULL),
          p_video_codec(NULL), p_video_codec_ctx(NULL),
          p_audio_codec(NULL), p_audio_codec_ctx(NULL),
          video_stream_idx(-1),audio_stream_idx(-1) {
        Audio::initialize();
    }
    ~Media() { delete instance; }
public:

    void            clear();

    static void     initialize();
    static int      load_media(const char *file_path);
    static int      process();
    static bool     set_parameters(std::string name, std::string value);
};

#endif
