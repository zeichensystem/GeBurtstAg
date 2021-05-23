
#ifndef GLOBALS_H
#define GLOBALS_H

#include <tonc_video.h>

#include "camera.h"
#include "render/draw.h"
#include "timer.h"

#define M5_SCALED_W 160
#define M5_SCALED_H 100

extern int g_mode;
extern Timer g_timer;
extern int g_frameCount;

void globalsInit(void);
#endif