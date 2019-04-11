//
//  sdl_thread_player.c
//  demo
//
//  Created by Apple on 2019/4/8.
//  Copyright © 2019年 Apple. All rights reserved.
//

#include <stdio.h>
#include <SDL2/SDL.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include "base_ffmpeg.h"
#include <time.h>
#define REFRESH_EVENT (SDL_USEREVENT+1)
#define BREAK_EVENT (SDL_USEREVENT+2)
int threadexit = 0;
int threadpause = 0;
int delayTime = 0;
void stp_refresh_event(void* data)
{

    while (!threadexit) {
        if (!threadpause) {
            SDL_Event event;
            event.type = REFRESH_EVENT;
            SDL_PushEvent(&event);
        }
        SDL_Delay(delayTime);

    }
    
    SDL_Event event;
    event.type = BREAK_EVENT;
    SDL_PushEvent(&event);
}


int mainthreadvideo(int agrc, char* args[])
{
    AVFormatContext* inCtx = NULL;
    AVCodec* codec = NULL;
    AVCodecContext* codecCtx = NULL;

    struct SwsContext* imgSwsContext = NULL;

    int ret = 0;

    char* mv = "/Users/apple/Desktop/MV/陈奕迅 - 富士山下  (DUO 陈奕迅2010演唱会Live).mp4";
    char* toradora = "/Users/apple/Desktop/龙与虎/Toradora! EP23 [BD 1080p 23.976fps AVC-yuv420p10 FLAC] - VCB-Studio & mawen1250.mkv";

    char* netUrl="https://media.w3.org/2010/05/sintel/trailer.mp4";
    char* filename = mv;

    int stream_index=-1;

    ret = initAVInfo(filename, &stream_index, AVMEDIA_TYPE_VIDEO, &inCtx, &codecCtx, &codec);


    av_dump_format(inCtx, 0, NULL, 0);

    if (ret < 0) {
        return ret;
    }

    int video_width;
    int video_height;
    int screen_width;
    int screen_height;

    video_width = codecCtx->width;
    video_height = codecCtx->height;

    screen_width = video_width / 2;
    screen_height = video_height / 2;
    AVFrame* frame = av_frame_alloc();
    AVFrame* pFrameYuv = av_frame_alloc();
    AVPacket pkt;
    uint8_t* buffer;

    av_init_packet(&pkt);

    delayTime = 1000 / (codecCtx->framerate.num / codecCtx->framerate.den);

    imgSwsContext = sws_getContext(video_width, video_height, codecCtx->pix_fmt, screen_width, screen_height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    int numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, screen_width, screen_height);

    buffer = (uint8_t*)av_malloc(sizeof(uint8_t) * numBytes);

    avpicture_fill((AVPicture*)pFrameYuv->data, buffer, AV_PIX_FMT_YUV420P, screen_width, screen_height);

    if (!imgSwsContext) {
        av_log(NULL, AV_LOG_ERROR, "sws_getContext failed \n");
        return -1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)<0) {
        av_log(NULL, AV_LOG_ERROR, "SDL INIT FAILED %s\n",SDL_GetError());
        return -1;
    }

    SDL_Thread* video_id = SDL_CreateThread(stp_refresh_event, NULL, NULL);
    if (!video_id) {
        av_log(NULL, AV_LOG_ERROR, "SDL_CreateThread failed  %s\n", SDL_GetError());
        return -1;
    }


    SDL_Window* window = SDL_CreateWindow(filename, 0, 0, screen_width, screen_height, SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS);

    if (!window) {
        av_log(NULL, AV_LOG_ERROR, "SDL_CreateWindow failed %s\n", SDL_GetError());
        return -1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    if (!renderer) {
        av_log(NULL, AV_LOG_ERROR, "SDL_CreateRenderer failed %s\n", SDL_GetError());
        return -1;
    }

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, screen_width, screen_height);

    if (!texture) {
        av_log(NULL, AV_LOG_ERROR, "SDL_CreateTexture failed %s\n", SDL_GetError());
        return -1;
    }


    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = screen_width;
    rect.h = screen_height;

    int got_pic;
    SDL_Event event;
    int numframe=0;
    int pretime=0;
    struct timeval tv;
    
    av_log(NULL, AV_LOG_ERROR, "delayTime = %d\n",delayTime);
    for(;;) {

        SDL_WaitEvent(&event);

        if (event.type == REFRESH_EVENT) {
            while (1) {
                
            if (av_read_frame(inCtx, &pkt) < 0) {
                threadexit = 1;
                av_log(NULL, AV_LOG_ERROR, "av_read_frame failed \n");
            }
            if (pkt.stream_index == stream_index) {
                break;
            }}

            ret = avcodec_decode_video2(codecCtx, frame, &got_pic, &pkt);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "avcodec_decode_video2 %s\n", av_err2str(ret));
                return ret;
            }

            if (got_pic) {
              
                sws_scale(imgSwsContext, frame->data, frame->linesize, 0, frame->height, pFrameYuv->data, pFrameYuv->linesize);
                
                SDL_UpdateYUVTexture(texture, &rect, pFrameYuv->data[0], pFrameYuv->linesize[0], pFrameYuv->data[1], pFrameYuv->linesize[1], pFrameYuv->data[2],pFrameYuv->linesize[2]);
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                SDL_RenderPresent(renderer);
                ++numframe;
                gettimeofday(&tv,NULL);
                if (pretime<=0) {
                    pretime=(tv.tv_sec*1000+tv.tv_usec/1000);
                }
//                av_log(NULL, AV_LOG_ERROR, "%d \t %d\n",numframe,(tv.tv_sec*1000+tv.tv_usec/1000)-pretime);
                pretime=(tv.tv_sec*1000+tv.tv_usec/1000);
            }
            av_packet_unref(&pkt);
        }else if(event.type==BREAK_EVENT){
            av_log(NULL, AV_LOG_ERROR, "got BREAK_EVENT \n");
            break;
        }

    }


    sws_freeContext(imgSwsContext);

    SDL_Quit();

    av_frame_free(&frame);
    av_frame_free(&pFrameYuv);

    avcodec_close(codecCtx);
    avformat_close_input(&inCtx);


    return 0;
}
