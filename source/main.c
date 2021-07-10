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

void audioInit(void) 
{
    AAS_SetConfig( AAS_CONFIG_MIX_24KHZ, AAS_CONFIG_CHANS_8, AAS_CONFIG_SPATIAL_MONO, AAS_CONFIG_DYNAMIC_OFF);
    irq_add(II_TIMER1, AAS_FastTimer1InterruptHandler);
    irq_add(II_VBLANK, AAS_DoWork);
}


int main(void) 
{
    // REG_WAITCNT controls the pre-fetch buffer and the waitstates of the external cartridge bus.
    REG_WAITCNT = 0x4317; // This setting yields a *major* perf improvement over the defaults; cf. https://problemkaputt.de/gbatek.htm#gbatechnicaldata (last retrieved 2021-05-28).
    sqran(2001);
    irq_init(NULL);

    audioInit();
    globalsInit();
    drawInit();
    mathInit();
    timerInit();
    modelInit();
    scenesInit();
    logutilsInit(6);

    AAS_MOD_Play(AAS_DATA_MOD_BuxWV250);
    
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
