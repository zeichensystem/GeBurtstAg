#include <tonc.h>
#include "AAS.h"

#include "logutils.h"
#include "globals.h"
#include "math.h"
#include "timer.h"
#include "scene.h"
#include "model.h"
#include "render/draw.h"

#include "../data-audio/AAS_Data.h"

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

void audioInit(void) 
{
    AAS_SetConfig( AAS_CONFIG_MIX_24KHZ, AAS_CONFIG_CHANS_8, AAS_CONFIG_SPATIAL_MONO, AAS_CONFIG_DYNAMIC_OFF);
    irq_add(II_TIMER1, AAS_FastTimer1InterruptHandler);
    irq_add(II_VBLANK, AAS_DoWork);

    AAS_MOD_Play(AAS_DATA_MOD_aaa);
}


int main(void) 
{
    // REG_WAITCNT controls the pre-fetch buffer and the waitstates of the external cartridge bus.
    REG_WAITCNT = 0x4317; // This setting yields a *major* perf improvement over the defaults; cf. https://problemkaputt.de/gbatek.htm#gbatechnicaldata (last retrieved 2021-05-28).
    sqran(2001);
    irq_init(NULL);
    // irq_add(II_VBLANK, NULL);
    // irq_add(II_HBLANK, wave);
    // irq_add(II_VBLANK, waveUpdate);

    audioInit();
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
