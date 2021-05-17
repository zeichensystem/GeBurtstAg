#include <stdlib.h>
#include <tonc.h>

#include "logutils.h"
#include "globals.h"
#include "math.h"
#include "timer.h"
#include "scene.h"
#include "model.h"

#define SHOW_DEBUG

static FIXED_12 g_wave_time;
static FIXED g_wave_amp;

void wave(void) 
{
    FIXED offset = fxmul(g_wave_amp, sinFx(int2fx12(REG_VCOUNT) >> 2));
    REG_BG2X = offset;

}

void waveUpdate(void) 
{
    g_wave_time = g_timer.time;
    g_wave_amp = int2fx(3) + fxmul(int2fx(6), sinFx(g_wave_time << 5));
}


int main(void) 
{
    srand(1997); 
    irq_init(NULL);
    irq_add(II_VBLANK, NULL);

    // irq_add(II_HBLANK, wave);
    // irq_add(II_VBLANK, waveUpdate);

    globalsInit();
    drawInit();
    mathInit();
    timerInit();
    modelInit();
    scenesInit();

    Timer showPerfTimer = timerNew(int2fx12(2), TIMER_REGULAR); // We don't want to print the performance data every frame, so we use a timer to gather it in intervals. 
    timerStart(&showPerfTimer);

    while (1) {
        key_poll();
        scenesDispatchUpdate();

        if (g_mode != DCNT_MODE5 && g_mode != DCNT_MODE4) {
            VBlankIntrWait();
        }

        scenesDispatchDraw();

        int fps = getFps();
        #ifdef SHOW_DEBUG
        char dbg[64];
        snprintf(dbg, sizeof(dbg),  "FPS: %d", fps);
        m5_puts(8, 8, dbg, CLR_LIME);
        #endif
        
        if (g_mode == DCNT_MODE5 || g_mode == DCNT_MODE4) {
            vid_flip();
        }

        if (showPerfTimer.done || !g_frameCount) { 
            performancePrintAll();
            timerStart(&showPerfTimer);
        }

        timerTick(&g_timer);
        timerTick(&showPerfTimer);
        performanceReset();
        ++g_frameCount;
    }
    return 0; // For dust thou art, and unto dust shalt thou return.
}
