#pragma once
#ifndef __YK_AUDIO_H__
#define __YK_AUDIO_H__

#include "ffmpeg.h"
#include "sphinx.h"

class Audio {
private:
    // instance
    static Audio *instance;

    // used for filter
    AVFilterContext     *buffersink_ctx;
    AVFilterContext     *buffersrc_ctx;
    AVFilterGraph       *filter_graph;
    AVFrame             *filt_frame;

    // cannot ctor
    Audio()
        : buffersink_ctx(NULL), buffersrc_ctx(NULL),
          filter_graph(NULL),   filt_frame(NULL) {
        Sphinx::initialize();
    }
    ~Audio() { clear(); }

    // methods
    void print_audio_frame(const AVFrame *frame, FILE *fp);
    void clear();
public:
    
    static void initialize();
    static void set_filters(
        AVSampleFormat audio_sample_fmt,
        int            audio_sample_rate,
        int            audio_channels,
        int64_t        audio_channel_layout,
        AVRational     audio_time_base);
    static void process(AVFrame *p_audio_frame, FILE *fp);
    static int  play_pcm(const char *file_path);
};

#endif  // !__YK_AUDIO_H__
