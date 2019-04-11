//
//  extra_audio.c
//  demo
//
//  Created by Apple on 2019/3/16.
//  Copyright © 2019年 Apple. All rights reserved.
//

#include <stdio.h>
#include <libavformat/avformat.h>

#define ADTS_HEADER_LEN  7;

void adts_header(char *szAdtsHeader, int dataLen){
    
    int audio_object_type = 2;
    int sampling_frequency_index = 7;
    int channel_config = 2;
    
    int adtsLen = dataLen + 7;
    
    szAdtsHeader[0] = 0xff;         //syncword:0xfff                          高8bits
    szAdtsHeader[1] = 0xf0;         //syncword:0xfff                          低4bits
    szAdtsHeader[1] |= (0 << 3);    //MPEG Version:0 for MPEG-4,1 for MPEG-2  1bit
    szAdtsHeader[1] |= (0 << 1);    //Layer:0                                 2bits
    szAdtsHeader[1] |= 1;           //protection absent:1                     1bit
    
    szAdtsHeader[2] = (audio_object_type - 1)<<6;            //profile:audio_object_type - 1                      2bits
    szAdtsHeader[2] |= (sampling_frequency_index & 0x0f)<<2; //sampling frequency index:sampling_frequency_index  4bits
    szAdtsHeader[2] |= (0 << 1);                             //private bit:0                                      1bit
    szAdtsHeader[2] |= (channel_config & 0x04)>>2;           //channel configuration:channel_config               高1bit
    
    szAdtsHeader[3] = (channel_config & 0x03)<<6;     //channel configuration:channel_config      低2bits
    szAdtsHeader[3] |= (0 << 5);                      //original：0                               1bit
    szAdtsHeader[3] |= (0 << 4);                      //home：0                                   1bit
    szAdtsHeader[3] |= (0 << 3);                      //copyright id bit：0                       1bit
    szAdtsHeader[3] |= (0 << 2);                      //copyright id start：0                     1bit
    szAdtsHeader[3] |= ((adtsLen & 0x1800) >> 11);           //frame length：value   高2bits
    
    szAdtsHeader[4] = (uint8_t)((adtsLen & 0x7f8) >> 3);     //frame length:value    中间8bits
    szAdtsHeader[5] = (uint8_t)((adtsLen & 0x7) << 5);       //frame length:value    低3bits
    szAdtsHeader[5] |= 0x1f;                                 //buffer fullness:0x7ff 高5bits
    szAdtsHeader[6] = 0xfc;
}

int extra_audio(char *srcpath,char * dstAudioPath){
//     上下文
    AVFormatContext *formatContext=NULL;
//    读取到的数据包结构体
    AVPacket packet;
    
    int ret=0;
    
    ret=avformat_open_input(&formatContext, srcpath, NULL, NULL);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "cant init avformatcontext %s\n",av_err2str(ret));
    
           goto __failed;
        
    }
    
    ret=av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    int audioStreamIndex=ret;
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "cant find best stream %s\n",av_err2str(ret));
        
        goto __failed;
    }
    
//    int stream_index=ret;
    av_init_packet(&packet);
    
    FILE *dstFile=fopen(dstAudioPath, "wb");
    
    while (av_read_frame(formatContext, &packet)>=0) {
        
        if (packet.stream_index==audioStreamIndex) {
            
            char adts_header_buf[7];
            adts_header(adts_header_buf,packet.size);
            fwrite(adts_header_buf, 1, 7, dstFile);
            int len=fwrite(packet.data,1, packet.size, dstFile);
            
            if (len!=packet.size) {
                av_log(NULL, AV_LOG_WARNING, "write len not equal packet.size\n");
            }
        }
        
        av_packet_unref(&packet);
        
    }
    
    
    fclose(dstFile);
    
    avformat_close_input(&formatContext);
    
    return ret;
__failed:
    if (formatContext!=NULL) {
        avformat_close_input(&formatContext);
    }
    
    if (dstFile!=NULL) {
        fclose(dstFile);
    }
    
    return -1;
    
}

//虽然输出了aac文件 但是播放不出原来视频的声音
//int main(int argc,char * argv[]){
//    extra_audio("/Users/apple/Desktop/MV/陈奕迅 - 龙舌兰.mp4", "./陈奕迅 - 龙舌兰.aac");
//}
