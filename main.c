////
////  main.c
////  demo
////
////  Created by Apple on 2018/12/24.
////  Copyright © 2018年 Apple. All rights reserved.
////
//
//#include <stdio.h>
//#include <libavutil/log.h>
//#include <libavformat/avformat.h>
//#include <libavformat/avio.h>
//#include "second.h"
//struct hello {
//    int a;
//    int b;
//};
//typedef struct hello Hello;
//
//#ifndef AV_WB32
//#   define AV_WB32(p, val) do {                 \
//uint32_t d = (val);                     \
//((uint8_t*)(p))[3] = (d);               \
//((uint8_t*)(p))[2] = (d)>>8;            \
//((uint8_t*)(p))[1] = (d)>>16;           \
//((uint8_t*)(p))[0] = (d)>>24;           \
//} while(0)
//#endif
//
//#ifndef AV_RB16
//#   define AV_RB16(x)                           \
//((((const uint8_t*)(x))[0] << 8) |          \
//((const uint8_t*)(x))[1])
//#endif
//
//static int alloc_and_copy(AVPacket *out,
//                          const uint8_t *sps_pps, uint32_t sps_pps_size,
//                          const uint8_t *in, uint32_t in_size)
//{
//    uint32_t offset         = out->size;
//    uint8_t nal_header_size = offset ? 3 : 4;
//    int err;
//    
//    err = av_grow_packet(out, sps_pps_size + in_size + nal_header_size);
//    if (err < 0)
//        return err;
//    
//    if (sps_pps)
//        memcpy(out->data + offset, sps_pps, sps_pps_size);
//    memcpy(out->data + sps_pps_size + nal_header_size + offset, in, in_size);
//    if (!offset) {
//        AV_WB32(out->data + sps_pps_size, 1);
//    } else {
//        (out->data + offset + sps_pps_size)[0] =
//        (out->data + offset + sps_pps_size)[1] = 0;
//        (out->data + offset + sps_pps_size)[2] = 1;
//    }
//    
//    return 0;
//}
//
//int h264_extradata_to_annexb(const uint8_t *codec_extradata, const int codec_extradata_size, AVPacket *out_extradata, int padding)
//{
//    uint16_t unit_size;
//    uint64_t total_size                 = 0;
//    uint8_t *out                        = NULL, unit_nb, sps_done = 0,
//    sps_seen                   = 0, pps_seen = 0, sps_offset = 0, pps_offset = 0;
//    const uint8_t *extradata            = codec_extradata + 4;
//    static const uint8_t nalu_header[4] = { 0, 0, 0, 1 };
//    int length_size = (*extradata++ & 0x3) + 1; // retrieve length coded size, 用于指示表示编码数据长度所需字节数
//    
//    sps_offset = pps_offset = -1;
//    
//    /* retrieve sps and pps unit(s) */
//    unit_nb = *extradata++ & 0x1f; /* number of sps unit(s) */
//    if (!unit_nb) {
//        goto pps;
//    }else {
//        sps_offset = 0;
//        sps_seen = 1;
//    }
//    
//    while (unit_nb--) {
//        int err;
//        
//        unit_size   = AV_RB16(extradata);
//        total_size += unit_size + 4;
//        if (total_size > INT_MAX - padding) {
//            av_log(NULL, AV_LOG_ERROR,
//                   "Too big extradata size, corrupted stream or invalid MP4/AVCC bitstream\n");
//            av_free(out);
//            return AVERROR(EINVAL);
//        }
//        if (extradata + 2 + unit_size > codec_extradata + codec_extradata_size) {
//            av_log(NULL, AV_LOG_ERROR, "Packet header is not contained in global extradata, "
//                   "corrupted stream or invalid MP4/AVCC bitstream\n");
//            av_free(out);
//            return AVERROR(EINVAL);
//        }
//        if ((err = av_reallocp(&out, total_size + padding)) < 0)
//            return err;
//        memcpy(out + total_size - unit_size - 4, nalu_header, 4);
//        memcpy(out + total_size - unit_size, extradata + 2, unit_size);
//        extradata += 2 + unit_size;
//    pps:
//        if (!unit_nb && !sps_done++) {
//            unit_nb = *extradata++; /* number of pps unit(s) */
//            if (unit_nb) {
//                pps_offset = total_size;
//                pps_seen = 1;
//            }
//        }
//    }
//    
//    if (out)
//        memset(out + total_size, 0, padding);
//    
//    if (!sps_seen)
//        av_log(NULL, AV_LOG_WARNING,
//               "Warning: SPS NALU missing or invalid. "
//               "The resulting stream may not play.\n");
//    
//    if (!pps_seen)
//        av_log(NULL, AV_LOG_WARNING,
//               "Warning: PPS NALU missing or invalid. "
//               "The resulting stream may not play.\n");
//    
//    out_extradata->data      = out;
//    out_extradata->size      = total_size;
//    
//    return length_size;
//}
//
//int h264_mp4toannexb(AVFormatContext *fmt_ctx, AVPacket *in, FILE *dst_fd)
//{
//    
//    AVPacket *out = NULL;
//    AVPacket spspps_pkt;
//    
//    int len;
//    uint8_t unit_type;
//    int32_t nal_size;
//    uint32_t cumul_size    = 0;
//    const uint8_t *buf;
//    const uint8_t *buf_end;
//    int            buf_size;
//    int ret = 0, i;
//    
//    out = av_packet_alloc();
//    
//    buf      = in->data;
//    buf_size = in->size;
//    buf_end  = in->data + in->size;
//    
//    do {
//        ret= AVERROR(EINVAL);
//        if (buf + 4 /*s->length_size*/ > buf_end)
//            goto fail;
//        
//        for (nal_size = 0, i = 0; i<4/*s->length_size*/; i++)
//            nal_size = (nal_size << 8) | buf[i];
//        
//        buf += 4; /*s->length_size;*/
//        unit_type = *buf & 0x1f;
//        
//        if (nal_size > buf_end - buf || nal_size < 0)
//            goto fail;
//        
//        /*
//         if (unit_type == 7)
//         s->idr_sps_seen = s->new_idr = 1;
//         else if (unit_type == 8) {
//         s->idr_pps_seen = s->new_idr = 1;
//         */
//        /* if SPS has not been seen yet, prepend the AVCC one to PPS */
//        /*
//         if (!s->idr_sps_seen) {
//         if (s->sps_offset == -1)
//         av_log(ctx, AV_LOG_WARNING, "SPS not present in the stream, nor in AVCC, stream may be unreadable\n");
//         else {
//         if ((ret = alloc_and_copy(out,
//         ctx->par_out->extradata + s->sps_offset,
//         s->pps_offset != -1 ? s->pps_offset : ctx->par_out->extradata_size - s->sps_offset,
//         buf, nal_size)) < 0)
//         goto fail;
//         s->idr_sps_seen = 1;
//         goto next_nal;
//         }
//         }
//         }
//         */
//        
//        /* if this is a new IDR picture following an IDR picture, reset the idr flag.
//         * Just check first_mb_in_slice to be 0 as this is the simplest solution.
//         * This could be checking idr_pic_id instead, but would complexify the parsing. */
//        /*
//         if (!s->new_idr && unit_type == 5 && (buf[1] & 0x80))
//         s->new_idr = 1;
//         
//         */
//        /* prepend only to the first type 5 NAL unit of an IDR picture, if no sps/pps are already present */
//        if (/*s->new_idr && */unit_type == 5 /*&& !s->idr_sps_seen && !s->idr_pps_seen*/) {
//            
//            h264_extradata_to_annexb( fmt_ctx->streams[in->stream_index]->codec->extradata,
//                                     fmt_ctx->streams[in->stream_index]->codec->extradata_size,
//                                     &spspps_pkt,
//                                     AV_INPUT_BUFFER_PADDING_SIZE);
//            
//            if ((ret=alloc_and_copy(out,
//                                    spspps_pkt.data, spspps_pkt.size,
//                                    buf, nal_size)) < 0)
//                goto fail;
//            /*s->new_idr = 0;*/
//            /* if only SPS has been seen, also insert PPS */
//        }
//        /*else if (s->new_idr && unit_type == 5 && s->idr_sps_seen && !s->idr_pps_seen) {
//         if (s->pps_offset == -1) {
//         av_log(ctx, AV_LOG_WARNING, "PPS not present in the stream, nor in AVCC, stream may be unreadable\n");
//         if ((ret = alloc_and_copy(out, NULL, 0, buf, nal_size)) < 0)
//         goto fail;
//         } else if ((ret = alloc_and_copy(out,
//         ctx->par_out->extradata + s->pps_offset, ctx->par_out->extradata_size - s->pps_offset,
//         buf, nal_size)) < 0)
//         goto fail;
//         }*/ else {
//             if ((ret=alloc_and_copy(out, NULL, 0, buf, nal_size)) < 0)
//                 goto fail;
//             /*
//              if (!s->new_idr && unit_type == 1) {
//              s->new_idr = 1;
//              s->idr_sps_seen = 0;
//              s->idr_pps_seen = 0;
//              }
//              */
//         }
//        
//        
//        len = fwrite( out->data, 1, out->size, dst_fd);
//        if(len != out->size){
//            av_log(NULL, AV_LOG_DEBUG, "warning, length of writed data isn't equal pkt.size(%d, %d)\n",
//                   len,
//                   out->size);
//        }
//        fflush(dst_fd);
//        
//    next_nal:
//        buf        += nal_size;
//        cumul_size += nal_size + 4;//s->length_size;
//    } while (cumul_size < buf_size);
//    
//    /*
//     ret = av_packet_copy_props(out, in);
//     if (ret < 0)
//     goto fail;
//     
//     */
//fail:
//    av_packet_free(&out);
//    
//    return ret;
//}
//
//void adts_header(char *szAdtsHeader, int dataLen){
//    
//    int audio_object_type = 2;
//    int sampling_frequency_index = 7;
//    int channel_config = 2;
//    
//    int adtsLen = dataLen + 7;
//    
//    szAdtsHeader[0] = 0xff;         //syncword:0xfff                          高8bits
//    szAdtsHeader[1] = 0xf0;         //syncword:0xfff                          低4bits
//    szAdtsHeader[1] |= (0 << 3);    //MPEG Version:0 for MPEG-4,1 for MPEG-2  1bit
//    szAdtsHeader[1] |= (0 << 1);    //Layer:0                                 2bits
//    szAdtsHeader[1] |= 1;           //protection absent:1                     1bit
//    
//    szAdtsHeader[2] = (audio_object_type - 1)<<6;            //profile:audio_object_type - 1                      2bits
//    szAdtsHeader[2] |= (sampling_frequency_index & 0x0f)<<2; //sampling frequency index:sampling_frequency_index  4bits
//    szAdtsHeader[2] |= (0 << 1);                             //private bit:0                                      1bit
//    szAdtsHeader[2] |= (channel_config & 0x04)>>2;           //channel configuration:channel_config               高1bit
//    
//    szAdtsHeader[3] = (channel_config & 0x03)<<6;     //channel configuration:channel_config      低2bits
//    szAdtsHeader[3] |= (0 << 5);                      //original：0                               1bit
//    szAdtsHeader[3] |= (0 << 4);                      //home：0                                   1bit
//    szAdtsHeader[3] |= (0 << 3);                      //copyright id bit：0                       1bit
//    szAdtsHeader[3] |= (0 << 2);                      //copyright id start：0                     1bit
//    szAdtsHeader[3] |= ((adtsLen & 0x1800) >> 11);           //frame length：value   高2bits
//    
//    szAdtsHeader[4] = (uint8_t)((adtsLen & 0x7f8) >> 3);     //frame length:value    中间8bits
//    szAdtsHeader[5] = (uint8_t)((adtsLen & 0x7) << 5);       //frame length:value    低3bits
//    szAdtsHeader[5] |= 0x1f;                                 //buffer fullness:0x7ff 高5bits
//    szAdtsHeader[6] = 0xfc;
//}
//int sum(int a,int b){
//    return a+b;
//}
//
//int main(int argc, const char * argv[]) {
//    // insert code here...
////    printf("Hello, World!\n");
//    int result=0;
//    
//    av_log_set_level(AV_LOG_DEBUG);
////
////    av_log(NULL, AV_LOG_DEBUG, "呜呜呜 终于出来了DEBUG\n");
////
////    av_log(NULL, AV_LOG_ERROR, "AV_LOG_ERROR\n");
////
////    av_log(NULL, AV_LOG_INFO, "AV_LOG_INFO\n");
//////    删除文件
////    result=avpriv_io_delete("/Users/apple/Desktop/DSC_0362.JPG");
//////
////    av_log(NULL,AV_LOG_ERROR, "result == %d",result);
//////     av_log(NULL, AV_LOG_INFO, "AV_LOG_INFO\n");
////
////    AVIODirContext *avioDirContext=NULL;
////    AVIODirEntry *avioEntry=NULL;
////
////   result= avio_open_dir(&avioDirContext, "../../../../", NULL);
////    if (result<0) {
////         av_log(NULL,AV_LOG_ERROR, "\n打开文件目录结果 result == %s",av_err2str(result));
////        return -1;
////    }
////
////    while (1) {
////        result=avio_read_dir(avioDirContext, &avioEntry);
////        if (result<0) {
////             av_log(NULL,AV_LOG_ERROR, "\n读取文件目录结果 result == %s",av_err2str(result));
////            return -1;
////        }
////
////        if (!avioEntry) {
////            break;
////        }
////
//////        打印文件部分属性
////        av_log(NULL,AV_LOG_ERROR, "\n %12s"PRId64" %lld \n",avioEntry->name,avioEntry->size);
////
////        avio_free_directory_entry(&avioEntry);
////
////    }
//////    关闭目录操作
////    result=avio_close_dir(&avioDirContext);
////
////    av_log(NULL,AV_LOG_ERROR, "\n关闭文件目录结果 result == %s",av_err2str(result));
////
////    return result;
//   
////    AVPacket packet;
////    int audio_index=0;
////    AVFormatContext *formatContext=NULL;
////
////    av_log_set_level(AV_LOG_DEBUG);
////
////    av_register_all();
////
////    result= avformat_open_input(&formatContext, "/Users/apple/Desktop/MV/周深 - 大鱼.mp4", NULL,NULL);
////
////
////    if (result<0) {
////        av_log(NULL, AV_LOG_ERROR, "打开视频失败 错误码%s\n",av_err2str(result));
////        return -1;
////    }
//    // y第一个参数表示之前打开文件的上下文。第二个参数表示流的索引 第三个表示之前的文件地址。第四个参数表示是输入还是输出
////    av_dump_format(formatContext, 0, "/Users/apple/Desktop/MV/陈奕迅 - 富士山下  (DUO 陈奕迅2010演唱会Live).mp4", 0);
//    
//    //    提取音频为aac;
////    FILE *dst_fd=fopen("/Users/apple/Desktop/MV/test.aac", "wb");
////
////    result=av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
////    audio_index=result;
////    if (result<0) {
////        av_log(NULL, AV_LOG_ERROR, "获取流失败 错误码 %s\n",av_err2str(result));
////        avformat_close_input(&formatContext);
////        fclose(dst_fd);
////        return -1;
////    }
////
////    av_init_packet(&packet);
////    int len;
////    while (av_read_frame(formatContext, &packet)>=0) {
////        if (packet.stream_index==audio_index) {
////
////            char adts_header_buf[7];
////            adts_header(adts_header_buf,packet.size);
////            fwrite(adts_header_buf, 1, 7, dst_fd);
////            len=fwrite(packet.data, 1, packet.size, dst_fd);
////
////            if (len!=packet.size) {
////                av_log(NULL, AV_LOG_ERROR, "长度不相等");
////            }
////        }
////        av_packet_unref(&packet);
////    }
////
////    avformat_close_input(&formatContext);
////
////
////    if (dst_fd) {
////        fclose(dst_fd);
////    }
//    //    av_log(NULL, AV_LOG_INFO, )
//    
//    
////    提取视频为h264
//    
////    FILE *dst_fd=fopen("/Users/apple/Desktop/MV/test.h264", "wb");
////
////    AVPacket videoPacket;
////    av_init_packet(&videoPacket);
////
////    int video_stream_index=av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
////
////    if (video_stream_index<0) {
////        av_log(NULL, AV_LOG_ERROR, "\nvideo stream cant find...");
////        return -1;
////    }
////
////    while (av_read_frame(formatContext, &videoPacket)>=0) {
////
////        if (videoPacket.stream_index==video_stream_index) {
////            h264_mp4toannexb(formatContext, &videoPacket, dst_fd);
////        }
////
////        av_packet_unref(&videoPacket);
////    }
////
////    avformat_close_input(&formatContext);
////    if (dst_fd) {
////        fclose(dst_fd);
////    }
//    
////    将mp4视频转换为flv
////    char *outfilename="/Users/apple/Desktop/MV/test.flv";
////
////    FILE *dst_fd=fopen(outfilename, "wb");
////
////    AVFormatContext *outFormatContext=NULL;
////    AVOutputFormat *outFormat=NULL;
////    AVPacket pkt;
////    int stream_mappint_size;
////    int *stream_mapping=NULL;
////    int stream_index=0;
////
////    av_init_packet(&pkt);
////    avformat_alloc_output_context2(&outFormatContext, NULL, NULL, outfilename);
////
////    if (!outFormatContext) {
////        av_log(NULL, AV_LOG_ERROR, "cant alloc output context\n");
////        return -1;
////    }
////
////    stream_mappint_size=formatContext->nb_streams;
////
////    stream_mapping=av_mallocz_array(stream_mappint_size, sizeof(stream_mapping));
////
////    outFormat=outFormatContext->oformat;
////
////
////    for (int i=0;i<formatContext->nb_streams; i++) {
////        AVStream *outStream=NULL;
////        AVStream *inStream=formatContext->streams[i];
////
////        AVCodecParameters *inCodecPar=inStream->codecpar;
////
////        if (inCodecPar->codec_type!=AVMEDIA_TYPE_VIDEO&&
////            inCodecPar->codec_type!=AVMEDIA_TYPE_AUDIO&&
////            inCodecPar->codec_type!=AVMEDIA_TYPE_SUBTITLE) {
////
////            stream_mapping[i]=-1;
////
////            continue;
////        }
////
////        stream_mapping[i]=stream_index++;
////
////        outStream=avformat_new_stream(outFormatContext, NULL);
////
////        if (!outStream) {
////            av_log(NULL, AV_LOG_ERROR, "cant alloc new output stream\n");
////            return -1;
////        }
////
////        result=avcodec_parameters_copy(outStream->codecpar, inCodecPar);
////
////        if (result<0) {
////            av_log(NULL, AV_LOG_ERROR, "cant copy parameters\n");
////            return -1;
////        }
////
////        outStream->codecpar->codec_tag=0;
////
////        if (!(outFormat->flags&AVFMT_NOFILE)) {
////            result=avio_open(&outFormatContext->pb, outfilename, AVIO_FLAG_WRITE);
////            if (result<0) {
////                av_log(NULL, AV_LOG_ERROR, "Could not open output file\n");
////                return -1;
////            }
////        }
////
////
////        result=avformat_write_header(outFormatContext, NULL);
////        if (result < 0) {
////            fprintf(stderr, "Error occurred when opening output file\n");
////            goto end;
////        }
////
////        while (1) {
////            AVStream *inStream,*outStream;
////
////            result=av_read_frame(formatContext, &pkt);
////
////            if (result<0) {
////                break;
////            }
////
////            inStream=formatContext->streams[pkt.stream_index];
////
////            if (pkt.stream_index >= stream_mappint_size ||
////                stream_mapping[pkt.stream_index] < 0) {
////                av_packet_unref(&pkt);
////                continue;
////            }
////
////
////            pkt.stream_index = stream_mapping[pkt.stream_index];
////            outStream = outFormatContext->streams[pkt.stream_index];
////
////            /* copy packet */
////            pkt.pts = av_rescale_q_rnd(pkt.pts, inStream->time_base, outStream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
////            pkt.dts = av_rescale_q_rnd(pkt.dts, inStream->time_base, outStream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
////            pkt.duration = av_rescale_q(pkt.duration, inStream->time_base, outStream->time_base);
////            pkt.pos = -1;
////
////            result=av_interleaved_write_frame(outFormatContext, &pkt);
////
////            if (result<-1) {
////                av_log(NULL, AV_LOG_ERROR, "Could not write frame \n");
////                return -1;
////            }
////
////            av_packet_unref(&pkt);
////
////        }
////        av_write_trailer(outFormatContext);
////
////    }
////
////    avformat_close_input(formatContext);
////
////    /* close output */
////    if (outFormatContext && !(outFormat->flags & AVFMT_NOFILE))
////        avio_closep(&outFormatContext->pb);
////    avformat_free_context(outFormatContext);
////
////    av_freep(&stream_mapping);
////
////    if (result < 0 && result != AVERROR_EOF) {
////        fprintf(stderr, "Error occurred: %s\n", av_err2str(result));
////        return 1;
////    }
////end:
////    return -1;
////
////
////    return 0;
//    
//    AVOutputFormat *ofmt = NULL;
//    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
//    AVPacket pkt;
//    const char *in_filename, *out_filename;
//    int ret, i;
//    int stream_index = 0;
//    int *stream_mapping = NULL;
//    int stream_mapping_size = 0;
//    
// 
//    in_filename  = "/Users/apple/Desktop/FFmpeg音视频核心技术精讲与实战/第1章 课程导学与准备工作/1-2 课程导学.mp4";
//    out_filename = "/Users/apple/Desktop/MV/test.avi";
//    
//    av_register_all();
//    
//    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
//        fprintf(stderr, "Could not open input file '%s'", in_filename);
//        goto end;
//    }
//    
//    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
//        fprintf(stderr, "Failed to retrieve input stream information");
//        goto end;
//    }
//    av_log(NULL, AV_LOG_ERROR, "input file format \n");
//    av_dump_format(ifmt_ctx, 0, in_filename, 0);
//    
//    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
//    if (!ofmt_ctx) {
//        fprintf(stderr, "Could not create output context\n");
//        ret = AVERROR_UNKNOWN;
//        goto end;
//    }
//    
//    stream_mapping_size = ifmt_ctx->nb_streams;
//    stream_mapping = av_mallocz_array(stream_mapping_size, sizeof(*stream_mapping));
//    if (!stream_mapping) {
//        ret = AVERROR(ENOMEM);
//        goto end;
//    }
//    
//    ofmt = ofmt_ctx->oformat;
//    
//    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
//        AVStream *out_stream;
//        AVStream *in_stream = ifmt_ctx->streams[i];
//        AVCodecParameters *in_codecpar = in_stream->codecpar;
//        
//        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
//            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
//            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
//            stream_mapping[i] = -1;
//            continue;
//        }
//        
//        stream_mapping[i] = stream_index++;
//        
//        out_stream = avformat_new_stream(ofmt_ctx, NULL);
//        if (!out_stream) {
//            fprintf(stderr, "Failed allocating output stream\n");
//            ret = AVERROR_UNKNOWN;
//            goto end;
//        }
//        
//        ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
//        if (ret < 0) {
//            fprintf(stderr, "Failed to copy codec parameters\n");
//            goto end;
//        }
//        out_stream->codecpar->codec_tag = 0;
//    }
//    av_log(NULL, AV_LOG_ERROR, "dump output file format\n");
//    av_dump_format(ofmt_ctx, 0, out_filename, 1);
////    打开输出文件
//    if (!(ofmt->flags & AVFMT_NOFILE)) {
//        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
//        if (ret < 0) {
//            fprintf(stderr, "Could not open output file '%s'", out_filename);
//            goto end;
//        }
//    }
//    AVDictionary *dict=NULL;
//    av_dict_set(&dict, "vstrict", "-1", 0);
//    ret = avformat_write_header(ofmt_ctx, &dict);
//    if (ret < 0) {
//        fprintf(stderr, "Error occurred when opening output file %d %s\n",ret,av_err2str(ret));
//        goto end;
//    }
//    
//    while (1) {
//        AVStream *in_stream, *out_stream;
//        
//        ret = av_read_frame(ifmt_ctx, &pkt);
//        if (ret < 0)
//            break;
//        
//        in_stream  = ifmt_ctx->streams[pkt.stream_index];
//        if (pkt.stream_index >= stream_mapping_size ||
//            stream_mapping[pkt.stream_index] < 0) {
//            av_packet_unref(&pkt);
//            continue;
//        }
//        
//        pkt.stream_index = stream_mapping[pkt.stream_index];
//        out_stream = ofmt_ctx->streams[pkt.stream_index];
////        log_packet(ifmt_ctx, &pkt, "in");
//        
//        /* copy packet */
//        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
//        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
//        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
//        pkt.pos = -1;
////        log_packet(ofmt_ctx, &pkt, "out");
//        
//        ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
//        if (ret < 0) {
//            fprintf(stderr, "Error muxing packet\n");
//            break;
//        }
//        av_packet_unref(&pkt);
//    }
//    
//    av_write_trailer(ofmt_ctx);
//end:
//    
//    avformat_close_input(&ifmt_ctx);
//    
//    /* close output */
//    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
//        avio_closep(&ofmt_ctx->pb);
//    avformat_free_context(ofmt_ctx);
//    
//    av_freep(&stream_mapping);
//    
//    if (ret < 0 && ret != AVERROR_EOF) {
//        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
//        return 1;
//    }
//    
//    return 0;
//}
