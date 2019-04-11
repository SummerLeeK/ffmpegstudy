//
//  base_ffmpeg.h
//  demo
//
//  Created by Apple on 2019/4/9.
//  Copyright © 2019年 Apple. All rights reserved.
//

#ifndef base_ffmpeg_h
#define base_ffmpeg_h

#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#endif /* base_ffmpeg_h */

int initAVInfo(char* filename, int* stream_index, enum AVMediaType type, AVFormatContext** inCtx, AVCodecContext** codecCtx, AVCodec** codec);
