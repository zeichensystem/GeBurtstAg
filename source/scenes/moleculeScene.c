
#include <tonc.h>

#include "moleculeScene.h"
#include "../scene.h"
#include "../globals.h"
#include "../timer.h"
#include "../render/draw.h"


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

void moleculeSceneDraw(void) 
{
    lightDirection = (Vec3){.x=int2fx(3), .y=int2fx(-4), .z=int2fx(-3)};
    lightDirection = vecUnit(lightDirection);
    drawBefore(&camera);
    m5ScaledFill(RGB15(30, 20, 22));
    ModelDrawLightingData lightDataPoint = {.type=LIGHT_POINT, .light.point=&camera.pos, .attenuation=&lightAttenuation200};
    ModelDrawLightingData lightDataDir = {.type=LIGHT_DIRECTIONAL, .light.directional=&lightDirection, .attenuation=NULL};
    drawModelInstancePools(&modelPool, 1, &camera, lightDataDir);


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
