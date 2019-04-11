//
//  sdl_audio_player.c
//  demo
//
//  Created by Apple on 2019/4/8.
//  Copyright © 2019年 Apple. All rights reserved.
//

#include <stdio.h>
#include <SDL2/SDL.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/samplefmt.h>
#include "base_ffmpeg.h"

#define BUFFER_SIZE 192000
Uint8 *audioChunk;
Uint32 audioLen;
Uint8 *audioPos;
void fillaudio(void *data,Uint8 *stream,int len){
    
    SDL_memset(stream, 0, len);
    
    if (audioLen<0) {
        return;
    }
    
    len=(len>audioLen?audioLen:len);
    
    SDL_MixAudio(stream, audioPos, len, SDL_MIX_MAXVOLUME);
    audioPos+=len;
    audioLen-=len;
}


int mainaudio(){
    AVFormatContext *inCtx=NULL;
    AVCodec *codec=NULL;
    AVCodecContext *codecCtx=NULL;
    struct SwrContext*swrCtx=NULL;
    
    char *mv="/Users/apple/Desktop/MV/陈奕迅 - 富士山下  (DUO 陈奕迅2010演唱会Live).mp4";
    char *toradora="/Users/apple/Desktop/龙与虎/Toradora! EP23 [BD 1080p 23.976fps AVC-yuv420p10 FLAC] - VCB-Studio & mawen1250.mkv";
    
    char *fileName=toradora;
    
    int stream_index;
    
    int ret;
    ret=initAVInfo(fileName, &stream_index, AVMEDIA_TYPE_AUDIO, &inCtx, &codecCtx, &codec);
//
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "init failed\n");
        return ret;
    }
    AVPacket pkt;
    av_init_packet(&pkt);
    
    AVFrame *frame=av_frame_alloc();
    AVFrame *pFrame=av_frame_alloc();
    
    //audio
    uint64_t out_channel_layout=AV_CH_LAYOUT_STEREO;
    int out_nb_samples=codecCtx->frame_size;
    enum AVSampleFormat out_fmt=AV_SAMPLE_FMT_S16;
    int out_sample_rate=codecCtx->sample_rate;
    int out_channels=av_get_channel_layout_nb_channels(out_channel_layout);
    int out_buffer_size=av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_fmt, 1);
    
    uint8_t *buffer=(uint8_t*)av_malloc(BUFFER_SIZE*2);
    
    ret= SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "SDL INIT FAILED %s\n",SDL_GetError());
        return ret;
    }
    
//    freq：采样率，如前所述。
//    格式：这告诉SDL我们将给它什么格式。 “S16SYS”中的“S”代表“有符号”，16表示每个样本长16位，“SYS”表示顺序取决于您所在的系统。这是avcodec_decode_audio2给我们音频的格式。
//    频道：音频频道的数量。
//    沉默：这是表示沉默的价值。由于音频是有符号的，0当然是通常的值。
//    样本：这是SDL在要求更多音频时给予我们的音频缓冲区的大小。这里的价值在512到8192之间。 ffplay使用1024。
//    callback：这是我们传递实际回调函数的地方。我们稍后会详细讨论回调函数。
//    userdata：SDL会给我们的回调一个void指针，指向我们想要回调函数的用户数据。我们想让它知道我们的编解码器上下文;你会明白为什么。
    SDL_AudioSpec audioSpec;
    audioSpec.freq=out_sample_rate;
    audioSpec.format=AUDIO_S16SYS;
    audioSpec.silence=0;
    audioSpec.samples=out_nb_samples;
    audioSpec.callback=fillaudio;
    audioSpec.userdata=codecCtx;
    
    //打开音频设备
    ret=SDL_OpenAudio(&audioSpec, NULL);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "cant open audio %s \n",SDL_GetError());
        return ret;
    }
    
    uint64_t in_channel_layout=av_get_default_channel_layout(codecCtx->channels);
    
    //初始化转换器
    swrCtx=swr_alloc();
    swrCtx=swr_alloc_set_opts(swrCtx, out_channel_layout, out_fmt, out_sample_rate, in_channel_layout, codecCtx->sample_fmt, codecCtx->sample_rate, 0, NULL);
    
    swr_init(swrCtx);
    //0是不暂停，非零是暂停
    SDL_PauseAudio(0);
    
    int got_frame;
    int size;
    while (av_read_frame(inCtx, &pkt)>=0) {
        if (pkt.stream_index==stream_index) {
           ret=avcodec_decode_audio4(codecCtx, frame, &got_frame, &pkt);
            if (ret<0) {
                av_log(NULL, AV_LOG_ERROR, "decode audio failed %s\n",av_err2str(ret));
                return ret;
            }
            
            if (got_frame) {
//                计算真正的数据大小 
                size=swr_convert(swrCtx, &buffer, BUFFER_SIZE, frame->data, frame->nb_samples);
                size*=(out_channels*av_get_bytes_per_sample(out_fmt));
            }
            
            while (audioLen>0) {
                SDL_Delay(1);
            }
            
            audioChunk=buffer;
            audioLen=size;
            audioPos=audioChunk;
        }
        
        av_packet_unref(&pkt);
    }
    swr_close(swrCtx);
    
    SDL_CloseAudio();
    SDL_Quit();
    av_frame_free(&frame);
    av_frame_free(&pFrame);
    avcodec_close(codecCtx);
    avformat_close_input(&inCtx);
    return 0;
}
