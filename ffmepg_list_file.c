//
//  ffmepg_list_file.c
//  demo
//
//  Created by Apple on 2019/3/16.
//  Copyright © 2019年 Apple. All rights reserved.
//  对文件目录的一些操作
//

#include <stdio.h>
#include <libavformat/avformat.h>


int print_dir_info(char * dirpath){
    int ret;
    
    AVIODirContext *dirContext=NULL;
    AVIODirEntry *dirEntry=NULL;
    
//    1.初始化上下文dirContext
    ret=avio_open_dir(&dirContext, dirpath, NULL);
    
    if (ret<0) {
        av_log(NULL, AV_LOG_ERROR, "cant alloc AVIODirContext %s",av_err2str(ret));
        return ret;
    }
    
    while (1) {
//        2.读取目录信息 将读取到的信息存放在dirEntry
       ret= avio_read_dir(dirContext,&dirEntry);
        
        if (ret<0) {
            av_log(NULL, AV_LOG_ERROR, "cant read dir %s",av_err2str(ret));
            
            goto __failed;
         
        }
        
        if (!dirEntry) {
            break;
        }
        
        
        av_log(NULL, AV_LOG_INFO, " %s \t %lld \t %lld \n",dirEntry->name,dirEntry->size,dirEntry->modification_timestamp);
        
//      将dirEntry占有的空间释放掉 如果不去释放 只要dirEntry最x开始不为空 那么上一步的判断非空便永远不会走break
        avio_free_directory_entry(&dirEntry);
    }
    
    
    
      avio_close_dir(&dirContext);
__failed:
    avio_close_dir(&dirContext);
    return ret;
    
  
    
    
    
    return 0;
}

//int main(int argc, char *argv[]){
//    
//    print_dir_info("./");
//}
