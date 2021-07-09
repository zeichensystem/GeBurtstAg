
#include <tonc.h>

#include "subwayScene.h"
#include "../scene.h"
#include "../globals.h"
#include "../timer.h"
#include "../render/draw.h"
#include "../math.h"

#include "../../data-models/subwayModel.h"

static Timer timer;

#define MAX_MODELS 4
EWRAM_DATA static ModelInstance __modelBuffer[MAX_MODELS];
static ModelInstancePool modelPool;
static ModelInstance *subwayInstance;

static Camera camera;
static Vec3 lightDirection;
void subwaySceneInit(void) 
{ 
    timer = timerNew(TIMER_MAX_DURATION, TIMER_REGULAR);
    subwayModelInit();
    camera = cameraNew((Vec3){.x=int2fx(0), .y=int2fx(6), .z=int2fx(42)}, CAMERA_VERTICAL_FOV_43_DEG, int2fx(1), int2fx(128), g_mode);
    lightDirection = (Vec3){.x=int2fx(3), .y=int2fx(-4), .z=int2fx(-3)};
    lightDirection = vecUnit(lightDirection);
    modelPool = modelInstancePoolNew(__modelBuffer, sizeof __modelBuffer / sizeof __modelBuffer[0]);
    subwayInstance = modelInstanceAddVanilla(&modelPool, subwayModel, &(Vec3){.x=0, .y=0, .z=0}, int2fx(2), SHADING_FLAT);
    subwayInstance->state.backfaceCulling = false;

}

void subwaySceneUpdate(void) 
{
    timerTick(&timer);
    const FIXED_12 camMoveDuration = int2fx12(10);
    const FIXED_12 startTimeOffset = 1000;  
    FIXED_12 t =  (fx12div(timer.time + startTimeOffset , camMoveDuration));
    mgba_printf("t %f", fx12ToFloat(t));
    camera.pos.z = lerpSmooth(int2fx(-42), int2fx(116), t);
    camera.pos.x =  lerpSmooth(int2fx(32), int2fx(12), t);
int2fx(24);


}

void subwaySceneDraw(void) 
{
    drawBefore(&camera);
    m5ScaledFill(CLR_BLACK);
    // ModelDrawLightingData lightDataPoint = {.type=LIGHT_POINT, .light.point=&camera.pos, .attenuation=&lightAttenuation100};
    ModelDrawLightingData lightDataDir = {.type=LIGHT_DIRECTIONAL, .light.directional=&lightDirection, .attenuation=NULL};
    drawModelInstancePools(&modelPool, 1, &camera, lightDataDir);
}

void subwaySceneStart(void) 
{
    // This function is called only once, namely when your scene is first entered. Later, the resume function will be called instead. 
    timerStart(&timer);
    videoM5ScaledInit();
}

void subwayScenePause(void) 
{
    timerStop(&timer);
}

void subwaySceneResume(void) 
{
    timerResume(&timer);
    videoM5ScaledInit();
}
