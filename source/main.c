#include <stdlib.h>
#include <tonc.h>

#include "logutils.h"
#include "globals.h"
#include "math.h"
#include "timer.h"
#include "scene.h"
#include "model.h"

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
    logutilsInit(2);

    while (1) {
        scenesDispatchUpdate();
        scenesDispatchDraw();
        
        performanceGather();
        perfPrint();
        timerTick(&g_timer);
        ++g_frameCount;
    }
    return 0; // For dust thou art, and unto dust shalt thou return.
}
