#include "media.h"
#include <fstream>
using namespace std;
Media *Media::instance = NULL;
vector<Frame> *frame_data;
static int slider_max;
static int data_index = 0;

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

int Media::read_media(const char *file_name) {
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

int Media::load_media(const char *file_name) {
    if (Media::read_media(file_name)) {
        return -1;
    }
    instance->media_file = file_name;
    instance->scan();
    Media::read_media(file_name);
    return 0;
}

int Media::process() {
    if (!instance) return -1;
    Media &media = *instance;


    for (int i = 0; i < media.time_stamps.size(); i += 2) {
        if (i >= media.time_stamps.size()) break;
        cout << media.time_stamps.size() << " " << i << " ?????\n";
        double start_time = media.time_stamps[i];
        double end_time   = media.time_stamps[i + 1];
        if (end_time - start_time < 2) continue;
        cout << start_time << " " << end_time << endl;
        char pcm[100];
        char lm[100];
        char ex[100];
        char phone[100];
        sprintf(pcm,   "processed/data%d.pcm", data_index);
        sprintf(lm,    "processed/data%d.land", data_index);
        sprintf(ex,    "processed/data%d.expr", data_index);
        sprintf(phone, "processed/data%d.phone", data_index);
        set_parameter("pcm", pcm);
        set_parameter("landmark_output", lm);
        set_parameter("expression_output", ex);
        process(start_time, end_time);
        Sphinx::run_pcm(pcm, phone);
        data_index++;
    }

    //media.clear();
}

int Media::process(double start_time, double end_time) {
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
    av_seek_frame(media.p_fmt_ctx, media.video_stream_idx, 0, AVSEEK_FLAG_BACKWARD);
    bool video_done = false;
    bool audio_done = false;
    while (!video_done && !audio_done && (av_read_frame(media.p_fmt_ctx, packet) >= 0)) {
        // video
        if (packet->stream_index == media.video_stream_idx) {
            ret = avcodec_decode_video2(media.p_video_codec_ctx, p_frame, &got_picture, packet);
            double pts = av_frame_get_best_effort_timestamp(p_frame) * av_q2d(media.p_fmt_ctx->streams[media.video_stream_idx]->time_base);
            if (pts > end_time) video_done = true;
            if (!(pts >= start_time && pts <= end_time)) continue;
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
            double pts = av_frame_get_best_effort_timestamp(p_frame_a) * av_q2d(media.p_fmt_ctx->streams[media.audio_stream_idx]->time_base);
            if (pts > end_time) audio_done = true;
            if (!(pts >= start_time && pts <= end_time)) continue;
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
    printf("\n");
    fclose(pcm);

    sws_freeContext(img_convert_ctx);

    av_frame_free(&p_frame);
    av_frame_free(&p_frame_rgb);
    av_frame_free(&p_frame_a);
    av_frame_free(&filt_frame);

    return 0;
}

void Media::initialize() {
    if (instance) return;
    av_register_all();
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
    p_video_codec_ctx = NULL;
    p_audio_codec_ctx = NULL;
    p_video_codec = NULL;
    p_audio_codec = NULL;
    video_stream_idx = -1;
    audio_stream_idx = -1;

    destroyAllWindows();
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

void on_trackbar(int pos, void *) {
    vector<Frame> &picture_data = *frame_data;
    imshow("select", picture_data[pos].img);
}

void Media::scan() {
    while (true) {
        // process
        AVFrame *p_frame        = av_frame_alloc();
        AVFrame *p_frame_rgb    = av_frame_alloc();
        AVFrame *p_frame_a      = av_frame_alloc();
        AVFrame *filt_frame     = av_frame_alloc();

        p_frame = av_frame_alloc();
        p_frame_rgb = av_frame_alloc();
        p_frame_a = av_frame_alloc();
        filt_frame = av_frame_alloc();
        avpicture_alloc((AVPicture *)p_frame_rgb, AV_PIX_FMT_BGRA, p_video_codec_ctx->width, p_video_codec_ctx->height);

        int ret, got_picture, got_audio;
        AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));

        struct SwsContext *img_convert_ctx;
        img_convert_ctx = sws_getContext(
            p_video_codec_ctx->width, p_video_codec_ctx->height, p_video_codec_ctx->pix_fmt,
            p_video_codec_ctx->width, p_video_codec_ctx->height, AV_PIX_FMT_BGRA,
            SWS_BICUBIC, NULL, NULL, NULL);

        // read packet
        time_stamps.clear();
        namedWindow("select");

        int video_frames = 0;
        int frame_size = p_video_codec_ctx->height * p_video_codec_ctx->width * 4;
        while (av_read_frame(p_fmt_ctx, packet) >= 0) {
            // video
            if (packet->stream_index == video_stream_idx) {
                ret = avcodec_decode_video2(p_video_codec_ctx, p_frame, &got_picture, packet);
                double pts = av_frame_get_best_effort_timestamp(p_frame) * av_q2d(p_fmt_ctx->streams[video_stream_idx]->time_base);

                if (ret < 0) {
                    printf("Decode error.\n");
                }
                // valid
                if (got_picture) {
                    printf("Frame: %d  \r", video_frames++);
                    sws_scale(img_convert_ctx, (const uint8_t * const *)p_frame->data, 
                        p_frame->linesize, 0, p_video_codec_ctx->height,
                        p_frame_rgb->data, p_frame_rgb->linesize);
                    Frame frame;
                    frame.img = Mat(p_video_codec_ctx->height, p_video_codec_ctx->width, CV_8UC3);
                    frame.pts  = pts;
                    int index = 0;
                    for (int r = 0; r < frame.img.rows; ++r) {
                        byte *data = frame.img.ptr<byte>(r);
                        for (int c = 0; c < frame.img.cols * 3; c += 3) {
                            data[c]     = p_frame_rgb->data[0][index++];
                            data[c + 1] = p_frame_rgb->data[0][index++];
                            data[c + 2] = p_frame_rgb->data[0][index++];
                            index++;
                        }
                    }

                    imshow("select", frame.img);
                    char c = waitKey();
                    if (c == 'q') {
                        if (time_stamps.size()) save_time_stamps();
                        break;
                    }
                    else if (c == 't') {
                        cout << "Select frame at time: " << pts << endl;
                        time_stamps.push_back(pts);
                    }
                    else if (c == 'z') {
                        cout << "Undo\n";
                        if (time_stamps.size()) time_stamps.pop_back();
                    }
                    else if (c == 'u') {
                        cout << "Update time: " << pts << endl;
                        time_stamps[time_stamps.size() - 1] = pts;
                    }
                }
            }
        }
        printf("\n");
        // close output pcm file
        sws_freeContext(img_convert_ctx);
        av_frame_free(&p_frame);
        av_frame_free(&p_frame_rgb);
        av_frame_free(&p_frame_a);
        av_frame_free(&filt_frame);


        if (time_stamps.size() > 0 && time_stamps.size() % 2 == 0)
            save_time_stamps();

        load_time_stamps();
        destroyWindow("select");

        if (time_stamps.size() > 0 && time_stamps.size() % 2 == 0) break;
        printf("Error in time stamps.\n");
    }
}

void Media::save_time_stamps() {
    ofstream fout(media_file + ".times");
    fout << time_stamps.size() << endl;
    for (int i = 0; i < time_stamps.size(); ++i) {
        fout << time_stamps[i] << endl;
    }
    fout.close();
}

void Media::load_time_stamps() {
    ifstream fin(media_file + ".times");
    time_stamps.clear();
    if (fin.is_open()) {
        int count;
        fin >> count;
        printf("Set %d time stamps:\n", count);
        for (int i = 0; i < count; ++i) {
            double x;
            fin >> x;
            time_stamps.push_back(x);
            printf("%f ", x);
        }
        printf("\n");
        fin.close();
    }
}