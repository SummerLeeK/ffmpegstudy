//
//  log.h
//  demo
//
//  Created by Apple on 2019/4/12.
//  Copyright © 2019年 Apple. All rights reserved.
//

#ifndef log_h
#define log_h

#include <stdio.h>
#include <libavutil/log.h>
#endif /* log_h */

void base_log(int level,char *info);

void log_e(char *info);

void log_d(char *info);

void log_i(char *info);

void log_w(char *info);
