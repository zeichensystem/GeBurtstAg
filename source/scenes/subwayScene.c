
#include <tonc.h>

#include "subwayScene.h"
#include "../scene.h"
#include "../globals.h"
#include "../timer.h"
#include "../render/draw.h"
#include "../math.h"

#include "../../data-models/subwayModel.h"
#include "../../data-models/treeModel.h"


static Timer timer;

// NUM_TREES must be divisible by 2.
#define NUM_TREES 20
#define MAX_MODELS (NUM_TREES + 1)
EWRAM_DATA static ModelInstance __modelBuffer[MAX_MODELS];
EWRAM_DATA static ModelInstance* trees[NUM_TREES];

static ModelInstancePool modelPool;
static ModelInstance *subwayInstance;

static Camera camera;
static Vec3 lightDirection;

static const int FAR = 202;

void subwaySceneInit(void) 
{ 
    timer = timerNew(TIMER_MAX_DURATION, TIMER_REGULAR);
    subwayModelInit();
    treeModelInit();
    
    camera = cameraNew((Vec3){.x=int2fx(0), .y=int2fx(6), .z=int2fx(42)}, CAMERA_VERTICAL_FOV_43_DEG, int2fx(1), int2fx(FAR), g_mode);
    lightDirection = (Vec3){.x=int2fx(3), .y=int2fx(-4), .z=int2fx(-3)};
    lightDirection = vecUnit(lightDirection);
    modelPool = modelInstancePoolNew(__modelBuffer, sizeof __modelBuffer / sizeof __modelBuffer[0]);

    subwayInstance = modelInstanceAddVanilla(&modelPool, subwayModel, &(Vec3){.x=0, .y=0, .z=0}, int2fx(2), SHADING_FLAT);
    subwayInstance->state.backfaceCulling = false;

    const int treeSpacing = 34; // Z-spacing.
    const int leftZOffset = 16; 
    for (int i = 0; i < NUM_TREES / 2; i+=2) {
        trees[i] = modelInstanceAddVanilla(&modelPool, treeModel, &(Vec3){.x=int2fx(28), .y=0, .z= i * int2fx(treeSpacing)}, int2fx(1) + 200, SHADING_FLAT); // Right.
        trees[i+1] = modelInstanceAddVanilla(&modelPool, treeModel, &(Vec3){.x=int2fx(-28), .y=0, .z= i * int2fx(treeSpacing - 4) + int2fx(leftZOffset)}, int2fx(1) + 200, SHADING_FLAT); // Left
        trees[i]->state.yaw = deg2fxangle(-56);
        trees[i+1]->state.yaw = deg2fxangle(-56);
    }
}

void subwaySceneUpdate(void) 
{
    timerTick(&timer);
    const FIXED_12 camMoveDuration = int2fx12(10);
    const FIXED camMoveDurationZ = int2fx12(15);
    const FIXED_12 startTimeOffset = 1000;  
    FIXED_12 t =  (fx12div(timer.time + startTimeOffset , camMoveDuration));
    FIXED_12 tZ =  (fx12div(timer.time + startTimeOffset, camMoveDurationZ ));

    if (tZ < int2fx12(1)) { // Camera outside. 
        camera.pos.z = lerpSmooth(int2fx(-35), int2fx(76), tZ);
        camera.pos.x =  lerpSmooth(int2fx(48), int2fx(16), t);
        camera.pos.y =  lerpSmooth(int2fx(0), int2fx(10), tZ);
    } else { // Camera inside
        FIXED_12 tInside = (fx12div(timer.time - camMoveDurationZ, int2fx12(8))); 
        camera.lookAt = (Vec3){.x = int2fx(-10), .y=int2fx(1), .z= lerpSmooth(int2fx(-2), int2fx(-16), tInside)};
        camera.pos = (Vec3){.x = 240, .y = int2fx(2), .z = int2fx(-5)};

        if (tInside > int2fx12(1)) {
            sceneSwitchTo(GBASCENE);
        }

    }

    const int treeSpeed = 142;
    for (int i = 0; i < NUM_TREES; ++i) {
        if (trees[i]->state.pos.z < int2fx(-FAR)) { // Tree is out of sight -> "respawn".
            trees[i]->state.pos.z = int2fx(120);
        } else {
            trees[i]->state.pos.z -=  fx12Tofx(fx12mul(timer.deltatime, int2fx12(treeSpeed)));
        }
        
    }

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
