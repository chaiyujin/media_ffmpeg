#include "audio.h"
#include <dsound.h>
#include <windows.h>
#include <stdint.h>
#include <inttypes.h>
static const char *filter_descr = "aresample=16000,aformat=sample_fmts=s16:channel_layouts=mono";
Audio *Audio::instance = NULL;

void Audio::initialize() {
    if (instance) return;
    avfilter_register_all();
    instance = new Audio();

    return;
}

void Audio::set_filters(
    AVSampleFormat audio_sample_fmt,
    int            audio_sample_rate,
    int            audio_channels,
    int64_t        audio_channel_layout,
    AVRational     audio_time_base) {
    
    if (!instance) return;
    Audio &audio = *instance;

    audio.clear();

    char args[512];
    int ret = 0;
    AVFilter *abuffersrc  = avfilter_get_by_name("abuffer");
    AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    static const enum AVSampleFormat out_sample_fmts[] = { AV_SAMPLE_FMT_S16, (AVSampleFormat)-1 };
    static const int64_t out_channel_layouts[] = { AV_CH_LAYOUT_MONO, -1 };
    static const int out_sample_rates[] = { 16000, -1 };
    const AVFilterLink *outlink;
    AVRational time_base = audio_time_base;

    audio.filter_graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !audio.filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    /* buffer audio source: the decoded frames from the decoder will be inserted here. */
    if (!audio_channel_layout)
        audio_channel_layout = av_get_default_channel_layout(audio_channels);
    snprintf(args, sizeof(args),
        "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%I64x",
        time_base.num, time_base.den, audio_sample_rate,
        av_get_sample_fmt_name(audio_sample_fmt), audio_channel_layout);
    ret = avfilter_graph_create_filter(&audio.buffersrc_ctx, abuffersrc, "in",
                                       args, NULL, audio.filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source\n");
        goto end;
    }

    /* buffer audio sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&audio.buffersink_ctx, abuffersink, "out",
                                       NULL, NULL, audio.filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer sink\n");
        goto end;
    }

    ret = av_opt_set_int_list(audio.buffersink_ctx, "sample_fmts", out_sample_fmts, -1,
        AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        // ffmpeg sample
        av_log(NULL, AV_LOG_ERROR, "Cannot set output sample format\n");
        goto end;
    }

    ret = av_opt_set_int_list(audio.buffersink_ctx, "channel_layouts", out_channel_layouts, -1,
        AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output channel layout\n");
        goto end;
    }

    ret = av_opt_set_int_list(audio.buffersink_ctx, "sample_rates", out_sample_rates, -1,
        AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output sample rate\n");
        goto end;
    }

    /*
    * Set the endpoints for the filter graph. The filter_graph will
    * be linked to the graph described by filters_descr.
    */

    /*
    * The buffer source output must be connected to the input pad of
    * the first filter described by filters_descr; since the first
    * filter input label is not specified, it is set to "in" by
    * default.
    */
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = audio.buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    /*
    * The buffer sink input must be connected to the output pad of
    * the last filter described by filters_descr; since the last
    * filter output label is not specified, it is set to "out" by
    * default.
    */
    inputs->name       = av_strdup("out");
    inputs->filter_ctx = audio.buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    if ((ret = avfilter_graph_parse_ptr(audio.filter_graph, filter_descr,
        &inputs, &outputs, NULL)) < 0)
        goto end;

    if ((ret = avfilter_graph_config(audio.filter_graph, NULL)) < 0)
        goto end;

    /* Print summary of the sink buffer
    * Note: args buffer is reused to store channel layout string */
    outlink = audio.buffersink_ctx->inputs[0];
    av_get_channel_layout_string(args, sizeof(args), -1, outlink->channel_layout);
    av_log(NULL, AV_LOG_INFO, "Output: srate:%dHz fmt:%s chlayout:%s\n",
        (int)outlink->sample_rate,
        (char *)av_x_if_null(av_get_sample_fmt_name((AVSampleFormat)outlink->format), "?"),
        args);

end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    audio.filt_frame = av_frame_alloc();

    return;
}

void Audio::process(AVFrame *p_audio_frame, FILE *output_pcm) {
    if (!instance) return;
    Audio &audio = *instance;

    if (av_buffersrc_add_frame_flags(audio.buffersrc_ctx, p_audio_frame, 0) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error while feeding the audio filtergraph\n");
        return;
    }

    int ret;
    /* pull filtered audio from the filtergraph */
    while (1) {
        ret = av_buffersink_get_frame(audio.buffersink_ctx, audio.filt_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        if (ret < 0)
            return;
        audio.print_audio_frame(audio.filt_frame, output_pcm);
        av_frame_unref(audio.filt_frame);
    }
}

// ffmpeg sample
void Audio::print_audio_frame(const AVFrame *frame, FILE *output_pcm) {
    const int n = frame->nb_samples * av_get_channel_layout_nb_channels(av_frame_get_channel_layout(frame));
    const uint16_t *p     = (uint16_t*)frame->data[0];
    const uint16_t *p_end = p + n;

    while (p < p_end) {
        fputc(*p    & 0xff, output_pcm);
        fputc(*p>>8 & 0xff, output_pcm);
        p++;
    }
    fflush(output_pcm);
}

void Audio::clear() {
    if (filter_graph) {
        avfilter_graph_free(&filter_graph);
        av_frame_free(&filt_frame);
    }
}

#define MAX_AUDIO_BUF 4   
#define BUFFERNOTIFYSIZE 19200   


int sample_rate=16000;  //PCM sample rate  
int channels=1;         //PCM channel number  
int bits_per_sample=16; //bits per sample 

int Audio::play_pcm(const char *file_path) {
    int i;  
    FILE * fp;  
    if((fp=fopen(file_path,"rb"))==NULL){  
        printf("cannot open this file\n");  
        return -1;  
    }  

    IDirectSound8 *m_pDS=NULL;                    
    IDirectSoundBuffer8 *m_pDSBuffer8=NULL; //used to manage sound buffers.  
    IDirectSoundBuffer *m_pDSBuffer=NULL;     
    IDirectSoundNotify8 *m_pDSNotify=NULL;        
    DSBPOSITIONNOTIFY m_pDSPosNotify[MAX_AUDIO_BUF];  
    HANDLE m_event[MAX_AUDIO_BUF];  

    SetConsoleTitle(TEXT("Simplest Audio Play DirectSound"));//Console Title  
                                                             //Init DirectSound  
    if(FAILED(DirectSoundCreate8(NULL,&m_pDS,NULL)))  
        return FALSE;  
    if(FAILED(m_pDS->SetCooperativeLevel(FindWindow(NULL,TEXT("Simplest Audio Play DirectSound")),DSSCL_NORMAL)))  
        return FALSE;  


    DSBUFFERDESC dsbd;  
    memset(&dsbd,0,sizeof(dsbd));  
    dsbd.dwSize=sizeof(dsbd);  
    dsbd.dwFlags=DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY |DSBCAPS_GETCURRENTPOSITION2;  
    dsbd.dwBufferBytes=MAX_AUDIO_BUF*BUFFERNOTIFYSIZE;   
    //WAVE Header  
    dsbd.lpwfxFormat=(WAVEFORMATEX*)malloc(sizeof(WAVEFORMATEX));  
    dsbd.lpwfxFormat->wFormatTag=WAVE_FORMAT_PCM; 
    /* format type */  
    (dsbd.lpwfxFormat)->nChannels=channels;            
    /* number of channels (i.e. mono, stereo...) */  
    (dsbd.lpwfxFormat)->nSamplesPerSec=sample_rate;       
    /* sample rate */  
    (dsbd.lpwfxFormat)->nAvgBytesPerSec=sample_rate*(bits_per_sample/8)*channels;   
    /* for buffer estimation */  
    (dsbd.lpwfxFormat)->nBlockAlign=(bits_per_sample/8)*channels;          
    /* block size of data */  
    (dsbd.lpwfxFormat)->wBitsPerSample=bits_per_sample;       
    /* number of bits per sample of mono data */  
    (dsbd.lpwfxFormat)->cbSize=0;  

    //Creates a sound buffer object to manage audio samples.   
    if( FAILED(m_pDS->CreateSoundBuffer(&dsbd,&m_pDSBuffer,NULL))){     
        return FALSE;  
    }  
    if( FAILED(m_pDSBuffer->QueryInterface(IID_IDirectSoundBuffer8,(LPVOID*)&m_pDSBuffer8))){  
        return FALSE ;  
    }  
    //Get IDirectSoundNotify8  
    if(FAILED(m_pDSBuffer8->QueryInterface(IID_IDirectSoundNotify,(LPVOID*)&m_pDSNotify))){  
        return FALSE ;  
    }  
    for(i =0;i<MAX_AUDIO_BUF;i++){  
        m_pDSPosNotify[i].dwOffset =i*BUFFERNOTIFYSIZE;  
        m_event[i]=::CreateEvent(NULL,false,false,NULL);   
        m_pDSPosNotify[i].hEventNotify=m_event[i];  
    }  
    m_pDSNotify->SetNotificationPositions(MAX_AUDIO_BUF,m_pDSPosNotify);  
    m_pDSNotify->Release();  

    //Start Playing  
    BOOL isPlaying =TRUE;  
    LPVOID buf=NULL;  
    DWORD  buf_len=0;  
    DWORD res=WAIT_OBJECT_0;  
    DWORD offset=BUFFERNOTIFYSIZE;  

    m_pDSBuffer8->SetCurrentPosition(0);  
    m_pDSBuffer8->Play(0,0,DSBPLAY_LOOPING);  
    //Loop  
    while(isPlaying){  
        if((res >=WAIT_OBJECT_0)&&(res <=WAIT_OBJECT_0+3)){  
            m_pDSBuffer8->Lock(offset,BUFFERNOTIFYSIZE,&buf,&buf_len,NULL,NULL,0);  
            if(fread(buf,1,buf_len,fp)!=buf_len){  
                //File End  
                //Loop:  
                fseek(fp, 0, SEEK_SET);  
                fread(buf,1,buf_len,fp);  
                //Close:  
                //isPlaying=0;  
            }  
            m_pDSBuffer8->Unlock(buf,buf_len,NULL,0);  
            offset+=buf_len;  
            offset %= (BUFFERNOTIFYSIZE * MAX_AUDIO_BUF);  
            printf("this is %7d of buffer\n",offset);  
        }  
        res = WaitForMultipleObjects (MAX_AUDIO_BUF, m_event, FALSE, INFINITE);  
    }  
    return 0;
}