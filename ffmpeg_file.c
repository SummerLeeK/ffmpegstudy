//
//  ffmpeg_file.c
//  demo
//
//  Created by Apple on 2019/3/16.
//  Copyright © 2019年 Apple. All rights reserved.
// 利用ffmepg对文件重命名以及移动删除操作
//

#include <stdio.h>
#include <libavformat/avformat.h>

/**
 * 移动挥或者重命名文件。
 **/
int movefile(char * srcpath,char *dstpath){
    
    int ret = avpriv_io_move(srcpath, dstpath);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "file cant move %d\n",ret);
        
        return ret;
    }
    
    av_log(NULL, AV_LOG_DEBUG, "%s file is move to %s\n",srcpath,dstpath);
    
    return 0;
}



/**
 *  删除文件
 */
int deletefile(char *srcfile){
    
    if (srcfile==NULL) {
        av_log(NULL, AV_LOG_ERROR, "file path cant null");
        return -1;
    }
    
//    关键代码。删除文件 传入文件路径 相对绝对均可。
    int ret=avpriv_io_delete(srcfile);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "file cant delete %d",AVERROR(ret));
        
        return ret;
    }
    
    av_log(NULL, AV_LOG_DEBUG, "%s file is delete",srcfile);
    
    printf("delete file succ");
    
    return 0;
}


//int main(int argc,char *argv[]){
//    
////    测试删除文件
////    deletefile("./下载必看.txt");
//    
//    
//    movefile("./下载必看.txt", "./下载不看.txt");
//    
//}
