
#include <tonc.h>

#include "benchmarkScene.h"
#include "../globals.h"
#include "../scene.h"
#include "../camera.h"
#include "../timer.h"
#include "../render/draw.h"

#include "../../data-models/suzanneModel.h"

static Timer timer;
static Camera cam;
static Vec3 lightDirection;

#define MAX_MONKEY_NUM 2
ModelInstance monkeyPoolBuffer[2];
ModelInstancePool monkeyPool;
ModelInstance *monkey, *cube;

/* 
    Current per-frame performance (it's above 30 FPS!): 
    - directional: 
    [INFO] GBA Debug: draw:c pre-rasterisation: 9.322998 ms (65 samples)
    [INFO] GBA Debug: draw.c: total: 27.709961 ms (65 samples)

    - point-light:
    [INFO] GBA Debug: draw:c pre-rasterisation: 9.506104 ms (64 samples)
    [INFO] GBA Debug: draw.c: total: 27.893066 ms (64 samples)
*/ 

void benchmarkSceneInit(void) 
{ 
    timer = timerNew(TIMER_MAX_DURATION, TIMER_REGULAR);
    suzanneModelInit();
    cam = cameraNew((Vec3){.x=0, .y=0, .z=0}, CAMERA_VERTICAL_FOV_43_DEG, int2fx(1), int2fx(64), g_mode);
    monkeyPool = modelInstancePoolNew(monkeyPoolBuffer, MAX_MONKEY_NUM);
    lightDirection = (Vec3){.x = 0, .y = 0, .z=int2fx(-3)};
    lightDirection = vecUnit(lightDirection);

    monkey = modelInstanceAddVanilla(&monkeyPool, suzanneModel, &(Vec3){.x=int2fx(0), .y=0, .z=int2fx(-4)}, int2fx(1), SHADING_FLAT_LIGHTING);
    cube = modelInstanceAddVanilla(&monkeyPool, cubeModel, &(Vec3){.x=float2fx(-1.6), .y=0, .z=int2fx(-3)}, int2fx(2), SHADING_FLAT_LIGHTING);
    cube->state.yaw = deg2fxangle(-45);
    cube->state.pitch = deg2fxangle(-45);
    cube->state.roll = deg2fxangle(12);

    monkey->state.yaw = deg2fxangle(-10);
}

void benchmarkSceneUpdate(void) 
{
    timerTick(&timer);

    cam.lookAt = monkey->state.pos;
}

static bool lightToggle;
IWRAM_CODE_ARM void benchmarkSceneDraw(void) 
{
    drawBefore(&cam);
    m5ScaledFill(CLR_BLACK);
    ModelDrawLightingData lightDataDir = {.type=LIGHT_DIRECTIONAL, .light.directional=&lightDirection, .attenuation=&lightAttenuation160};
    ModelDrawLightingData lightDataPoint = {.type=LIGHT_POINT, .light.directional=&cam.pos, .attenuation=&lightAttenuation160};
    if (key_hit(KEY_A)) {
        lightToggle = !lightToggle;
    }
    if (!lightToggle) {
        drawModelInstancePools(&monkeyPool, 1, &cam, lightDataDir);
    } else {
        drawModelInstancePools(&monkeyPool, 1, &cam, lightDataPoint);
    }
}

void benchmarkSceneStart(void) 
{
    timerStart(&timer);
    videoM5ScaledInit();
}

void benchmarkScenePause(void) 
{
    timerStop(&timer);
}

void benchmarkSceneResume(void) 
{
    timerResume(&timer);
    videoM5ScaledInit();
}
