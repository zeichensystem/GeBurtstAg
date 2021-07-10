
#include <tonc.h>

#include "moleculeScene.h"
#include "../scene.h"
#include "../globals.h"
#include "../timer.h"
#include "../render/draw.h"

#include "../../data-audio/AAS_Data.h"



static Timer timer;
#include "../../data-models/cpaModel.h"

EWRAM_DATA static ModelInstance __modelBuffer[1];
static ModelInstancePool modelPool;
static ModelInstance *moleculeInstance;

static Camera camera;
static Vec3 lightDirection;

static const int FAR = 200;

void moleculeSceneInit(void) 
{     
    cpaModelInit();
    timer = timerNew(TIMER_MAX_DURATION, TIMER_REGULAR);
    camera = cameraNew((Vec3){.x=int2fx(0), .y=int2fx(0), .z=int2fx(0)}, CAMERA_VERTICAL_FOV_43_DEG, int2fx(1), int2fx(FAR), g_mode);

    modelPool = modelInstancePoolNew(__modelBuffer, sizeof __modelBuffer / sizeof __modelBuffer[0]);

    moleculeInstance = modelInstanceAddVanilla(&modelPool, cpaModel, &(Vec3){.x=0, .y=0, .z=0}, int2fx(1), SHADING_WIREFRAME);
    moleculeInstance->state.backfaceCulling = false;

}

void moleculeSceneUpdate(void) 
{
    timerTick(&timer);

    camera.pos.x = 80 * sinFx(fx12mul(timer.time, int2fx12(8)) );
    camera.pos.z = 80 * cosFx(fx12mul(timer.time, int2fx12(8)) );
    camera.pos.y = 120 * cosFx(fx12mul(timer.time, int2fx12(4)) );
}

static bool musicSwitched = false;
void moleculeSceneDraw(void) 
{
    lightDirection = (Vec3){.x=int2fx(3), .y=int2fx(-4), .z=int2fx(-3)};
    lightDirection = vecUnit(lightDirection);
    drawBefore(&camera);
    m5ScaledFill(RGB15(30, 20, 22));
    ModelDrawLightingData lightDataPoint = {.type=LIGHT_POINT, .light.point=&camera.pos, .attenuation=&lightAttenuation200};
    ModelDrawLightingData lightDataDir = {.type=LIGHT_DIRECTIONAL, .light.directional=&lightDirection, .attenuation=NULL};
    drawModelInstancePools(&modelPool, 1, &camera, lightDataDir);

    // Lol. 
    if (timer.time > int2fx12(2) && timer.time < int2fx12(4)) {
        m5_puts(10, 10, "audio", CLR_WHITE);
        m5_puts(10, 20, "Apex Audio System",  RGB15(10, 25, 31));
    } else if (timer.time >= int2fx12(4) && timer.time < int2fx12(6)) {
        m5_puts(10, 10, "rasteriser", CLR_WHITE);
        m5_puts(10, 20, "fatmap.txt (MRI)",  RGB15(10, 25, 31));
    } else if (timer.time >= int2fx12(6) && timer.time < int2fx12(8)) {
        m5_puts(10, 10, "fast division", CLR_WHITE);
        m5_puts(10, 20, "gba-modern",  RGB15(10, 25, 31));
        m5_puts(10, 30, "(JoaoBaptMG)",  RGB15(10, 25, 31));
    } else if (timer.time >= int2fx12(8) && timer.time < int2fx12(10)) {
        m5_puts(10, 10, "GBA library:", CLR_WHITE);
        m5_puts(10, 20, "libtonc",  RGB15(10, 25, 31));
    } else if (timer.time >= int2fx12(10) && timer.time < int2fx12(12)) {
        m5_puts(10, 10, "math etc.", CLR_WHITE);
        m5_puts(10, 20, "wikipedia.org",  RGB15(10, 25, 31));
        m5_puts(10, 30, "sol.gfxile.net",  RGB15(10, 25, 31));

    } else if (timer.time >= int2fx12(12) && timer.time < int2fx12(14)) {
        m5_puts(10, 10, "samples", CLR_WHITE);
        m5_puts(10, 20, "ST-01",  RGB15(10, 25, 31));
        m5_puts(10, 30, "junglebreaks.co.uk",  RGB15(10, 25, 31));
    } else if (timer.time >= int2fx12(14) && timer.time < int2fx12(16)) {
        m5_puts(10, 10, "music based on", CLR_WHITE);
        m5_puts(10, 20, "BuxWV250",  RGB15(10, 25, 31));
    } else if (timer.time >= int2fx12(16) && timer.time < int2fx12(18)) {
        m5_puts(10, 10, "molecule model", CLR_WHITE);
        m5_puts(10, 20, "generated w. jsmol",  RGB15(10, 25, 31));
    } else if (timer.time >= int2fx12(18) && timer.time < int2fx12(20)) {
        m5_puts(10, 10, "toolchain", CLR_WHITE);
        m5_puts(10, 20, "devkitARM",  RGB15(10, 25, 31));
    } else if (timer.time >= int2fx12(20) && timer.time < int2fx12(22)) {
        m5_puts(10, 10, "emulator", CLR_WHITE);
        m5_puts(10, 20, "mGBA",  RGB15(10, 25, 31));
    } else if (timer.time >= int2fx12(22) && timer.time < int2fx12(24)) {
        m5_puts(10, 10, "mgba_printf", CLR_WHITE);
        m5_puts(10, 20, "Nick Sells",  RGB15(10, 25, 31));
        m5_puts(10, 30, "(adverseengineer)",  RGB15(10, 25, 31));

    } else if (timer.time >= int2fx12(24) && timer.time < int2fx12(26)) {
        m5_puts(10, 10, "for more", CLR_WHITE);
        m5_puts(10, 20, "CREDITS.md",  RGB15(10, 25, 31));
        m5_puts(10, 20, "CREDITS.md",  RGB15(10, 25, 31));

    } else if (timer.time >= int2fx12(28) && timer.time < int2fx12(30)) {
        m5_puts(24, 10, "happy birthday", CLR_WHITE);
    }  else if (timer.time >= int2fx12(32) && timer.time < int2fx12(34)) {
        m5_puts(2, 10, "you can kill me now", CLR_WHITE);
    } else if (timer.time >= int2fx12(36) && timer.time < int2fx12(38)) {
        m5_puts(10, 10, "thanks", CLR_WHITE);
    } else if (timer.time >= int2fx12(40) && timer.time < int2fx12(43)) {
        m5_puts(10, 10, "secret greets to", CLR_WHITE);
        m5_puts(10, 22, "Oli D.",  RGB15(10, 25, 31));
    }  else if (timer.time >= int2fx12(45) && timer.time < int2fx12(48)) {
        m5_puts(10, 10, "but now for real", CLR_WHITE);
    } else if (timer.time >= int2fx12(50) && timer.time < int2fx12(53)) {
        m5_puts(10, 10, "goodbye", CLR_WHITE);
    } else if (timer.time >= int2fx12(56) && timer.time < int2fx12(59)) {
        m5_puts(10, 10, "Go away!", CLR_WHITE);
        if (!musicSwitched) {
            AAS_MOD_Stop(AAS_DATA_MOD_BuxWV250);
            AAS_MOD_Play(AAS_DATA_MOD_aaa);
            musicSwitched = true;
        }
    }

}    


void moleculeSceneStart(void) 
{
    timerStart(&timer);
    videoM5ScaledInit();
}

void moleculeScenePause(void) 
{
    timerStop(&timer);
}

void moleculeSceneResume(void) 
{
    timerResume(&timer);
    videoM5ScaledInit();
}
