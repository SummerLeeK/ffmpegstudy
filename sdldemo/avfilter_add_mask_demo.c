//
//  avfilter_add_mask_demo.c
//  demo
//
//  Created by Apple on 2019/4/11.
//  Copyright © 2019年 Apple. All rights reserved.
//

#include <stdio.h>
#include <SDL2/SDL.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libswscale/swscale.h>
//黑白滤镜 部分视频呈现的效果并非黑白的
//const char *filter_descr = "lutyuv='u=128:v=128'";
//添加图片水印overlay=50:50 水印的xy坐标
const char *filter_descr = "movie=/Users/apple/Desktop/MV/大鱼/周深 - 大鱼_72.bmp[wm];[in][wm]overlay=50:50[out]";



static AVFormatContext *inCtx;
static AVCodec *codec;
static AVCodecContext *codecCtx;
static int stream_index;


static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static SDL_Rect videoRect;

static int screen_width;
static int screen_height;

static int video_width,video_height;


static AVFilterContext *buffersink_ctx;
static AVFilterContext *buffersrc_ctx;
static AVFilterGraph * filter_graph;

static char *args;

int initFormat(char *filename){
    
    int rst=0;
    rst=avformat_open_input(&inCtx, filename, NULL, NULL);
    
    if (rst<0) {
        av_log(NULL, AV_LOG_ERROR, "avformat_open_input failed %s\n",av_err2str(rst));
        return rst;
    }
    
    av_dump_format(inCtx, 0, filename, 0);
    
    rst=avformat_find_stream_info(inCtx, NULL);
    
    if (rst<0) {
        av_log(NULL, AV_LOG_ERROR, "avformat_find_stream_info failed %s \n",AVERROR(rst));
        
        return rst;
    }
    
    
    rst=av_find_best_stream(inCtx, AVMEDIA_TYPE_VIDEO, -1, 0, NULL, 0);
    
    if (rst<0) {
        av_log(NULL, AV_LOG_ERROR, "av_find_best_stream failed %s\n",AVERROR(rst));
        return rst;
    }
    
    stream_index=rst;
    
    codecCtx=inCtx->streams[stream_index]->codec;
    
    if (!codecCtx) {
        codecCtx=avcodec_alloc_context3(NULL);
        
        if (!codecCtx) {
            av_log(NULL, AV_LOG_ERROR, "avcodec_alloc_context3 failed \n");
            return -1;
        }
       rst=avcodec_parameters_copy(codecCtx, inCtx->streams[stream_index]->codecpar);
        
        if (rst<0) {
            av_log(NULL, AV_LOG_ERROR, "avcodec_parameters_copy failed %s\n",AVERROR(rst));
            return rst;
        }
    
    }
    
    codec=avcodec_find_decoder(codecCtx->codec_id);
    
    if (!codec) {
        av_log(NULL, AV_LOG_ERROR, "avcodec_find_decoder failed\n");
        return rst;
    }
    
    rst=avcodec_open2(codecCtx, codec, NULL);
    if (rst<0) {
        av_log(NULL, AV_LOG_ERROR, "avcodec_open2 failed %s\n",AVERROR(rst));
        return rst;
    }
    
    

    return 0;
    
}


int init_filter(){
    
    
    int ret;
    struct AVFilter *buffersrc=avfilter_get_by_name("buffer");
    struct AVFilter *buffersink=avfilter_get_by_name("buffersink");

    
    AVFilterInOut *outputs=avfilter_inout_alloc();
    AVFilterInOut *inputs=avfilter_inout_alloc();
    
    enum AVPixelFormat pixfmts[]={AV_PIX_FMT_YUV420P,AV_PIX_FMT_NONE};
    AVBufferSinkParams *buffersink_params;
    
    filter_graph=avfilter_graph_alloc();
    
    args=av_malloc(sizeof(char)*512);
    
    sprintf(args, "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",codecCtx->width,codecCtx->height,codecCtx->pix_fmt,codecCtx->time_base.num,codecCtx->time_base.den,
            codecCtx->sample_aspect_ratio.num,codecCtx->sample_aspect_ratio.den);
    
    ret=avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, NULL, filter_graph);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "buffersrc_ctx avfilter_graph_create_filter failed %d \t%s\n",ret,av_err2str(ret));
        
        return ret;
    }
    
    buffersink_params=av_buffersink_params_alloc();
    
    buffersink_params->pixel_fmts=pixfmts;
    
    ret=avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, buffersink_params, filter_graph);
    
   
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "buffersink_ctx avfilter_graph_create_filter failed %d\t%s\n",ret,av_err2str(ret));
        
        return ret;
    }
     av_free(buffersink_params);
    outputs->name=av_strdup("in");
    outputs->filter_ctx=buffersrc_ctx;
    outputs->pad_idx=0;
    outputs->next=NULL;
    
    inputs->name=av_strdup("out");
    inputs->filter_ctx=buffersink_ctx;
    inputs->pad_idx=0;
    inputs->next=NULL;
    
    ret=avfilter_graph_parse_ptr(filter_graph, filter_descr, &inputs, &outputs, NULL);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "avfilter_graph_parse_ptr failed %s\n",av_err2str(ret));
        return ret;
    }
    
    ret=avfilter_graph_config(filter_graph, NULL);
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "avfilter_graph_config failed %s\n",av_err2str(ret));
        return ret;
    }
    
    
    return 0;
}


int init_sdl_player(){
    video_width=codecCtx->width;
    video_height=codecCtx->height;
    
    screen_width=video_width/2;
    screen_height=video_height/2;
    
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
    
    window=SDL_CreateWindow(inCtx->filename, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,screen_width , screen_height, SDL_WINDOW_OPENGL);
    
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

int main(int argc,char**argv){
    
    int result=0;
    char *mv="/Users/apple/Desktop/MV/丢火车 - 《浮生之旅》丢火车乐队2018巡演纪录片.mp4";
    char *toradora="/Users/apple/Desktop/龙与虎/Toradora! EP23 [BD 1080p 23.976fps AVC-yuv420p10 FLAC] - VCB-Studio & mawen1250.mkv";

    char* fileName = toradora;

    result = initFormat(fileName);
    
    if (result<0) {
        av_log(NULL, AV_LOG_ERROR, "init format failed \n");
        return result;
    }
    
    
    result=init_filter();
    
    if (result<0) {
        av_log(NULL, AV_LOG_ERROR, "init filter failed \n");
        return result;
    }
    
    result=init_sdl_player();
    
    if (result<0) {
        av_log(NULL, AV_LOG_ERROR, "init sdl player failed \n");
        return result;
    }
    
    AVPacket pkt;
    
    av_init_packet(&pkt);
    
    int got_frame;
    struct SwsContext *swsCtx;
    AVFrame *frame=av_frame_alloc();
    AVFrame *pFrameYuv=av_frame_alloc();
    
    AVFrame *filter_frame=av_frame_alloc();
    

    
    int numBytes=avpicture_get_size(AV_PIX_FMT_YUV420P, screen_width, screen_height);
    uint8_t *buffer=(uint8_t*)av_malloc(sizeof(uint8_t)*numBytes);
    avpicture_fill(pFrameYuv->data,buffer , AV_PIX_FMT_YUV420P, screen_width, screen_height);
    
//    int numAvfilterBytes=avpicture_get_size(codecCtx->pix_fmt, video_width, video_height);
//    uint8_t *avfilterbuffer=(uint8_t*)av_malloc(sizeof(uint8_t)*numAvfilterBytes);
//    avpicture_fill(filter_frame->data,buffer , AV_PIX_FMT_YUV420P, screen_width, screen_height);
    
    while (av_read_frame(inCtx, &pkt)>=0) {
        if (pkt.stream_index==stream_index) {
           result=avcodec_send_packet(codecCtx, &pkt);
            if (result<0) {
                av_log(NULL, AV_LOG_ERROR, "avcodec_send_packet failed %s \n",av_err2str(result));
                return result;
            }
            
            while (avcodec_receive_frame(codecCtx, frame)>=0) {
//                av_log(NULL, AV_LOG_ERROR,"hahaha = %d\n", frame->format);
//
                frame->pts=frame->best_effort_timestamp;
                
                if(av_buffersrc_add_frame(buffersrc_ctx, frame)<0){
                    break;
                }
                
                while (1) {
                    result=av_buffersink_get_frame(buffersink_ctx, filter_frame);
                    if (result<0) {
                        break;
                    }
                    
                    //延迟去初始化变换器 因为有些时候filter_frame->format和视频的像素格式不一样
                    //原因目前未知 在添加水印的时候像素格式会发生变化
                    if (!swsCtx) {
                          swsCtx=sws_getCachedContext(swsCtx,video_width, video_height, filter_frame->format, screen_width, screen_height, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
                    }
                    
                    result=sws_scale(swsCtx, filter_frame->data, filter_frame->linesize, 0, filter_frame->height, pFrameYuv->data, pFrameYuv->linesize);
                    
//                         av_log(NULL, AV_LOG_ERROR, "%d \t %d \t %d\n",filter_frame->format,codecCtx->pix_fmt,result);
                    
                        SDL_UpdateYUVTexture(texture, &videoRect, pFrameYuv->data[0],pFrameYuv->linesize[0],pFrameYuv->data[1],pFrameYuv->linesize[1]
                                             ,pFrameYuv->data[2],pFrameYuv->linesize[2]);
                        
                        SDL_RenderClear(renderer);
                        SDL_RenderCopy(renderer, texture, NULL, NULL);
                        SDL_RenderPresent(renderer);
                        
                        SDL_Delay(40);
                    
                    av_frame_unref(filter_frame);
                }
            }
            
        }
        av_frame_unref(frame);
        
        av_packet_unref(&pkt);
    }

    return 0;
}
