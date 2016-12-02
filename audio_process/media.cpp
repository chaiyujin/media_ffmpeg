#include "media.h"

Media *Media::instance = NULL;

void save_frame(AVFrame *pFrame, int width, int height, int iFrame) {
    FILE *pFile;
    char szFilename[32];
    int  y;

    // Open file
    sprintf(szFilename, "data/frame%d.ppm", iFrame);
    pFile=fopen(szFilename, "wb");
    if(pFile==NULL)
        return;

    // Write header
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);

    // Write pixel data
    for(y=0; y<height; y++)
        fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);

    // Close file
    fclose(pFile);
    return;
}

int Media::load_media(const char *file_name) {
    if (!instance) return -1;
    Media &media = *instance;
    media.clear();
    
    // read the file
    media.p_fmt_ctx = avformat_alloc_context();
    if (avformat_open_input(&media.p_fmt_ctx, file_name, NULL, NULL) != 0) {
        printf("Fail to open file %s.\n", file_name);
    }
    if (avformat_find_stream_info(media.p_fmt_ctx, NULL) < 0) {
        printf("Fail to find stream information.\n");
    }

    for (int i = 0; i < media.p_fmt_ctx->nb_streams; i++) {
        if (media.p_fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            printf("Video Stream %d.\n", i);
            media.video_stream_idx = i;
        }
        else if (media.p_fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            printf("Audio Stream %d.\n", i);
            media.audio_stream_idx = i;
        }
    }

    if (media.video_stream_idx == -1 || 
        media.audio_stream_idx == -1) {
        printf("Fail to find video or audio stream.\n");
        return -1;
    }

    // get the information of video format
    av_dump_format(media.p_fmt_ctx, 0, file_name, 0);

    // process the video
    printf("Finding the video codec.\n");
    media.p_video_codec_ctx = media.p_fmt_ctx->streams[media.video_stream_idx]->codec;
    media.p_video_codec     = avcodec_find_decoder(media.p_video_codec_ctx->codec_id);
    if (media.p_video_codec == NULL) {
        printf("Codec not found.\n");
        return -1;
    }
    if (avcodec_open2(media.p_video_codec_ctx, media.p_video_codec, NULL) < 0) {
        printf("Fail to open codec.\n");
        return -1;
    }
    printf("Success.\n");

    // process audio
    printf("Finding the audio codec.\n");
    media.p_audio_codec_ctx = media.p_fmt_ctx->streams[media.audio_stream_idx]->codec;
    media.p_audio_codec     = avcodec_find_decoder(media.p_audio_codec_ctx->codec_id);
    if (media.p_audio_codec == NULL) {
        printf("Codec not found.\n");
        return -1;
    }
    if (avcodec_open2(media.p_audio_codec_ctx, media.p_audio_codec, NULL) < 0) {
        printf("Fail to open codec.\n");
        return -1;
    }
    // set for filter
    av_opt_set_int(media.p_audio_codec_ctx, "refcounted_frames", 1, 0);
    printf("Success.\n");

    Audio::set_filters(
        media.p_audio_codec_ctx->sample_fmt,
        media.p_audio_codec_ctx->sample_rate,
        media.p_audio_codec_ctx->channels,
        media.p_audio_codec_ctx->channel_layout,
        media.p_fmt_ctx->streams[media.audio_stream_idx]->time_base);

    return 0;
}

int Media::process() {
    if (!instance) return -1;
    Media &media = *instance;
    // process
    AVFrame *p_frame        = av_frame_alloc();
    AVFrame *p_frame_rgb    = av_frame_alloc();
    AVFrame *p_frame_a      = av_frame_alloc();
    AVFrame *filt_frame     = av_frame_alloc();

    p_frame = av_frame_alloc();
    p_frame_rgb = av_frame_alloc();
    p_frame_a = av_frame_alloc();
    filt_frame = av_frame_alloc();
    avpicture_alloc((AVPicture *)p_frame_rgb, AV_PIX_FMT_BGRA, media.p_video_codec_ctx->width, media.p_video_codec_ctx->height);

    int ret, got_picture, got_audio;
    AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));

    bool video_first_time = true;
    bool audio_first_time = true;

    struct SwsContext *img_convert_ctx;
    img_convert_ctx = sws_getContext(
        media.p_video_codec_ctx->width, media.p_video_codec_ctx->height, media.p_video_codec_ctx->pix_fmt,
        media.p_video_codec_ctx->width, media.p_video_codec_ctx->height, AV_PIX_FMT_BGRA,
        SWS_BICUBIC, NULL, NULL, NULL);

    FILE *pcm = fopen(media.pcm_file.c_str(), "wb");

    // read packet
    int video_frames = 0;
    while (av_read_frame(media.p_fmt_ctx, packet) >= 0) {
        // video
        if (packet->stream_index == media.video_stream_idx) {
            ret = avcodec_decode_video2(media.p_video_codec_ctx, p_frame, &got_picture, packet);
            double pts = av_frame_get_best_effort_timestamp(p_frame) * av_q2d(media.p_fmt_ctx->streams[media.video_stream_idx]->time_base);

            if (ret < 0) {
                printf("Decode error.\n");
                return -1;
            }
            // valid
            if (got_picture) {
                printf("Frame: %d  \r", video_frames++);
                sws_scale(img_convert_ctx, (const uint8_t * const *)p_frame->data, 
                    p_frame->linesize, 0, media.p_video_codec_ctx->height,
                    p_frame_rgb->data, p_frame_rgb->linesize);
                Video::process_image(
                    p_frame_rgb->data[0], 
                    media.p_video_codec_ctx->width,
                    media.p_video_codec_ctx->height,
                    "");
            }
        }
        // audio
        if (packet->stream_index == media.audio_stream_idx) {
            ret = avcodec_decode_audio4(media.p_audio_codec_ctx, p_frame_a, &got_audio, packet);

            if (ret < 0) {
                printf("Decode error.\n");
                return -1;
            }

            if (got_audio) {
                Audio::process(p_frame_a, pcm);
            }
        }
        av_free_packet(packet);
    }
    // close output pcm file

    fclose(pcm);

    sws_freeContext(img_convert_ctx);

    av_frame_free(&p_frame);
    av_frame_free(&p_frame_rgb);
    av_frame_free(&p_frame_a);
    av_frame_free(&filt_frame);

    media.clear();

    return 0;
}

void Media::initialize() {
    av_register_all();
    delete instance;
    instance = new Media();
    return;
}

void Media::clear() {
    if (p_fmt_ctx) { 
        avcodec_close(p_video_codec_ctx);
        avcodec_close(p_audio_codec_ctx);
        avformat_close_input(&p_fmt_ctx);
    }
    p_fmt_ctx = NULL;
    p_video_codec_ctx = 
        p_audio_codec_ctx = NULL;
    p_video_codec = 
        p_audio_codec = NULL;
    video_stream_idx = -1;
    audio_stream_idx = -1;
    return;
}

bool Media::set_parameter(string name, string value) {
    if (!instance) return false;
    if (name == "pcm") {
        instance->pcm_file = value;
        return true;
    }
    else if (name == "opencv") {
        return Video::set_parameter("opencv", value);
    }
    else if (name == "warp") {
        return Video::set_parameter("warp", value);
    }
    else if (name == "landmark_output") {
        return Video::set_parameter("landmark_output", value);
    }
    else if (name == "expression_output") {
        return Video::set_parameter("expression_output", value);
    }
    return false;
}