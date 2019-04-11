//
//  video2img.c
//  demo
//
//  Created by Apple on 2019/4/1.
//  Copyright © 2019年 Apple. All rights reserved.
//  生成一秒的h264文件 不是视频转图片的例子
//

#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
int amain(int argc, char* argv[])
{

    const char* filename;

    const char* codec_name;

    const AVCodec* codec;
    AVCodecContext* codeCtx = NULL;

    int i, ret, x, y, go_output;

    FILE* f;
    
//    go_output=1;

    AVFrame* frame;
    AVPacket pkt;
    uint8_t encode[] = { 0, 1, 1, 0xb7 };
    //输出文件地址
    filename = "/Users/apple/Desktop/MV/1.h264";
    //编码器
    codec_name = "libx264";

    avcodec_register_all();
    //根据名字寻找编码器
    codec = avcodec_find_encoder_by_name(codec_name);

    if (!codec) {
        av_log(NULL, AV_LOG_ERROR, "avcodec_find_encoder_by_name error\n");
        return -1;
    }
    //初始化编码器上下文
    codeCtx = avcodec_alloc_context3(codec);

    if (!codeCtx) {

        av_log(NULL, AV_LOG_ERROR, "avcodec_alloc error\n");

        return -1;
    }

    codeCtx->bit_rate = 400000;
    //视频宽高
    codeCtx->width = 1920;
    codeCtx->height = 1080;

    codeCtx->time_base = (AVRational){ 1, 25 };
    codeCtx->framerate = (AVRational){ 25, 1 };

    //表示每gop_size帧为一组，一组中有一个关键字
    codeCtx->gop_size = 10;
    codeCtx->max_b_frames = 1;
    codeCtx->pix_fmt = AV_PIX_FMT_YUV420P;

    if (codeCtx->codec_id == AV_CODEC_ID_H264) {
        av_opt_set(codeCtx->priv_data, "preset", "slow", 0);
    }

    if (avcodec_open2(codeCtx, codec, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "cant open avcodec\n");
        return -1;
    }

    f = fopen(filename, "wb");

    if (!f) {
        av_log(NULL, AV_LOG_ERROR, "cant open file\n");
        return -1;
    }

    frame = av_frame_alloc();

    if (!frame) {
        av_log(NULL, AV_LOG_ERROR, "cant alloc frame\n");
        return -1;
    }

    frame->width = codeCtx->width;
    frame->height = codeCtx->height;
    frame->format = codeCtx->pix_fmt;

    ret = av_frame_get_buffer(frame, 32);

    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "could get buffer in frame\n");
        return -1;
    }

    for (int i = 0; i < 25; i++) {
        av_init_packet(&pkt);
        pkt.data = NULL;
        pkt.size = 0;

        fflush(stdout);

        ret = av_frame_make_writable(frame);

        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "cant make sure frame is writeable");
            return -1;
        }

        for (y = 0; y < codeCtx->height; y++) {
            for (x = 0; x < codeCtx->width; x++) {
                frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
            }
        }

        for (y = 0; y < codeCtx->height / 2; y++) {
            for (x = 0; x < codeCtx->width / 2; x++) {
                frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
                frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
            }
        }

        frame->pts = i;

        ret = avcodec_encode_video2(codeCtx, &pkt, frame, &go_output);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "cant encode video\n");
            return -1;
        }

        if (go_output) {
            av_log(NULL, AV_LOG_INFO, "write %d frame to file\n", i);
            fwrite(pkt.data, 1, pkt.size, f);

            av_packet_unref(&pkt);
        }
    }

    for (go_output = 1; go_output; go_output++) {
        fflush(stdout);

        ret = avcodec_encode_video2(codeCtx, &pkt, frame, &go_output);

        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error encoding frame\n");
            return -1;
        }

        if (go_output) {
            printf("Write frame %3d (size=%5d)\n", i, pkt.size);
            fwrite(pkt.data, 1, pkt.size, f);
            av_packet_unref(&pkt);
        }
    }
    
    fwrite(encode, 1, sizeof(encode), f);
    fclose(f);

    
    avcodec_free_context(&codeCtx);
    av_frame_free(&frame);
    return 0;
}
