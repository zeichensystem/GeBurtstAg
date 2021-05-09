#include <stdlib.h>
#include <tonc.h>

#include "logutils.h"
#include "globals.h"
#include "math.h"
#include "timer.h"
#include "scene.h"
#include "model.h"
#include "tracker.h"

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
    // irq_add(II_HBLANK, wave);
    // irq_add(II_VBLANK, waveUpdate);

    globalsInit();
    drawInit();
    mathInit();
    timerInit();
    modelInit();
    trackerInit();
    scenesInit();
    
    /* 
        Scaling using the affine background capabilities of the GBA. 
        We use Mode 5 (160x128) with an "internal/logical" resolution of 160x100 scaled to fit the 
        240x160 (factor 1.5) screen of the GBA (with 5px letterboxes on the top and bottom). 
    */
    FIXED threeHalfInv = 170; // 170 is about (3/2)^-1 in .8 fixed point.
    AFF_SRC_EX asx= {
        .alpha=0,
        .sx=threeHalfInv,
        .sy=threeHalfInv,
        .scr_x=0,
        .scr_y=5, // Vertical letterboxing.
        .tex_x=0,
        .tex_y=0 };
    BG_AFFINE bgaff;
    bg_rotscale_ex(&bgaff, &asx);
    REG_BG_AFFINE[2]= bgaff;

    Timer showPerfTimer = timerNew(int2fx12(2), TIMER_REGULAR);
    timerStart(&showPerfTimer);

    while (1) {
        key_poll();
        scenesDispatchUpdate();
        scenesDispatchDraw();

        int fps = getFps();
        #ifdef SHOW_DEBUG
        char dbg[64];
        snprintf(dbg, sizeof(dbg),  "FPS: %d", fps);
        m5_puts(8, 8, dbg, CLR_LIME);
        #endif

        vid_flip();

        if (showPerfTimer.done) { // We don't want to print the performance data every frame, so we use a timer. 
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
