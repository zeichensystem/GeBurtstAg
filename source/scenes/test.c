#include <tonc.h>
#include <stdlib.h>
#include <math.h>

#include "test.h"
#include "../globals.h"
#include "../camera.h"
#include "../draw.h"
#include "../model.h"
#include "../logutils.h"
#include "../timer.h"


#define NUM_CUBES 9
#define NUM_POINTS 200

EWRAM_DATA static ModelInstance __cubesBuffer[NUM_CUBES];
static ModelInstancePool cubePool;

static Camera camera;
static Vec3 lightDirection;
static Timer timer;

EWRAM_DATA static Vec3 points[NUM_POINTS];

static int perfDrawID, perfProjectID, perfSortID;


void sceneTestInit(void) {     
        setDispScaleM5Scaled();

        camera = cameraNew((Vec3){.x=int2fx(0), .y=int2fx(0), .z=int2fx(0)}, float2fx(M_PI / 180. * 43), int2fx(1), int2fx(256), g_mode);
        timer = timerNew(TIMER_MAX_DURATION, TIMER_REGULAR);
        perfDrawID = performanceDataRegister("Drawing");
        perfProjectID = performanceDataRegister("3d-math");
        perfSortID = performanceDataRegister("Polygon depth sort");
        lightDirection = (Vec3){.x=int2fx(5), .y=int2fx(-8), .z=int2fx(2)};
        lightDirection = vecUnit(lightDirection);
        
        cubePool = modelInstancePoolNew(__cubesBuffer, sizeof __cubesBuffer / sizeof __cubesBuffer[0]);
        int size = 12;
        for (int i = 0; i < NUM_CUBES; ++i) {
                modelInstanceAddVanilla(&cubePool, cubeModel, &(Vec3){.x=int2fx(size * (i % 3)), .y=int2fx(0), .z=int2fx(size * (i / 3))}, int2fx(size), SHADING_FLAT_LIGHTING );
        }

        for (int i = 0; i < NUM_POINTS; ++i) {  // Initialise stars.
                int dirx = rand() % 2 ? 1 : -1;
                int diry = rand() % 2 ? 1 : -1;
                int dirz = rand() % 2 ? 1 : -1;
                FIXED x = int2fx( (rand() % 8 + 1) * 9 );
                FIXED y = int2fx( (rand() % 8 + 1) * 9 );
                FIXED z = int2fx( (rand() % 8 + 1) * 9 );
                points[i] = (Vec3){(x * dirx), (y * diry), (dirz*z)};
        }
}


void sceneTestUpdate(void) 
{
        for (int i = 0; i < NUM_CUBES; ++i) {
                FIXED_12 dir = i % 2 ? int2fx12(-1) : int2fx12(1);
                cubePool.instances[i].state.yaw -= fx12mul(dir, fx12mul(timer.deltatime, deg2fxangle(120)) );
                cubePool.instances[i].state.pos.y = fxmul(sinFx( fx12mul(timer.time, deg2fxangle(120)) + fx2fx12(cubePool.instances[i].state.pos.z) + fx2fx12(cubePool.instances[i].state.pos.x)  ), int2fx(2));
        }

        cubePool.instances[4].state.pos.y = fxmul(sinFx( fx12mul(timer.time, deg2fxangle(360) )), int2fx(20));
        cubePool.instances[4].state.yaw -= fx12mul(int2fx12(1), fx12mul(timer.deltatime, deg2fxangle(100)) );
        cubePool.instances[4].state.pitch -= fx12mul(int2fx12(1), fx12mul(timer.deltatime, deg2fxangle(120)) );
        cubePool.instances[4].state.roll -= fx12mul(int2fx12(1), fx12mul(timer.deltatime, deg2fxangle(110)) );

        camera.lookAt = (Vec3){cubePool.instances[4].state.pos.x, 0, cubePool.instances[4].state.pos.z};
        camera.pos.x = cubePool.instances[4].state.pos.x + fxmul(cosFx(fx12mul(timer.time, deg2fxangle(160))), int2fx(64)); 
        camera.pos.z = cubePool.instances[4].state.pos.z + fxmul(sinFx(fx12mul(timer.time, deg2fxangle(160))), int2fx(64)); 
        camera.pos.y = int2fx(32) + fxmul(cosFx(timer.time << 2), int2fx(80)); 
        camera.roll += timer.deltatime * 2;
        camera.pitch = deg2fxangle(4);
        camera.yaw = deg2fxangle(2);
        timerTick(&timer);
}


void sceneTestDraw(void) 
{
        drawBefore(&camera);
        memset32(vid_page, dup16(CLR_BLACK), (M5_SCALED_H  * M5_SCALED_W)/2);	
        drawPoints(&camera, points, NUM_POINTS, CLR_WHITE);
        drawModelInstancePools(&cubePool, 1, &camera, (ModelDrawLightingData){.type=LIGHT_DIRECTIONAL, .light.directional=&lightDirection, .attenuation=NULL});
}


void sceneTestStart(void) {
        timerStart(&timer);
}

void sceneTestPause(void) {
        timerStop(&timer);
}

void sceneTestResume(void) {
        setDispScaleM5Scaled();
        timerResume(&timer);
}
