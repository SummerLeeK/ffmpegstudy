//
//  ffmpeg_mediainfo.c
//  demo
//
//  Created by Apple on 2019/3/16.
//  Copyright © 2019年 Apple. All rights reserved.
//  打印音视频文件的基本信息
//

#include <stdio.h>
#include <libavformat/avformat.h>

int print_media_info(char *srcpath){

    AVFormatContext *formatContext=NULL;
    int ret=0;
    
    //    初始化上下文 第一个参数是需要初始化的的上下文对象 第二个是传入的文件路径 第三个是文件格式 第四个是附加的一些命令行选项默认null;
    ret=avformat_open_input(&formatContext, srcpath, NULL, NULL);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "cant init AVFormatContext %s\n",av_err2str(ret));
        return ret;
    }
    
//     打印音视频的meta基本信息 第一个参数是format上下文 第二个是音频文件中流的索引值，第三个参数是文件地址 如果这个文件地址和之前初始化的音视频地址相同可传入NULL 否则dump出的信息就是以当前传入的文件为准 第四个表示是输入文件0还是输出文件1；
    av_dump_format(formatContext, 1, "/Users/apple/Desktop/MV/陈奕迅 - 龙舌兰.mp4", 0);
    
//    结束后关闭上下文
    avformat_close_input(&formatContext);

    return 0;
}

//int main(int argc,char *argv[]){
//    
//    print_media_info("/Users/apple/Desktop/FFmpeg音视频核心技术精讲与实战/第5章 FFmpeg多媒体文件处理/5-7 ffmpeg打印音视频Meta信息.mp4");
////    println("aaa");
//    return 0;
//}
