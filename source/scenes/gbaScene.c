
#include <tonc.h>

#include "gbaScene.h"
#include "../scene.h"
#include "../globals.h"
#include "../timer.h"
#include "../render/draw.h"

#include "../data/gbaModel.h"

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

    camera = cameraNew((Vec3){.x=int2fx(0), .y=int2fx(0), .z=int2fx(50)}, CAMERA_VERTICAL_FOV_43_DEG, int2fx(1), int2fx(200), g_mode);
    lightDirection = (Vec3){.x=int2fx(3), .y=int2fx(-4), .z=int2fx(-3)};
    lightDirection = vecUnit(lightDirection);
    gbaPool = modelInstancePoolNew(__gbaBuffer, sizeof __gbaBuffer / sizeof __gbaBuffer[0]);
    gbaModelInstance = modelInstanceAddVanilla(&gbaPool, gbaModel, &(Vec3){.x=0, .y=0, .z=0}, int2fx(10), SHADING_FLAT);
}

void gbaSceneUpdate(void) 
{
    timerTick(&timer);
    gbaModelInstance->state.yaw -= fx12mul(timer.deltatime, deg2fxangle(140));
    // gbaModelInstance->state.roll += fx12mul(timer.deltatime, deg2fxangle(120));
}

void gbaSceneDraw(void) 
{
    drawBefore(&camera);
    m5ScaledFill(CLR_TEAL);
    ModelDrawLightingData lightDataPoint = {.type=LIGHT_POINT, .light.point=&camera.pos, .attenuation=&lightAttenuation100};
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
