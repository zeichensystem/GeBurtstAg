
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <tonc.h>

#include "logutils.h"
#include "render/draw.h"
#include "globals.h"
#include "timer.h"

// mgba_log and mgba_printf by Nick Sells/adverseengineer: https://github.com/adverseengineer/libtonc/blob/master/src/tonc_mgba.c
// (Modified by me.)

void mgba_log(const u32 level, const char* str) 
{
    REG_LOG_ENABLE = 0xC0DE;
    u32 chars_left = strlen(str);

    while(chars_left) { //splits the message into 256-char log entries
        u32 chars_to_write = min(chars_left, LOG_MAX_CHARS_PER_LINE);

        memcpy(REG_LOG_STR, str, chars_to_write);
        REG_LOG_LEVEL = level; //every time this is written to, mgba creates a new log entry

        str += chars_to_write;
        chars_left -= chars_to_write;
    }
}

void mgba_printf(const char* fmt, ...) 
{
	va_list args;
	va_start(args, fmt);
	char buf[LOG_MAX_CHARS_PER_LINE];
	vsnprintf(buf, LOG_MAX_CHARS_PER_LINE, fmt, args);
	mgba_log(LOG_INFO, buf);
	va_end(args);
}


// My own code from here onwards:
void panic(const char* message) 
{
    if (message)
        mgba_printf("Error: %s", message);
    else
        mgba_printf("Unnamed error");
 
    int w = g_mode == DCNT_MODE5 ? M5_SCALED_W : SCREEN_WIDTH;
    int h = g_mode == DCNT_MODE5 ? M5_SCALED_H : SCREEN_HEIGHT;
    char msg[256] = "Sorry ;w;";

    COLOR frontPal[3] = {CLR_YELLOW, CLR_RED, CLR_WHITE};

    switch (g_mode) {
    case DCNT_MODE5:
        setDispScaleM5Scaled();
        memset32(vid_page, dup16(CLR_RED), (160 * 100)/2);
        m5_puts(w / 2 - 8 * (strlen(msg) / 2), h / 2 - 8, msg, CLR_WHITE);
        vid_flip();
        break;
    case DCNT_MODE4:
        resetDispScale();
        setM4Pal(frontPal, sizeof frontPal / sizeof frontPal[0]);
        m4_fill(1);
        m4_puts(w / 2 - 8 * (strlen(msg) / 2), h / 2 - 8, msg, 2);
        vid_flip();
        break;
    case DCNT_MODE3:
        resetDispScale();
        m3_fill(CLR_RED);
        m3_puts(w / 2 - 8 * (strlen(msg) / 2), h / 2 - 8, msg, CLR_WHITE);
        break;
    default:
        mgba_printf("panic: no valid mode selected, can't display.");
        break;
    }
    Halt();
    VBlankIntrDelay(60 * 16);
}

void assertion(bool cond, const char* name) 
{
    if (!cond) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Assertion '%s' failed", name ? name : "(unnamed)");
        panic(msg);
    }
}


static FIXED prevFps = 0;
static Timer showPerfTimer;

int getFps(void) 
{
    const FIXED smoothing = 205; // 0,8
    FIXED currentFps =  fx12Tofx(fx12div(int2fx12(1), g_timer.deltatime));
    FIXED fps = fxmul(prevFps, smoothing) + fxmul(currentFps, int2fx(1) - smoothing);
    prevFps = fps;
    return fx2int(fps);
} 

void logutilsInit(int showPerfInteval) 
{
    showPerfTimer = timerNew(int2fx12(showPerfInteval), TIMER_REGULAR); // We don't want to print the performance data every frame, so we use a timer to gather it in intervals. 
    timerStart(&showPerfTimer);
}

void perfPrint(void) {
    timerTick(&showPerfTimer);
    if (showPerfTimer.done || !g_frameCount) { 
        performancePrintAll();
        timerStart(&showPerfTimer);
    }     

}