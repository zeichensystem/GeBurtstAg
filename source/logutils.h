#ifndef LOGUTILS_H
#define LOGUTILS_H

#include "tonc_types.h"

// By Nick Sells/adverseengineer: https://github.com/adverseengineer/libtonc/blob/master/include/tonc_mgba.h (last retrieved 2021-07-09)
typedef enum LogLevel {
	LOG_FATAL= 0x100,
	LOG_ERR                     = 0x101,
	LOG_WARN                    = 0x102,
	LOG_INFO                    = 0x103
} LogLevel;

void mgba_printf(const char* fmt, ...);


// My own functions from here...
void assertion(bool cond, const char* name);
void panic(const char* message);

int getFps(void);
void logutilsInit(int showPerfInteval);
void perfPrint(void);

#endif