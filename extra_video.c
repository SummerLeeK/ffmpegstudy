//
//  extra_video.c
//  demo
//
//  Created by Apple on 2019/3/18.
//  Copyright © 2019年 Apple. All rights reserved.
//

#include <stdio.h>
#include <libavformat/avformat.h>


#ifndef AV_WB32
#   define AV_WB32(p, val) do {                 \
uint32_t d = (val);                     \
((uint8_t*)(p))[3] = (d);               \
((uint8_t*)(p))[2] = (d)>>8;            \
((uint8_t*)(p))[1] = (d)>>16;           \
((uint8_t*)(p))[0] = (d)>>24;           \
} while(0)
#endif

#ifndef AV_RB16
#   define AV_RB16(x)                           \
((((const uint8_t*)(x))[0] << 8) |          \
((const uint8_t*)(x))[1])
#endif

static int alloc_and_copy(AVPacket *out,
                          const uint8_t *sps_pps, uint32_t sps_pps_size,
                          const uint8_t *in, uint32_t in_size)
{
    uint32_t offset         = out->size;
    uint8_t nal_header_size = offset ? 3 : 4;
    int err;
    
    err = av_grow_packet(out, sps_pps_size + in_size + nal_header_size);
    if (err < 0)
        return err;
    
    if (sps_pps)
        memcpy(out->data + offset, sps_pps, sps_pps_size);
    memcpy(out->data + sps_pps_size + nal_header_size + offset, in, in_size);
    if (!offset) {
        AV_WB32(out->data + sps_pps_size, 1);
    } else {
        (out->data + offset + sps_pps_size)[0] =
        (out->data + offset + sps_pps_size)[1] = 0;
        (out->data + offset + sps_pps_size)[2] = 1;
    }
    
    return 0;
}


/**
 * 当该nalu单元是关键帧时 写入sps/pps到packet中
 */
int h264_extradata_to_annexb(const uint8_t *codec_extradata,const int codec_extradata_size,AVPacket *spspps,int padding){
    
    uint16_t unit_size;
    uint64_t total_size=0;
    
    uint8_t *out                        = NULL, unit_nb, sps_done = 0,
    sps_seen                   = 0, pps_seen = 0, sps_offset = 0, pps_offset = 0;
    const uint8_t *extradata=codec_extradata+4;
    static const uint8_t nalu_header[4]={0,0,0,1};
    
    int length_size=((*extradata++)&0x3)+1;
    
    sps_offset=pps_offset=-1;
    
    unit_nb=*extradata++ & 0x1f;
    
    if (!unit_nb) {
        goto pps;
    }else{
        sps_seen=1;
        sps_offset=0;
    }
    
    
    while (unit_nb--) {
        
        int ret;
        
        unit_size=AV_RB16(extradata);
        
        total_size += unit_size+4;
        
        if (total_size>INT_MAX-padding) {
            
            av_log(NULL, AV_LOG_ERROR, "Too big extradata size, corrupted stream or invalid MP4/AVCC bitstream\n");
            
            av_free(out);
            
            return AVERROR(EINVAL);
        }
        
        if (extradata+2+unit_size>codec_extradata+codec_extradata_size) {
            av_log(NULL, AV_LOG_ERROR, "Packet header is not contained in global extradata, "
                   "corrupted stream or invalid MP4/AVCC bitstream\n");
            av_free(out);
            return AVERROR(EINVAL);
        }
        
        
        if ((ret=av_reallocp(&out, total_size+padding))<0){
            return ret;
        }
        
        memcpy(out+total_size-unit_size-4, nalu_header, 4);
        
        memcpy(out+total_size-unit_size, extradata+2, unit_size);
        
        extradata+=2+unit_size;
        
    pps:
        if (!unit_nb&&!sps_done++) {
            unit_nb = *extradata++; /* number of pps unit(s) */
            if (unit_nb) {
                pps_offset = total_size;
                pps_seen = 1;
            }
        }
    }
    
    
    if (out)
        memset(out + total_size, 0, padding);
    
    if (!sps_seen)
        av_log(NULL, AV_LOG_WARNING,
               "Warning: SPS NALU missing or invalid. "
               "The resulting stream may not play.\n");
    
    if (!pps_seen)
        av_log(NULL, AV_LOG_WARNING,
               "Warning: PPS NALU missing or invalid. "
               "The resulting stream may not play.\n");
    
    spspps->data      = out;
    spspps->size      = total_size;
    
    return length_size;
    
}



/**
 * 从每一个packet包中分离出nalu单元
 */
int h264_mp4toannexb(AVFormatContext *in_fmt_ctx,AVPacket *in,FILE *dst_fd){
    
    AVPacket *out = NULL;
    AVPacket spspps_pkt;
    
    int len;
    uint8_t unit_type;
    int32_t nal_size;
    uint32_t cumul_size    = 0;
    const uint8_t *buf;
    const uint8_t *buf_end;
    int            buf_size;
    int ret = 0, i;
    
    out = av_packet_alloc();
    
    buf      = in->data;
    buf_size = in->size;
    buf_end  = in->data + in->size;
    
    
    
    
    do{
        ret=AVERROR(EINVAL);
        //        整个buf的长度不足4
        if (buf+4>buf_end) {
            
            goto failed;
        }
        //        每一个packet可能包含了多个nalu单元
        //        前四位存储了整个nal单元的长度
        
        for (nal_size = 0,i=0; i<4; i++) {
            //            相当于32的nal_size 每八位从左至右依次是buf[0] buf[1] buf[2] buf[3]组成
            //            av_log(NULL, AV_LOG_ERROR, "buf %d = %d\t%d \n",i,buf[i],buf_end-buf);
            nal_size=(nal_size<<8) | buf[i];
        }
        //
        //               av_log(NULL,AV_LOG_ERROR,"nal_size = %d \t buf_end - buf = %d \n",nal_size,buf_end-buf);
        //        往前移4位
        buf += 4; /*s->length_size;*/

        //       第5位 取*buf值的后5位 0x1f=00011111
        unit_type= *buf & 0x1f;
        
        if (nal_size > buf_end-buf || nal_size<0) {
                        av_log(NULL, AV_LOG_ERROR, "nal_size>buf_size||nal_size<0 %d\t %d \t%d\n",nal_size,buf_size,buf_end-buf);
            goto failed;
        }
        //       关于unit_type的几种类型  https://blog.csdn.net/leixiaohua1020/article/details/50534369
        //        为1表示非关键帧 表示sps时为7 表示pps=8 关键帧为5
        if (unit_type==5) {
            // 该帧为关键帧
            h264_extradata_to_annexb(in_fmt_ctx->streams[in->stream_index]->codec->extradata,in_fmt_ctx->streams[in->stream_index]->codec->extradata_size,&spspps_pkt,AV_INPUT_BUFFER_PADDING_SIZE);
            
            if ((ret=alloc_and_copy(out,spspps_pkt.data,spspps_pkt.size,buf,nal_size))<0)
                goto failed;
            
            
        }else{
            if ((ret=alloc_and_copy(out,NULL,0,buf,nal_size))<0)
                goto failed;
            
        }
        
        len=fwrite(out->data, 1, out->size, dst_fd);
        
        if(len != out->size){
            av_log(NULL, AV_LOG_DEBUG, "warning, length of writed data isn't equal pkt.size(%d, %d)\n",
                   len,
                   out->size);
        }
        
        fflush(dst_fd);
    next_nal:
        
        buf        += nal_size;
        cumul_size += nal_size + 4;//s->length_size;
        
    }while (cumul_size < buf_size);
    
failed:
    av_packet_free(&out);
    return -1;
    
}




/**
 * 1.
 */
int mp42h264(char *srcfile,char *dsrfile){
    
    AVFormatContext *in_fmt_ctx=NULL;
    
    AVPacket pkt;
    
    FILE *dst_fd=NULL;
    
    int err_code;
    char errors[1024];
    
    int video_stream_index=-1;
    
    dst_fd = fopen(dsrfile, "wb");
    if (!dst_fd) {
        av_log(NULL, AV_LOG_DEBUG, "Could not open destination file %s\n", dsrfile);
        return -1;
    }
    
    /*open input media file, and allocate format context*/
    if((err_code = avformat_open_input(&in_fmt_ctx, srcfile, NULL, NULL)) < 0){
        av_strerror(err_code, errors, 1024);
        av_log(NULL, AV_LOG_DEBUG, "Could not open source file: %s, %d(%s)\n",
               srcfile,
               err_code,
               errors);
        return -1;
    }
    
    /*dump input information*/
//    av_dump_format(in_fmt_ctx, 0, srcfile, 0);
    
    /*initialize packet*/
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    
    /*find best video stream*/
    video_stream_index = av_find_best_stream(in_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if(video_stream_index < 0){
        av_log(NULL, AV_LOG_DEBUG, "Could not find %s stream in input file %s\n",
               av_get_media_type_string(AVMEDIA_TYPE_VIDEO),
               srcfile);
        return AVERROR(EINVAL);
    }
    
    /*
     if (avformat_write_header(ofmt_ctx, NULL) < 0) {
     av_log(NULL, AV_LOG_DEBUG, "Error occurred when opening output file");
     exit(1);
     }
     */
    
    /*read frames from media file*/
    while(av_read_frame(in_fmt_ctx, &pkt) >=0 ){
        if(pkt.stream_index == video_stream_index){
            /*
             pkt.stream_index = 0;
             av_write_frame(ofmt_ctx, &pkt);
             av_free_packet(&pkt);
             */
            
            h264_mp4toannexb(in_fmt_ctx, &pkt, dst_fd);
            
        }
        
        //release pkt->data
        av_packet_unref(&pkt);
    }
    
    //av_write_trailer(ofmt_ctx);
    
    /*close input media file*/
    avformat_close_input(&in_fmt_ctx);
    if(dst_fd) {
        fclose(dst_fd);
    }
    
    //avio_close(ofmt_ctx->pb);
    
    return 0;
}



//int main(int argc,char *argv[]){
//    av_log_set_level(AV_LOG_INFO);
//    mp42h264("/Users/apple/Desktop/MV/陈奕迅 - 龙舌兰.mp4", "/Users/apple/Desktop/MV/陈奕迅 - 龙舌兰.h264");
//    return 0;
//}
