
#include <tonc.h>

#include "gbaScene.h"
#include "../scene.h"
#include "../globals.h"
#include "../timer.h"
#include "../render/draw.h"

#include "../../data-models/gbaModel.h"

static Timer timer;

#define NUM_GBA 1
EWRAM_DATA static ModelInstance __gbaBuffer[NUM_GBA];
static ModelInstancePool gbaPool;
static ModelInstance *gbaModelInstance;

static Camera camera;
static Vec3 lightDirection;


void gbaSceneInit(void) 
{ 
    timer = timerNew(TIMER_MAX_DURATION, TIMER_REGULAR);
    gbaModelInit();

    camera = cameraNew((Vec3){.x=int2fx(0), .y=int2fx(0), .z=int2fx(120)}, CAMERA_VERTICAL_FOV_43_DEG, int2fx(1), int2fx(256), g_mode);
    lightDirection = (Vec3){.x=int2fx(3), .y=int2fx(-4), .z=int2fx(-3)};
    lightDirection = vecUnit(lightDirection);
    gbaPool = modelInstancePoolNew(__gbaBuffer, sizeof __gbaBuffer / sizeof __gbaBuffer[0]);
    gbaModelInstance = modelInstanceAddVanilla(&gbaPool, gbaModel, &(Vec3){.x=0, .y=0, .z=0}, int2fx(10), SHADING_FLAT);
}

void gbaSceneUpdate(void) 
{
    timerTick(&timer);
    // gbaModelInstance->state.pitch +=  fx12mul(timer.deltatime, deg2fxangle(320));
    gbaModelInstance->state.yaw -=  fx12mul(timer.deltatime, deg2fxangle(420));
    gbaModelInstance->state.roll += fx12mul(timer.deltatime, deg2fxangle(210));
    camera.pos.z = 80 * sinFx(timer.time * 5);
    camera.pos.x =  200 * cosFx(timer.time * 5);
    camera.lookAt = (Vec3){0,0,0};
}

INLINE void beGay(void) 
{
    const COLOR RAINBOW[6] = {RGB15(31, 0, 3), RGB15(31, 20, 5), RGB15(31, 31, 8), RGB15(0, 16, 3), RGB15(0, 0, 30), RGB15(16, 0, 15)};
    for (int i = 0; i < 6; ++i) {
        m5_rect(0, i * 16, 160, 3 + i * 16 + 16, RAINBOW[i]);
    }    
}

void gbaSceneDraw(void) 
{
    drawBefore(&camera);
    beGay();
    // ModelDrawLightingData lightDataPoint = {.type=LIGHT_POINT, .light.point=&camera.pos, .attenuation=&lightAttenuation100};
    ModelDrawLightingData lightDataDir = {.type=LIGHT_DIRECTIONAL, .light.directional=&lightDirection, .attenuation=NULL};

    drawModelInstancePools(&gbaPool, 1, &camera, lightDataDir);

}

void gbaSceneStart(void) 
{
    timerStart(&timer);
    videoM5ScaledInit();
}

void gbaScenePause(void) 
{
    timerStop(&timer);
}

void gbaSceneResume(void) 
{
    timerResume(&timer);
    videoM5ScaledInit();
}
