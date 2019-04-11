//
//  analyze_h264_info.c
//  demo
//  https://blog.csdn.net/leixiaohua1020/article/details/50534369
//
//  Created by Apple on 2019/3/21.
//  Copyright © 2019年 Apple. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>

typedef enum {

    NALU_TYPE_SLICE = 1,
    NALU_TYPE_DPA = 2,
    NALU_TYPE_DPB = 3,
    NALU_TYPE_DPC = 4,
    //    关键帧
    NALU_TYPE_IDR = 5,
    NALU_TYPE_SEI = 6,
    NALU_TYPE_SPS = 7,
    NALU_TYPE_PPS = 8,
    NALU_TYPE_AUD = 9,
    NALU_TYPE_EOSEQ = 10,
    NALU_TYPE_EOSTREAM = 11,
    NALU_TYPE_FILL = 12,

} NALUType;

typedef enum {
    NALU_PRIORITY_DISPOSABLE = 0,
    NALU_PRIORITY_LOW = 1,
    NALU_PRIORITY_HIGH = 2,
    NALU_PRIORITY_HIGHEST = 3
} NALUPriority;

typedef struct
{
    int start_prefixcode_len;
    unsigned len;
    unsigned max_size;
    int forbidden_bit;
    int nal_refrence_bit;
    int nal_unit_type;
    char* buf;

} NALU;

FILE* h264stream = NULL;

int findStartCode2(unsigned char* Buf)
{
    if (Buf[0] != 0 || Buf[1] != 0 || Buf[2] != 1) {
        return 0;
    }
    return 1;
}

int findStartCode3(unsigned char* Buf)
{
    if (Buf[0] != 0 || Buf[1] != 0 || Buf[2] != 0 || Buf[3] != 1) {
        return 0;
    }
    return 1;
}

int get_nalu_unit(NALU* nalu)
{
    int info2, info3 = 0;
    int startFound = 0;
    int pos = 0;
    nalu->start_prefixcode_len = 3;

    int rewind = 0;
    unsigned char* Buf;
    int length = 0;

    Buf = (unsigned char*)calloc(20, sizeof(unsigned char));

    if (Buf == NULL) {
        printf("Buf cant calloc any space");
        return 0;
    }
    length = fread(Buf, 1, 3, h264stream);

    if (length != 3) {
        return 0;
    }

    info2 = findStartCode2(Buf);

    if (info2 != 1) {
        if (fread(Buf + 3, 1, 1, h264stream) != 1) {
            free(Buf);
            return 0;
        }

        info3 = findStartCode3(Buf);

        if (info3 != 1) {
            free(Buf);
            return 0;
        } else {
            nalu->start_prefixcode_len = 4;
            pos = 4;
        }
    } else {
        nalu->start_prefixcode_len = 3;
        pos = 3;
    }

    startFound = 0;
    info2 = info3 = 0;

    while (!startFound) {
        if (feof(h264stream)) {
            nalu->len = pos - 1 - nalu->start_prefixcode_len;

            memcpy(nalu->buf, &Buf[nalu->start_prefixcode_len], 1);

            nalu->forbidden_bit = nalu->buf[0] & 0x80;
            nalu->nal_refrence_bit = nalu->buf[0] & 0x60;
            nalu->nal_unit_type = nalu->buf[0] & 0x1f;

            free(Buf);
            return pos - 1;
        }

        Buf[pos++] = fgetc(h264stream);
        info3 = findStartCode3(&Buf[pos - 4]);
        if (info3 != 1)
            info2 = findStartCode2(&Buf[pos - 3]);
        startFound = (info2 == 1 || info3 == 1);
    }

    rewind = (info3 == 1) ? -4 : -3;

    if (0 != fseek(h264stream, rewind, SEEK_CUR)) {
        free(Buf);
        printf("GetAnnexbNALU: Cannot fseek in the bit stream file");
    }

    // Here the Start code, the complete NALU, and the next start code is in the Buf.
    // The size of Buf is pos, pos+rewind are the number of bytes excluding the next
    // start code, and (pos+rewind)-startcodeprefix_len is the size of the NALU excluding the start code

    nalu->len = (pos + rewind) - nalu->start_prefixcode_len;
    if (Buf!=NULL) {
        memcpy(nalu->buf, &Buf[nalu->start_prefixcode_len], 1); //
    }
    
    nalu->forbidden_bit = nalu->buf[0] & 0x80; //1 bit
    nalu->nal_refrence_bit = nalu->buf[0] & 0x60; // 2 bit
    nalu->nal_unit_type = (nalu->buf[0]) & 0x1f; // 5 bit

    if (Buf!=NULL) {
        free(Buf);
    }

    return pos + rewind;
}

int analyze_h264(char* srcfile,char *dstfile)
{
    printf("start analyze h264 ...\n");
    NALU* nalu = NULL;

    int buffersize = 500;

    h264stream = fopen(srcfile, "r+");

    FILE *out=fopen(dstfile, "wb+");
    
    if (h264stream == NULL) {
        return 0;
    }
    nalu = (NALU*)calloc(1, sizeof(NALU));

    if (nalu == NULL) {
        return 0;
    }
    nalu->start_prefixcode_len = 3;
    nalu->max_size = buffersize;
    nalu->buf = (char*)calloc(buffersize, sizeof(char));

    int data_offset = 0;
    int nal_num = 0;
    if (nalu->buf == NULL) {
        free(nalu);
        return 0;
    }

    printf("-----+-------- NALU Table ------+---------+\n");
    printf(" NUM |    POS  |    IDC |  TYPE |   LEN   |\n");
    printf("-----+---------+--------+-------+---------+\n");

    char header[]="-----+-------- NALU Table ------+---------+\n NUM |    POS  |    IDC |  TYPE |   LEN   |\n-----+---------+--------+-------+---------+\n";
    
    fwrite(header, sizeof(header), 1, out);
    
    while (!feof(h264stream) ) {
        int data_length = 0;

        data_length = get_nalu_unit(nalu);
        char type_str[20] = { 0 };

        switch (nalu->nal_unit_type) {
        case NALU_TYPE_SLICE:
            sprintf(type_str, "SLICE");

            break;
        case NALU_TYPE_DPA:
            sprintf(type_str, "DPA");

            break;
        case NALU_TYPE_DPB:
            sprintf(type_str, "DPB");

            break;
        case NALU_TYPE_DPC:
            sprintf(type_str, "DPC");

            break;
        case NALU_TYPE_IDR:
            sprintf(type_str, "IDR");

            break;
        case NALU_TYPE_SEI:
            sprintf(type_str, "SEI");

            break;
        case NALU_TYPE_SPS:
            sprintf(type_str, "SPS");

            break;
        case NALU_TYPE_PPS:
            sprintf(type_str, "PPS");

            break;
        case NALU_TYPE_AUD:
            sprintf(type_str, "AUD");

            break;
        case NALU_TYPE_EOSEQ:
            sprintf(type_str, "EOSEQ");

            break;
        case NALU_TYPE_EOSTREAM:
            sprintf(type_str, "EOSTREAM");

            break;
        case NALU_TYPE_FILL:
            sprintf(type_str, "FILL");

            break;
        default:
            sprintf(type_str, "%d",nalu->nal_unit_type);
        }

        char idc_str[20] = { 0 };

        switch (nalu->nal_refrence_bit >> 5) {
        case NALU_PRIORITY_DISPOSABLE:
            sprintf(idc_str, "DISPOS");

            break;
        case NALU_PRIORITY_LOW:
            sprintf(idc_str, "LOW");

            break;
        case NALU_PRIORITY_HIGH:
            sprintf(idc_str, "HIGH");

            break;
        case NALU_PRIORITY_HIGHEST:
            sprintf(idc_str, "HIGHEST");

            break;
        }

        printf("%5d| %8d| %7s| %6s| %8d|\n", nal_num, data_offset,
            idc_str, type_str, nalu->len);

        char item[100];
        
        sprintf(item, "%5d| %8d| %7s| %6s| %8d|\n", nal_num, data_offset,
                idc_str, type_str, nalu->len);
        
        
        
        fwrite(item, sizeof(item), 1, out);
        
    
        data_offset = data_offset + data_length;

        nal_num++;
    }

    if (nalu) {
        if (nalu->buf) {
            free(nalu->buf);
            nalu->buf = NULL;
        }
        free(nalu);
    }

    if (h264stream) {
        fclose(h264stream);
    }
    
    if (out) {
        fclose(out);
    }

    return 0;
}

//int main(int argc, char* argv[])
//{
//    analyze_h264("/Users/apple/Desktop/MV/陈奕迅 - 龙舌兰.mp4","/Users/apple/Desktop/MV/陈奕迅 - 龙舌兰h264.txt");
//
//    return 0;
//}
