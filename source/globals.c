#include "globals.h"

int g_mode = DCNT_MODE5;

Timer g_timer;
int g_frameCount;

void globalsInit(void) 
{
    g_timer = timerNew(TIMER_MAX_DURATION, TIMER_REGULAR); // Time stored in seconds as .12 fixedpoint, overflows in about 291 hours I think, so we should be fine to assume it never will
    g_frameCount = 0;
    timerStart(&g_timer);
 }
