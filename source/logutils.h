#ifndef LOGUTILS_H
#define LOGUTILS_H

#include "tonc_types.h"

/* 
    mGBA logging functionality
    author: Nick Sells
    date: 20210221
    Credits: https://github.com/adverseengineer/libtonc/blob/master/include/tonc_mgba.h
             https://github.com/adverseengineer/libtonc/blob/master/src/tonc_mgba.c
*/

#define LOG_FATAL               (u32) 0x100
#define LOG_ERR                 (u32) 0x101
#define LOG_WARN                (u32) 0x102
#define LOG_INFO                (u32) 0x103
#define LOG_MAX_CHARS_PER_LINE  (u32) 256
#define REG_LOG_STR             (char*) 0x4FFF600
#define REG_LOG_LEVEL           *(vu16*) 0x4FFF700
#define REG_LOG_ENABLE          *(vu16*) 0x4FFF780

void mgba_log(const u32 level, const char* str);
void mgba_printf(const char* fmt, ...);

// My own functions here
void assertion(bool cond, const char* name);
void panic(const char* message);

int getFps(void);

#endif