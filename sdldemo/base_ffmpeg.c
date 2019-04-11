//
//  base_ffmpeg.c
//  demo
//
//  Created by Apple on 2019/4/9.
//  Copyright © 2019年 Apple. All rights reserved.
//

#include "base_ffmpeg.h"
#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

int initAVInfo(char* filename, int* stream_index, enum AVMediaType type, AVFormatContext** inCtx, AVCodecContext** codecCtx, AVCodec** codec)
{
    int ret = 0;
    ret = avformat_open_input(inCtx, filename, NULL, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "avformat_open_input err = %s\n", av_err2str(ret));
        return ret;
    }
    
    if ((ret = avformat_find_stream_info(*inCtx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "avformat_find_stream_info err = %s\n", av_err2str(ret));
        return ret;
    }
    
    ret = av_find_best_stream(*inCtx, type, -1, 0, NULL, 0);
    
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "av_find_best_stream err = %s\n", av_err2str(ret));
        return ret;
    }
    
    *stream_index = ret;
    
    *codecCtx = (*inCtx)->streams[*stream_index]->codec;
    
    if (!*codecCtx) {
        av_log(NULL, AV_LOG_ERROR, "avcodeccontext is null err = %s\n", av_err2str(ret));
        return ret;
    }
    
    *codec = avcodec_find_decoder((*codecCtx)->codec_id);
    
    if (!*codec) {
        av_log(NULL, AV_LOG_ERROR, "avcodec_find_decoder is null err = %s\n", av_err2str(ret));
        return ret;
    }
    
    if ((ret = avcodec_open2(*codecCtx, *codec, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "av_find_best_stream err = %s\n", av_err2str(ret));
        return ret;
    }
    
    return 0;
}
