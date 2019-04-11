//
//  sdl_video_player.c
//  demo
//
//  Created by Apple on 2019/4/4.
//  Copyright © 2019年 Apple. All rights reserved.
//

#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <SDL2/SDL.h>
#include <libswscale/swscale.h>


#define OUTPUT_YUV420P 0

int mainsdl(int argc, char* argv[])
{
    AVFormatContext* inCtx = NULL;
    char *mv="/Users/apple/Desktop/MV/陈奕迅 - 富士山下  (DUO 陈奕迅2010演唱会Live).mp4";
    char *toradora="/Users/apple/Desktop/龙与虎/Toradora! EP23 [BD 1080p 23.976fps AVC-yuv420p10 FLAC] - VCB-Studio & mawen1250.mkv";
    char* fileName = toradora;

    AVStream* videoStream;
    AVCodecContext* codecCtx;
    AVCodec* codec;
    int stream_index;
    int ret;
    AVFrame* frame;
    int delayTime=0;
    av_log_set_level(AV_LOG_INFO);

    struct SwsContext*imgSwsContext;
    
#if OUTPUT_YUV420P
    FILE *fp_yuv=fopen("/Users/apple/Desktop/龙与虎/1.yuv","wb+");
#endif
    av_register_all();
    avformat_network_init();
    
    inCtx=avformat_alloc_context();
    
    ret = avformat_open_input(&inCtx, fileName, NULL, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "open inputformat failed %s\t", av_err2str(ret));
        return -1;
    }
    
    //必须加这一句 否则流信息 部分获取不到
    if(avformat_find_stream_info(inCtx,NULL)<0){
        printf("Couldn't find stream information.\n");
        return -1;
    }
    
    stream_index=-1;
    
    ret = av_find_best_stream(inCtx, AVMEDIA_TYPE_VIDEO, -1, 0, NULL, 0);

    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "cant find best target stream : %s\n", av_err2str(ret));
        goto _Close_Input_Format;
    }

    stream_index = ret;

    AVPacket pkt;

    videoStream = inCtx->streams[stream_index];

    
    codecCtx = inCtx->streams[stream_index]->codec;

    if (!codecCtx) {
        av_log(NULL, AV_LOG_ERROR, "cant avcodec_alloc_context3\n");
        goto _Close_Codec_Context;
    }

//    if (avcodec_parameters_to_context(codecCtx, videoStream->codecpar)<0) {
//        av_log(NULL, AV_LOG_ERROR, "cant avcodec_parameters_to_context\n");
//        goto _Close_Codec_Context;
//    }
    
    codec = avcodec_find_decoder(codecCtx->codec_id);
    
    if (!codec) {
        av_log(NULL, AV_LOG_ERROR, "cant find decoder\n");
        goto _Close_Input_Format;
    }
    
    int screen_width=videoStream->codecpar->width;
    
    int screen_height=videoStream->codecpar->height;
    
    delayTime=1000/(videoStream->avg_frame_rate.num/videoStream->avg_frame_rate.den);
    
    av_log(NULL, AV_LOG_ERROR, "fps = %d\n",delayTime);
    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "cant avcodec_open2\n");
        goto _Close_Codec_Context;
    }

    frame = av_frame_alloc();

    if (!frame) {
        av_log(NULL, AV_LOG_ERROR, "cant av_frame_alloc\n");
        goto _Close_Codec_Context;
    }
    
//    av_dump_format(inCtx, 0, NULL ,0);

     av_log(NULL, AV_LOG_ERROR, "%d",codecCtx->pix_fmt);
    
    imgSwsContext=sws_getContext(screen_width, screen_height,AV_PIX_FMT_YUV420P10LE, screen_width/2, screen_height/2, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    
   
    
    if (!imgSwsContext) {
        av_log(NULL, AV_LOG_ERROR, "cant sws_getContext %s\n",SDL_GetError());
        goto _Close_Codec_Context;
    }
    
    screen_width=screen_width/2;
    screen_height=screen_height/2;
    
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);

    SDL_Window* window = NULL;
    SDL_Renderer* render = NULL;
    SDL_Texture* texture = NULL;
    window = SDL_CreateWindow(fileName,SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, SDL_WINDOW_OPENGL);

    if (!window) {
        av_log(NULL, AV_LOG_ERROR, "cant SDL_CreateWindow %s\n",SDL_GetError());
        goto _Close_Codec_Context;
    }
    render = SDL_CreateRenderer(window, -1, 0);

    
    if (!render) {
        av_log(NULL, AV_LOG_ERROR, "cant SDL_CreateRenderer\n");
        goto _Close_Codec_Context;
    }
    
    
    
    texture = SDL_CreateTexture(render, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, screen_width, screen_height);
    
    if (!texture) {
        av_log(NULL, AV_LOG_ERROR, "cant SDL_CreateTexture %s\n",SDL_GetError());
        goto _Close_Codec_Context;
    }

    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = screen_width;
    rect.h =screen_height;
    int got_frame;
    int len;
    
    av_init_packet(&pkt);
    
    
    AVFrame* pFrameRgb = av_frame_alloc();
    
    uint8_t* buffer;
    int numBytes=avpicture_get_size(AV_PIX_FMT_YUV420P, screen_width, screen_height);
//
    buffer=(uint8_t*)av_malloc(sizeof(uint8_t)*numBytes);

    avpicture_fill((AVPicture*)pFrameRgb, buffer, AV_PIX_FMT_YUV420P, screen_width, screen_height);

  
    
    while (av_read_frame(inCtx, &pkt)>=0) {
        if (pkt.stream_index == stream_index) {
           len= avcodec_decode_video2(codecCtx, frame,&got_frame, &pkt);
            if (len<0) {
                av_log(NULL, AV_LOG_ERROR, "avcodec_decode_video2 failed %s\n",av_err2str(len));
                break;
            }
            
            
            if (got_frame) {
             sws_scale(imgSwsContext, (const unsigned char* const*) frame->data, frame->linesize, 0,codecCtx->height, pFrameRgb->data, pFrameRgb->linesize);
            
//                SDL_UpdateTexture(texture, NULL, pFrameRgb->data[0], pFrameRgb->linesize[0]);
                
                SDL_UpdateYUVTexture(texture, &rect, pFrameRgb->data[0], pFrameRgb->linesize[0], pFrameRgb->data[1], pFrameRgb->linesize[1], pFrameRgb->data[2], pFrameRgb->linesize[2]);
#if OUTPUT_YUV420P
                int y_size=codecCtx->width*codecCtx->height;
                fwrite(frame->data[0],1,y_size,fp_yuv);    //Y
                fwrite(frame->data[1],1,y_size/4,fp_yuv);  //U
                fwrite(frame->data[2],1,y_size/4,fp_yuv);  //V
#endif
                
                
                SDL_RenderClear(render);
                SDL_RenderCopy(render, texture, NULL, &rect);
                
                SDL_RenderPresent(render);
                
                SDL_Delay(delayTime);
            }
        
        }
        
        av_packet_unref(&pkt);
    }
    

    while (1) {
        ret = avcodec_decode_video2(codecCtx, frame, &got_frame, &pkt);
        if (ret < 0)
            break;
        if (!got_frame)
            break;
        sws_scale(imgSwsContext, (const unsigned char* const*)frame->data, frame->linesize, 0, codecCtx->height,
                  pFrameRgb->data, pFrameRgb->linesize);
#if OUTPUT_YUV420P
        int y_size=codecCtx->width*codecCtx->height;
        fwrite(frame->data[0],1,y_size,fp_yuv);    //Y
        fwrite(frame->data[1],1,y_size/4,fp_yuv);  //U
        fwrite(frame->data[2],1,y_size/4,fp_yuv);  //V
#endif
        //SDL---------------------------
        SDL_UpdateTexture( texture, &rect, pFrameRgb->data[0], pFrameRgb->linesize[0] );
        SDL_RenderClear( render );
        SDL_RenderCopy( render, texture,  NULL, &rect);
        SDL_RenderPresent( render );
        //SDL End-----------------------
        //Delay 40ms
        SDL_Delay(delayTime);
        
    }
    
    
    sws_freeContext(imgSwsContext);
#if OUTPUT_YUV420P
    fclose(fp_yuv);
#endif
_Close_Codec_Context:
    avcodec_close(codecCtx);
_Close_Input_Format:
    if (inCtx) {
        avformat_close_input(&inCtx);
    }
    return 0;
}
