//
//  log.c
//  demo
//
//  Created by Apple on 2019/4/12.
//  Copyright © 2019年 Apple. All rights reserved.
//

#include "log.h"
#include <libavutil/log.h>

void base_log(int level, char* info)
{
    av_log(NULL, level, info);
}

void log_e(char* info)
{
    base_log(AV_LOG_ERROR, info);
}

void log_d(char* info)
{
    base_log(AV_LOG_DEBUG, info);
}

void log_i(char* info)
{
    base_log(AV_LOG_INFO, info);
}

void log_w(char* info)
{
    base_log(AV_LOG_WARNING, info);
}
