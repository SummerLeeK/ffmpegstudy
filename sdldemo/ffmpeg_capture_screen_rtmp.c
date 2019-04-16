//
//  ffmepg_avdevices_demo.c
//  demo
//  获取摄像头并sdl显示
//  参考 https://blog.csdn.net/leixiaohua1020/article/details/39702113
//  rtmp推流
//  Created by Apple on 2019/4/12.
//  Copyright © 2019年 Apple. All rights reserved.
//

#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <SDL2/SDL.h>

#define SAVE_FILE 1
static AVFormatContext* fmtCtx;
static AVInputFormat* inCtx;
static AVCodecContext* codecCtx;
static AVCodec* codec;

static int stream_index;
static int video_width;
static int video_height;

static int screen_width;
static int screen_height;

static SDL_Window* window;
static SDL_Renderer* renderer;
static SDL_Texture* texture;
static SDL_Rect videoRect;
static SDL_Thread* thread;
static int delayTime=40;
static int thread_exit = 0;
static int bit_rate=400000;
static AVCodecContext* encodeCodecCtx;
static AVOutputFormat* outputfmt;
static AVFormatContext *ofmt;
static AVStream *outStream;
static AVFrame* outFrame;
static AVPacket outPkt;


static int init_encoder(char* outfilename)
{
    int ret;
    
    ret=avformat_alloc_output_context2(&ofmt, NULL, "flv", outfilename);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "avformat_alloc_output_context2 failed \n");
        return -1;
    }
    outputfmt=av_guess_format("flv", outfilename, NULL);
    if (!outputfmt) {
        av_log(NULL, AV_LOG_ERROR, "av_guess_format failed \n");
        return -1;
    }
    ofmt->oformat=outputfmt;
    
    
    
    AVCodec* codec = avcodec_find_encoder(outputfmt->video_codec);
    
    if (!codec) {
        av_log(NULL, AV_LOG_ERROR, "avcodec_find_encoder failed \n");
        return -1;
    }
    
    outStream=avformat_new_stream(ofmt, codec);
    
    if (!outStream) {
        av_log(NULL, AV_LOG_ERROR, "avformat_new_stream failed \n");
        return -1;
    }
    
    
    encodeCodecCtx = avcodec_alloc_context3(codec);
    
    encodeCodecCtx->bit_rate = bit_rate;
    encodeCodecCtx->width = screen_width;
    encodeCodecCtx->height = screen_height;
    
    encodeCodecCtx->time_base.den=60;
    encodeCodecCtx->time_base.num=10;
    encodeCodecCtx->framerate.den=1;
    encodeCodecCtx->framerate.num=60;
    
    encodeCodecCtx->gop_size = 100;
    
    encodeCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    
    //    encodeCodecCtx->max_b_frames = 3;
    encodeCodecCtx->mb_lmin=1;
    encodeCodecCtx->mb_lmax=50;
    
    //最大和最小量化系数
    encodeCodecCtx->qmin = 10;
    encodeCodecCtx->qmax = 51;
    encodeCodecCtx->qblur=0.0;
    
    av_dump_format(ofmt, 0, outfilename, 1);
    
    if (encodeCodecCtx->codec_id == AV_CODEC_ID_H264) {
        av_opt_set(encodeCodecCtx->priv_data, "preset", "slow", 0);
    }
    
    if (avcodec_open2(encodeCodecCtx, codec, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "avcodec_open2 failed \n");
        return -1;
    }
    
    ret=avcodec_parameters_from_context(outStream->codecpar, encodeCodecCtx);
    
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "avcodec_parameters_from_context failed %s\n",av_err2str(ret));
        return -1;
    }
    //    outStream->x
    outStream->time_base=encodeCodecCtx->time_base;
    ret=avio_open(&ofmt->pb, outfilename, AVIO_FLAG_READ_WRITE);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "avio_open failed \n");
        return -1;
    }
    
    
    ret= avformat_write_header(ofmt, NULL);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "avformat_write_header failed %s\n",av_err2str(ret));
        return -1;
    }
    
    av_init_packet(&outPkt);
    
    return 0;
}
static int pts = 0;
static void encodeFrame(FILE* dstFile, AVFrame* frame)
{
    int ret;
    //设置一些属性 这些属性不设置会在控制台中打印出来
    frame->format=AV_PIX_FMT_YUV420P;
    frame->width=screen_width;
    frame->height=screen_height;
    //向encodeCodecCtx发送一帧数据
    pts++;
    frame->pts = pts;
    
    ret = avcodec_send_frame(encodeCodecCtx, frame);
    
    if (ret < 0) {
        
        av_log(NULL, AV_LOG_ERROR, "avcodec_send_frame failed %s\n",av_err2str(ret));
        return;
    }
    
    while (ret >= 0) {
        //接受数据。一个packet中可能包含多个frame
        ret = avcodec_receive_packet(encodeCodecCtx, &outPkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            //走到这忽略即可 packet可能还没填满
            //            av_log(NULL, AV_LOG_ERROR, "avcodec_receive_packet EAGAIN  AVERROR_EOF  %s\n", av_err2str(ret));
            return;
        } else if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "avcodec_receive_packet failed %s\n", av_err2str(ret));
            return;
        }
        
        // 计算packet的pts
        //        av_packet_rescale_ts(&outPkt,
        //                            codecCtx->time_base,
        //                             outStream->time_base);
        
        outPkt.pts=av_rescale_q_rnd(outPkt.pts, codecCtx->time_base, outStream->time_base, AV_ROUND_PASS_MINMAX|AV_ROUND_NEAR_INF);
        
        outPkt.dts=outPkt.pts;
        
        outPkt.duration=av_rescale_q(outPkt.duration, codecCtx->time_base, outStream->time_base);
        
        av_interleaved_write_frame(ofmt, &outPkt);
        //        fwrite(outPkt.data, 1, outPkt.size, dstFile);
        
        av_packet_unref(&outPkt);
    }
}
#define RETRESH_EVENT (SDL_USEREVENT + 1)
#define BREAK_EVENT (SDL_USEREVENT + 2)
static void refresh_thread(void* data)
{
    
    while(!thread_exit) {
        SDL_UserEvent event;
        event.type = RETRESH_EVENT;
        SDL_PushEvent(&event);
        SDL_Delay(delayTime);
    }
}

static int init_sdl_player()
{
    
    video_width = codecCtx->width;
    video_height = codecCtx->height;
    
    screen_width = video_width;
    screen_height = video_height;
    
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
    
    window = SDL_CreateWindow(fmtCtx->filename, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, SDL_WINDOW_OPENGL);
    
    if (!window) {
        av_log(NULL, AV_LOG_ERROR, "cant create window %s \n", SDL_GetError());
        return -1;
    }
    
    renderer = SDL_CreateRenderer(window, -1, 0);
    
    if (!renderer) {
        av_log(NULL, AV_LOG_ERROR, "cant create renderer %s \n", SDL_GetError());
        return -1;
    }
    
    videoRect.x = 0;
    videoRect.y = 0;
    videoRect.w = screen_width;
    videoRect.h = screen_height;
    
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, screen_width, screen_height);
    
    if (!texture) {
        av_log(NULL, AV_LOG_ERROR, "cant create texture %s \n", SDL_GetError());
        return -1;
    }
    
    thread = SDL_CreateThread(refresh_thread, NULL, NULL);
    
    if (!thread) {
        av_log(NULL, AV_LOG_ERROR, "cant create thread %s \n", SDL_GetError());
        return -1;
    }
    
    return 0;
}

static void freeSDL()
{
    SDL_Quit();
}

//Show AVFoundation Device
//打印macos上的音频视频输入设备名及信息
static void show_avfoundation_device()
{
    AVFormatContext* pFormatCtx = avformat_alloc_context();
    AVDictionary* options = NULL;
    av_dict_set(&options, "framerate", "30", 0);
    av_dict_set(&options, "video_size", "1280x720", 0);
    av_dict_set(&options, "list_devices", "true", 0);
    av_dict_set(&options, "list_options", "true", 0);
    AVInputFormat* iformat = av_find_input_format("avfoundation");
    printf("==AVFoundation Device Info===\n");
    avformat_open_input(&pFormatCtx, "", iformat, &options);
    printf("=============================\n");
}

int main_rtmp(int argc, char** argv)
{
    
    int ret;
    avformat_network_init();
    //这句必加 我还以为所有的register_all都被废弃了
    avdevice_register_all();
    //    show_avfoundation_device();
    AVDictionary* options = NULL;
    //  寻找Macos上的摄像头设备
    inCtx = av_find_input_format("avfoundation");
    av_dict_set(&options, "framerate", "60", 0);
    av_dict_set(&options, "pixel_format", "uyvy422", 0);
    
    // Mac上设置无效
    //    av_dict_set(&options, "offset_x", "100", 0);
    //    av_dict_set(&options, "offset_y", "100", 0);
    //    av_dict_set(&options, "title", "Xcode", 0);
    //在macOS上 如果是录制屏幕的状态那么video_size设置是无效的
    //最后生成的图像宽高是屏幕的宽高
    av_dict_set(&options, "video_size", "2560x1600", 0);
    if (!inCtx) {
        av_log(NULL, AV_LOG_ERROR, "av_find_input_format failed \n");
        return -1;
    }
    
    char* outFile = "rtmp://192.168.13.108:1935/zbcs/test6";
    FILE* dstFile;
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };
    // 0=获取摄像头 1=录制屏幕
    ret = avformat_open_input(&fmtCtx, "1", inCtx, &options);
    
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "avformat_open_input failed %s\n", av_err2str(ret));
        return ret;
    }
    
    ret = avformat_find_stream_info(fmtCtx, NULL);
    
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "");
        return ret;
    }
    
    av_dump_format(fmtCtx, 0, NULL, 0);
    
    ret = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, 0, NULL, 0);
    
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "av_find_best_stream failed %s\n", av_err2str(ret));
        return ret;
    }
    
    stream_index = ret;
    
    codecCtx = avcodec_alloc_context3(NULL);
    
    ret = avcodec_parameters_to_context(codecCtx, fmtCtx->streams[stream_index]->codecpar);
    
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "avcodec_parameters_to_context failed %s \n", av_err2str(ret));
        return ret;
    }
    //设置帧率
    codecCtx->framerate=(AVRational){60,1};
    codecCtx->time_base=(AVRational){1,60};
    codec = avcodec_find_decoder(codecCtx->codec_id);
    
    if (!codec) {
        av_log(NULL, AV_LOG_ERROR, "avcodec_find_decoder failed %d", codecCtx->codec_id);
        return -1;
    }
    
    ret = avcodec_open2(codecCtx, codec, &options);
    
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "avcodec_open2 failed %s", av_err2str(ret));
        return ret;
    }
    
    delayTime=1000/(codecCtx->framerate.num);
    
    ret = init_sdl_player();
    
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "init_sdl_player failed \n");
        return -1;
    }
    
#if SAVE_FILE
    //    dstFile = fopen(outFile, "wb+");
    
    ret=init_encoder(outFile);
    if (ret<0) {
        return -1;
    }
#endif
    
    AVFrame *frame, *pFrameYuv;
    
    frame = av_frame_alloc();
    
    pFrameYuv = av_frame_alloc();
    
    int numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, screen_width, screen_width);
    uint8_t* buffer = (uint8_t*)av_malloc(sizeof(uint8_t) * numBytes);
    
    avpicture_fill(pFrameYuv->data, buffer, AV_PIX_FMT_YUV420P, screen_width, screen_height);
    
    //    AV_PIX_FMT_YUVJ420P
    
    AVPacket pkt;
    
    struct SwsContext* imageSwsContext;
    
    av_init_packet(&pkt);
    
    av_log(NULL, AV_LOG_ERROR, "%d\t%d\t%d\t%d\n", video_width, video_height, screen_width, screen_height);
    int framecount = 0;
    
    SDL_Event event;
    while (!thread_exit){
        SDL_WaitEvent(&event);
        if (event.type == RETRESH_EVENT) {
            while (1) {
                if (av_read_frame(fmtCtx, &pkt)<0) {
                    thread_exit=1;
                }
                
                if (pkt.stream_index==stream_index) {
                    break;
                }
            }
            
            ret = avcodec_send_packet(codecCtx, &pkt);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "avcodec_send_packet failed %s \n", av_err2str(ret));
                break;
            }
            
            while (avcodec_receive_frame(codecCtx, frame) >= 0) {
                
                if (!imageSwsContext) {
                    imageSwsContext = sws_getCachedContext(imageSwsContext, video_width, video_height, frame->format, screen_width, screen_height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
                }
                
                int result = sws_scale(imageSwsContext, frame->data, frame->linesize, 0, video_height, pFrameYuv->data, pFrameYuv->linesize);
#if SAVE_FILE
                
                
                encodeFrame(dstFile, pFrameYuv);
                
                
#endif
                
                SDL_UpdateYUVTexture(texture, &videoRect, pFrameYuv->data[0], pFrameYuv->linesize[0], pFrameYuv->data[1], pFrameYuv->linesize[1], pFrameYuv->data[2], pFrameYuv->linesize[2]);
                
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                SDL_RenderPresent(renderer);
                
            }
            av_packet_unref(&pkt);
        }else if(event.type==SDL_QUIT)
        {
            thread_exit=1;
        }
    }
    
#if SAVE_FILE
    av_write_trailer(ofmt);
    //    fwrite(endcode, 1, sizeof(endcode), dstFile);
    //    fclose(dstFile);
#endif
    
    freeSDL();
    
    avformat_close_input(&fmtCtx);
    
    
    
    return 0;
}
