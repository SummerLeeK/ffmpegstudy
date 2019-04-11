//
//  ffmpeg_extra_video.c
//  demo
//
//  Created by Apple on 2019/3/17.
//  Copyright © 2019年 Apple. All rights reserved.
//

#include <stdio.h>
#include <libavformat/avformat.h>


int ffmpeg_extra_for_media_type(char *srcpath,char *dstpath,enum AVMediaType type){
    
    AVFormatContext *inFmtCtx=NULL;
    
    AVFormatContext *outFmtCtx=NULL;
    
    AVOutputFormat *outFmt=NULL;
    
    int stream_index;
    
    AVStream *inStream;
    AVStream *outStream;
    
    AVPacket packet;
    
    int ret=0;
    ret=avformat_open_input(&inFmtCtx, srcpath, NULL, NULL);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "cant open input format %s \n",av_err2str(ret));
        
        goto __failed;
    }
    
    
    outFmtCtx=avformat_alloc_context();
    
    outFmt=av_guess_format(NULL, dstpath, NULL);
    
    if (outFmt==NULL) {
        av_log(NULL, AV_LOG_ERROR, "cant guess output format \n");
        
        goto __failed;
    }
    
    outFmtCtx->oformat=outFmt;
    
    ret=av_find_best_stream(inFmtCtx, type, -1, -1, NULL, 0);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "cant find best stream %s \n",av_err2str(ret));
        
        goto __failed;
    }
    
    stream_index=ret;
    
    inStream=inFmtCtx->streams[stream_index];
    
    outStream=avformat_new_stream(outFmtCtx, NULL);
    
    if (outStream==NULL) {
        av_log(NULL, AV_LOG_ERROR, "cant create new stream \n");
        
        goto __failed;
    }
    
    ret=avcodec_parameters_copy( outStream->codecpar,inStream->codecpar);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "cant copy params %s \n",av_err2str(ret));
        
        goto __failed;
    }
    
    outStream->codecpar->codec_tag=0;
    
    ret=avio_open(&outFmtCtx->pb, dstpath, AVIO_FLAG_WRITE);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "cant open output file %s \n",av_err2str(ret));
        
        goto __failed;
    }
    ret=avformat_write_header(outFmtCtx, NULL);
    
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "cant write header  %s \n",av_err2str(ret));
        
        goto __failed;
    }
    
    
    av_init_packet(&packet);
    packet.data=NULL;
    packet.size=0;
    
    while (av_read_frame(inFmtCtx, &packet)>=0) {
        if (packet.stream_index==stream_index) {
//            将时间基等一些参数转换
            packet.pts=av_rescale_q_rnd(packet.pts, inStream->time_base, outStream->time_base, AV_ROUND_PASS_MINMAX|AV_ROUND_NEAR_INF);
            
            packet.dts=packet.pts;
            
            packet.duration=av_rescale_q(packet.duration, inStream->time_base, outStream->time_base);
            
            packet.pos=-1;
            packet.stream_index=0;
            
            //            将转换好的数据包写入
            av_interleaved_write_frame(outFmtCtx, &packet);
            
            //            释放数据包
            av_packet_unref(&packet);
        }
    }
    
    //    写入尾巴
    av_write_trailer(outFmtCtx);
    //    结束后关闭
    avformat_close_input(&inFmtCtx);
    
    avio_close(outFmtCtx->pb);
    
    return 0;
__failed:
    
    return -1;
    
}

/**
 * 分离视频 会导致花屏
 */
//int main(int argc,char *argv[]){
//
//     ffmpeg_extra_for_media_type("/Users/apple/Desktop/MV/陈奕迅 - 龙舌兰.mp4", "./陈奕迅 - 龙舌兰1.flv",AVMEDIA_TYPE_VIDEO);
//
//    return 0;
//}
