//
//  decode_video.c
//  demo
//
//  Created by Apple on 2019/4/2.
//  Copyright © 2019年 Apple. All rights reserved.
// 将视频转成图片

#include <stdio.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#define INBUF_SIZE 4096

#define WORD uint16_t
#define DWORD uint32_t
#define LONG int32_t

#pragma pack(2)
typedef struct tagBITMAPFILEHEADER {
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;
void saveBMP(struct SwsContext* imgSwsCtx, AVFrame* frame, char* filename)
{
    
    //输出文件的宽高相当于photoshop中的图层的大小  imgSwsCtx设置的输出图像的宽高相当于真实内容的大小
    int width = frame->width/10;
    int height = frame->height/10;
    
    //计算图片所需要的缓存空间
    int numBytes = avpicture_get_size(AV_PIX_FMT_BGR24, width, height);

    uint8_t* buffer = (uint8_t*)av_malloc(sizeof(uint8_t) * numBytes);

    AVFrame* pFrameRgb = av_frame_alloc();
//  将pFrameRgb结构内的data指向了buffer的地址 相当于为pFrameRgb开辟了空间
    avpicture_fill((AVPicture*)pFrameRgb, buffer, AV_PIX_FMT_BGR24, width, height);
//    http://guguclock.blogspot.com/2009/12/ffmpeg-swscale.html
//    第一个参数即是由 sws_getContext 所取得的参数。
//    第二个 src 及第六个 dst 分別指向input 和 output 的 buffer。
//    第三个 srcStride 及第七 个 dstStride 分別指向 input 及 output 的 stride；如果不知道什么是 stride，姑且可以先把它看成是每一列的 byte 数。
//    第四个srcSliceY，就注解的意思来看，是指第一列要处理的位置；这里我是从头处理，所以直接填0。想知道更详细说明的人，可以参考 swscale.h 的注解。
//    第五个srcSliceH指的是 source slice 的高度。
    //简单来讲 就是将源frame的数据复制给输出的data;
    sws_scale(imgSwsCtx, frame->data, frame->linesize, 0, frame->height, pFrameRgb->data, pFrameRgb->linesize);

    BITMAPINFOHEADER infoHeader;
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = width;
    infoHeader.biHeight = height * -1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = 0;
    infoHeader.biSizeImage = 0;
    infoHeader.biClrImportant = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biPlanes = 1;

    BITMAPFILEHEADER fileHeader = {
        0,
    };

    DWORD dwTotalWriten = 0;
    DWORD dwWriten;

    fileHeader.bfType = 0x4d42; //'BM';
    fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + numBytes;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    FILE* pf = fopen(filename, "wb");
    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, pf);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, pf);
    fwrite(pFrameRgb->data[0], 1, numBytes, pf);
    fclose(pf);

    //释放资源
    //av_free(buffer);
    av_freep(&pFrameRgb[0]);
    av_free(pFrameRgb);
}

int decode_write_frame(char* outfilename, AVCodecContext* codecCtx, struct SwsContext* imgSwsCtx, AVFrame* frame, int* frame_count, AVPacket* pkt, int flag)
{

    int len;
    //是否读取到帧数据的标志判断
    int got_frame;
    char buf[1024];

    // 解码出frame并返回读取的大小len
    len = avcodec_decode_video2(codecCtx, frame, &got_frame, pkt);

    if (len < 0) {
        av_log(NULL, AV_LOG_ERROR, "avcodec_decode_video2 failed\n");
        return -1;
    }

    if (got_frame) {

        if (*frame_count % 24 == 0) {

            fflush(stdout);

            //按照第三个参数的格式 生成字符串赋值给第一个参数
            snprintf(buf, sizeof(buf), "%s_%d.bmp", outfilename, *frame_count);

            av_log(NULL, AV_LOG_ERROR,"%s\n",buf);
            saveBMP(imgSwsCtx, frame, buf);

            (*frame_count)++;
        } else {
            (*frame_count)++;
        }

        if (*frame_count > 240) {
            return -1;
        }
    }

    // 读取的游标向前移动一个frame的大小
    if (pkt->data) {
        pkt->size -= len;
        pkt->data += len;
    }

    return 0;
}

int mains(int argc, char* argv[])
{
    //输入文件
    AVFormatContext* inCtx = NULL;
    //源文件的编码信息
    AVCodec* codec;
    //主要作用是从得到的AVPacket中解码出AVFrame的数据
    AVCodecContext* codecCtx = NULL;
    //获取到的数据流（音频或者视频）
    AVStream* stream = NULL;
    //数据包
    AVPacket pkt;
    //帧数据
    AVFrame* frame;
    
    struct SwsContext* imgSwsCtx;
    int ret = 0;
    int stream_index;
    char* fileName = "/Users/apple/Desktop/MV/周深 - 大鱼.mp4";
    char* outfilename = "/Users/apple/Desktop/MV/大鱼/周深 - 大鱼";
    int frame_out;
    ret = avformat_open_input(&inCtx, fileName, NULL, NULL);

    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "avformat_open_input\n");
        return -1;
    }

    av_dump_format(inCtx, -1, fileName, 0);
    //寻找文件中的流 判断文件中是否有视频流等
    ret = avformat_find_stream_info(inCtx, NULL);

    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "avformat_find_stream_info\n");
        return -1;
    }

    ret = av_find_best_stream(inCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "cant find video stream \n");
        return -1;
    }
    stream_index = ret;

    stream = inCtx->streams[ret];

    //寻找解码器
    codec = avcodec_find_decoder(stream->codecpar->codec_id);

    if (!codec) {
        av_log(NULL, AV_LOG_ERROR, "cant find decoder\n");
        return -1;
    }

    //初始化上下文
    codecCtx = avcodec_alloc_context3(codec);

    if (!codecCtx) {
        av_log(NULL, AV_LOG_ERROR, "cant alloc avcodec\n");
        return -1;
    }

    //复制流中的参数给codecCtx
    if ((ret = avcodec_parameters_to_context(codecCtx, stream->codecpar)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "cant avcodec_parameters_to_context\n");
        return -1;
    }

    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "cant open avcodec\n");
        return -1;
    }

    //初始化图像处理 前三个参数表示输入的宽高和编码方式 后三个参数表示输出图像的宽高和解码方式
    imgSwsCtx = sws_getContext(codecCtx->width, codecCtx->height, codecCtx->pix_fmt, codecCtx->width/2, codecCtx->height/2, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);

    if (!imgSwsCtx) {
        av_log(NULL, AV_LOG_ERROR, "sws_getContext failed\n");
        return -1;
    }

    frame = av_frame_alloc();

    if (!frame) {
        av_log(NULL, AV_LOG_ERROR, "av_frame_alloc failed\n");
        return -1;
    }
    
    av_init_packet(&pkt);
    pkt.data=NULL;
    pkt.size=0;
    
    while (av_read_frame(inCtx, &pkt) >= 0) {

        if (pkt.stream_index == stream_index) {
            if (decode_write_frame(outfilename, codecCtx, imgSwsCtx, frame, &frame_out, &pkt, 0) < 0) {

                return -1;
            }
        }
       
        if (pkt.buf) {
            av_packet_unref(&pkt);
        }
     
        
    }

    pkt.data = NULL;
    pkt.size = 0;

    decode_write_frame(outfilename, codecCtx, imgSwsCtx, frame, &frame_out, &pkt, 1);

    avformat_close_input(&inCtx);
    
    sws_freeContext(imgSwsCtx);
    
    av_frame_free(&frame);
    
    return 0;
}
