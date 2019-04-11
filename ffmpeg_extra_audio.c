//
//  ffmpeg_extra_audio.c
//  demo
//
//  Created by Apple on 2019/3/17.
//  Copyright © 2019年 Apple. All rights reserved.
// 利用ffmpeg的i一些API分离音频数据。
//

#include <stdio.h>
#include <libavformat/avformat.h>


int ffmpeg_extra_audio(char *srcpath,char *dstpath){
    int ret=0;
//    输入文件的上下文
    AVFormatContext *formatContext=NULL;
//    输出文件的上下文
    AVFormatContext *outputFormatContext=NULL;
    AVOutputFormat *outFormat=NULL;
//    输入文件的音频流
    AVStream *inStream=NULL;
//    输出文件的音频流
    AVStream *audioStream=NULL;
//    数据包
    AVPacket packet;
//    初始化输入上下文
    ret = avformat_open_input(&formatContext, srcpath, NULL, NULL);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "cant init formatcontext  %s\n",av_err2str(ret));
        goto __failed;
    }
//    初始化输出文件的上下文
    outputFormatContext=avformat_alloc_context();
//    通过输出的文件后缀名 猜测文件格式
   outFormat= av_guess_format(NULL, dstpath, NULL);
    
    if (outFormat==NULL) {
        av_log(NULL, AV_LOG_ERROR, "cant guest format \n");
             goto __failed;
    }
    
//    将猜测出的文件格式 赋值给输出文件上下文
    outputFormatContext->oformat=outFormat;
    
//    对输出上下文 新建一个流
    audioStream=avformat_new_stream(outputFormatContext,NULL);
    
    


    if (audioStream==NULL) {
        av_log(NULL, AV_LOG_ERROR, "cant new stream \n");
             goto __failed;
    }
    
    
//    找到输入文件最适合的那个流的索引值
    ret=av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "cant find best stream %s\n",av_err2str(ret));
        goto __failed;
    }
    
    int audio_stream_index=ret;
    
    inStream=formatContext->streams[audio_stream_index];
    
//    将输入流的一些参数 赋值给输出流
    ret=avcodec_parameters_copy(audioStream->codecpar,inStream->codecpar);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "cant copy parameters %s \n",av_err2str(ret));
        goto __failed;
    
    }
    
    audioStream->codecpar->codec_tag=0;
    
//    打开输出文件
    ret=avio_open(&outputFormatContext->pb, dstpath, AVIO_FLAG_WRITE);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "cant open output io %s \n",av_err2str(ret));
        goto __failed;
    }
//    初始化数据包 分配空间
    av_init_packet(&packet);
    packet.data=NULL;
    packet.size=0;
    
//将header写入
   ret= avformat_write_header(outputFormatContext, NULL);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "cant write header  %s \n",av_err2str(ret));
        goto __failed;
    }
    
//    读取每一帧的数据
    while (av_read_frame(formatContext, &packet)>=0) {
        if (packet.stream_index==audio_stream_index) {
            
//            将时间基等一些参数转换
            packet.pts=av_rescale_q_rnd(packet.pts, inStream->time_base, audioStream->time_base, AV_ROUND_PASS_MINMAX|AV_ROUND_NEAR_INF);
            
            packet.dts=packet.pts;
            
            packet.duration=av_rescale_q(packet.duration, inStream->time_base, audioStream->time_base);
            
            packet.pos=-1;
            packet.stream_index=0;
            
//            将转换好的数据包写入
            av_interleaved_write_frame(outputFormatContext, &packet);
            
//            释放数据包
            av_packet_unref(&packet);
            
        }
        
    }
    
//    写入尾巴
    av_write_trailer(outputFormatContext);
//    结束后关闭
    avformat_close_input(&formatContext);
    
    avio_close(outputFormatContext->pb);
    
    return ret;
    
    
__failed:
    if (formatContext!=NULL) {
        avformat_close_input(&formatContext);
    }
    
    if (formatContext!=NULL) {
        avformat_close_input(&formatContext);
    }
    
    
    if (outputFormatContext!=NULL) {
        if (outputFormatContext->pb!=NULL) {
             avio_close(outputFormatContext->pb);
        }
    }
    
    return -1;
}

//int main(int argc,char *argv[]){
//
//     ffmpeg_extra_audio("/Users/apple/Desktop/MV/陈奕迅 - 龙舌兰.mp4", "./陈奕迅 - 龙舌兰1.m4a");

//return 0;
//}
