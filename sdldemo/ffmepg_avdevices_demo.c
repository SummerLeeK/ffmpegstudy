//
//  ffmepg_avdevices_demo.c
//  demo
//  获取摄像头并sdl显示
//  参考 https://blog.csdn.net/leixiaohua1020/article/details/39702113
//  Created by Apple on 2019/4/12.
//  Copyright © 2019年 Apple. All rights reserved.
//

#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <SDL2/SDL.h>

AVFormatContext* fmtCtx;
AVInputFormat *inCtx;
AVCodecContext *codecCtx;
AVCodec *codec;

static int stream_index;
static int video_width;
static int video_height;

static int screen_width;
static int screen_height;

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture  *texture;
SDL_Rect videoRect;

static int init_sdl_player(){
    video_width=codecCtx->width;
    video_height=codecCtx->height;
    
    screen_width=video_width/2;
    screen_height=video_height/2;
    
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
    
    window=SDL_CreateWindow(fmtCtx->filename, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,screen_width , screen_height, SDL_WINDOW_OPENGL);
    
    if (!window) {
        av_log(NULL, AV_LOG_ERROR, "cant create window %s \n",SDL_GetError());
        return -1;
    }
    
    renderer=SDL_CreateRenderer(window, -1, 0);
    
    if (!renderer) {
        av_log(NULL, AV_LOG_ERROR, "cant create renderer %s \n",SDL_GetError());
        return -1;
    }
    
    videoRect.x=0;
    videoRect.y=0;
    videoRect.w=screen_width;
    videoRect.h=screen_height;
    
    
    texture=SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV,SDL_TEXTUREACCESS_STREAMING , screen_width, screen_height);
    
    if (!texture) {
        av_log(NULL, AV_LOG_ERROR, "cant create texture %s \n",SDL_GetError());
        return -1;
    }
    return 0;
}


static void freeSDL(){
    SDL_Quit();
}


//Show AVFoundation Device
//打印macos上的音频视频输入设备名及信息
void show_avfoundation_device(){
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    AVDictionary* options = NULL;
    av_dict_set(&options, "framerate", "30", 0);
    av_dict_set(&options, "video_size", "1280x720", 0);
    av_dict_set(&options,"list_devices","true",0);
    AVInputFormat *iformat = av_find_input_format("avfoundation");
    printf("==AVFoundation Device Info===\n");
    avformat_open_input(&pFormatCtx,"",iformat,&options);
    printf("=============================\n");
}

int main(int argc,char**argv){
    
    int ret;
    //这句必加 我还以为所有的register_all都被废弃了
    avdevice_register_all();
//    show_avfoundation_device();
    AVDictionary* options = NULL;
//  寻找Macos上的摄像头设备
    inCtx=av_find_input_format("avfoundation");
    av_dict_set(&options, "framerate", "30", 0);
    av_dict_set(&options, "video_size", "1280x720", 0);
    if (!inCtx) {
        av_log(NULL, AV_LOG_ERROR, "av_find_input_format failed \n");
        return -1;
    }
    
    ret=avformat_open_input(&fmtCtx, "0", inCtx, &options);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "avformat_open_input failed %s\n",av_err2str(ret));
        return ret;
    }
    
    
    ret=avformat_find_stream_info(fmtCtx, NULL);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "");
        return ret;
    }
    av_dump_format(fmtCtx, 0, NULL, 0);
    
    ret=av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, 0, NULL, 0);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "av_find_best_stream failed %s\n",av_err2str(ret));
        return ret;
    }
    
    stream_index=ret;
    
    codecCtx=avcodec_alloc_context3(NULL);
    
    ret =avcodec_parameters_to_context(codecCtx,fmtCtx->streams[stream_index]->codecpar);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "avcodec_parameters_to_context failed %s \n",av_err2str(ret));
        return ret;
    }
    
    codec=avcodec_find_decoder(codecCtx->codec_id);
    
    if (!codec) {
        av_log(NULL, AV_LOG_ERROR, "avcodec_find_decoder failed %d",codecCtx->codec_id);
        return -1;
    }
    
    ret=avcodec_open2(codecCtx, codec, NULL);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "avcodec_open2 failed %s",av_err2str(ret));
        return ret;
    }
    
    ret =init_sdl_player();
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "init_sdl_player failed \n");
        return -1;
    }
    
    AVFrame *frame,*pFrameYuv;
    
    frame=av_frame_alloc();
    
    pFrameYuv=av_frame_alloc();
    
    int numBytes=avpicture_get_size(AV_PIX_FMT_YUV420P, screen_width, screen_width);
    uint8_t *buffer=(uint8_t *)av_malloc(sizeof(uint8_t)*numBytes);
    
    avpicture_fill(pFrameYuv->data, buffer,AV_PIX_FMT_YUV420P , screen_width, screen_height);
    
    AVPacket pkt;
    
    struct SwsContext *imageSwsContext;
    
    
    av_init_packet(&pkt);
    
    
    av_log(NULL,AV_LOG_ERROR, "%d\t%d\t%d\t%d\n",video_width,video_height,screen_width,screen_height);
    
    while (av_read_frame(fmtCtx, &pkt)>=0) {
        if (pkt.stream_index==stream_index) {
            ret =avcodec_send_packet(codecCtx, &pkt);
            if (ret<0) {
                av_log(NULL, AV_LOG_ERROR, "avcodec_send_packet failed %s \n",av_err2str(ret));
                break;
            }
            
            while (avcodec_receive_frame(codecCtx, frame)>=0) {
               
                if (!imageSwsContext) {
                    imageSwsContext=sws_getCachedContext(imageSwsContext, video_width, video_height, frame->format, screen_width, screen_height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
                }
                
                int result=sws_scale(imageSwsContext, frame->data, frame->linesize, 0, frame->height, pFrameYuv->data, pFrameYuv->linesize);
                
                SDL_UpdateYUVTexture(texture, &videoRect, pFrameYuv->data[0], pFrameYuv->linesize[0], pFrameYuv->data[1], pFrameYuv->linesize[1], pFrameYuv->data[2], pFrameYuv->linesize[2]);
                
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                SDL_RenderPresent(renderer);
                
                SDL_Delay(40);
                
            }
        }
        
        av_packet_unref(&pkt);
    }
    
    freeSDL();
    
    avformat_close_input(fmtCtx);
    

    
    return 0;
}
