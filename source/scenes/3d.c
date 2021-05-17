#include <tonc.h>
#include <math.h>
#include <stdlib.h>

#include "test.h"
#include "../globals.h"
#include "../math.h"
#include "../camera.h"
#include "../draw.h"
#include "../model.h"
#include "../logutils.h"
#include "../timer.h"

#include "../data/headModel.h"
#include "../data/suzanneModel.h"


#define NUM_CUBES 9
#define NUM_CUBES_SQRT 3

EWRAM_DATA static ModelInstance __cubeBuffer[NUM_CUBES];
static ModelInstancePool cubePool;

EWRAM_DATA static ModelInstance __headBuffer[3];
static ModelInstancePool headPool;

static Camera camera;
static Vec3 lightDirection;
static Timer timer;
static ModelInstance *weirdHead, *weirdHead2;

static Vec3 playerHeading;
static ANGLE_FIXED_12 playerAngle;

static int perfDrawID, perfProjectID, perfSortID;


static void videoModeInit(void) {
        g_mode = DCNT_MODE5;
        REG_DISPCNT = g_mode | DCNT_BG2;
        setDispScaleM5Scaled();
}

void scene3dInit(void) 
{     
        videoModeInit();

        headModelInit(); 
        // suzanneModelInit();

        camera = cameraNew((Vec3){.x=int2fx(0), .y=int2fx(0), .z=int2fx(20)}, float2fx(M_PI / 180. * 43), float2fx(1.f), float2fx(42.f), g_mode);
        timer = timerNew(TIMER_MAX_DURATION, TIMER_REGULAR);
        perfDrawID = performanceDataRegister("Drawing");
        perfProjectID = performanceDataRegister("3d-math");
        perfSortID = performanceDataRegister("Polygon depth sort");
        lightDirection = (Vec3){.x=int2fx(3), .y=int2fx(-4), .z=int2fx(-3)};
        lightDirection = vecUnit(lightDirection);
        
        cubePool = modelInstancePoolNew(__cubeBuffer, sizeof __cubeBuffer / sizeof __cubeBuffer[0]);
        headPool = modelInstancePoolNew(__headBuffer, sizeof __headBuffer / sizeof __headBuffer[0]);
        
        // Grid of cubes:
        FIXED size = int2fx(4);
        FIXED padding = size >> 1;
        FIXED x_start =  -fxdiv(fxmul(int2fx(NUM_CUBES_SQRT), size) + fxmul(int2fx(NUM_CUBES_SQRT - 1), padding), int2fx(2)); 
        FIXED z = 0;
        Vec3 cubesCenter;
        for (int i = 0; i < NUM_CUBES; ++i) {
             if (i && i % (NUM_CUBES_SQRT) == 0) {
                     z -= size + padding;
             }
             FIXED x = x_start + (i % NUM_CUBES_SQRT) * (size + padding);
             if (i == (NUM_CUBES / 2)  ) { // Assumes NUM_CUBES is uneven (we want to leave a "hole" in the center of the grid).
                cubesCenter.x = x;
                cubesCenter.z = z;
                cubesCenter.y = 0;
             } else {
                modelCubeNewInstance(&cubePool, (Vec3){.x=x , .y=int2fx(0), .z=z }, size, SHADING_FLAT_LIGHTING);
             }
        }

        weirdHead = modelInstanceAdd(&headPool, headModel, &cubesCenter, 620, 0, deg2fxangle(-62), 0, SHADING_FLAT_LIGHTING);
        weirdHead2 = modelInstanceAdd(&headPool, headModel, &cubesCenter, 620, 0, deg2fxangle(62), deg2fxangle(180), SHADING_FLAT_LIGHTING);

        // modelInstanceAdd(&headPool, headModel, &(Vec3){.x=int2fx(6), .y=0, .z=0}, int2fx(1), 0, 0, 0, SHADING_FLAT_LIGHTING);
        // modelInstanceAdd(&headPool, headModel, &(Vec3){.x=int2fx(-6), .y=0, .z=0}, int2fx(1), 0, 0, 0, SHADING_FLAT_LIGHTING);
}        


void scene3dUpdate(void) 
{
        timerTick(&timer);
        for (int i = 0; i < NUM_CUBES; ++i) {
                FIXED_12 dir = i % 2 ? int2fx12(-1) : int2fx12(1);
                cubePool.instances[i].state.yaw -= fx12mul(dir, fx12mul(timer.deltatime, deg2fxangle(80)) );
                cubePool.instances[i].state.pitch -= fx12mul(int2fx12(1), fx12mul(timer.deltatime, deg2fxangle(120)) );
                cubePool.instances[i].state.roll -= fx12mul(int2fx12(1), fx12mul(timer.deltatime, deg2fxangle(110)) );
                // cubePool.instances[i].state.pos.y = fxmul(sinFx(cubePool.instances[i].state.pos.x * 2 + cubePool.instances[i].state.pos.z* 2 +  fx12mul(timer.time, deg2fxangle(250)  )), int2fx(3));
                // cubePool.instances[i].state.scale = int2fx(8) +  fxmul(sinFx( fx12mul(timer.time, deg2fxangle(360)  )), int2fx(2));
        }
        weirdHead->state.yaw -= fx12mul(int2fx12(1), fx12mul(timer.deltatime, deg2fxangle(80)) );
        weirdHead2->state.yaw += fx12mul(int2fx12(1), fx12mul(timer.deltatime, deg2fxangle(80)) );

        weirdHead->state.pos.y = fxmul(sinFx(fx12mul(timer.time, deg2fxangle(320))), int2fx(1)) - int2fx(4);
        weirdHead2->state.pos.y = fxmul(cosFx(fx12mul(timer.time, deg2fxangle(320))), int2fx(1)) + int2fx(4);


        // Calculate the point to lookAt from the heading of the player/camera.
        playerAngle += fx12mul(-int2fx12(key_tri_shoulder()), deg2fxangle(fx12Tofx(timer.deltatime >> 1)));
        FIXED rotmat[16];
        matrix4x4createRotY(rotmat, playerAngle);
        playerHeading = vecTransformed(rotmat, (Vec3){.x=0, .y=0, .z = int2fx(-1)});
        Vec3 right = vecCross(playerHeading, (Vec3){0, int2fx(1), 0});
        FIXED velX = fxmul(int2fx(key_tri_horz()), fx12Tofx(timer.deltatime << 5));
        FIXED velZ = fxmul(-int2fx(key_tri_vert()), fx12Tofx(timer.deltatime << 5));
        camera.pos.x += fxmul(velX, right.x) + fxmul(velZ, playerHeading.x);  
        camera.pos.z += fxmul(velX, right.z) + fxmul(velZ, playerHeading.z );
        camera.pos.y += fxmul(int2fx(key_tri_fire()), fx12Tofx(timer.deltatime << 4));
        camera.lookAt = (Vec3){.x = camera.pos.x + playerHeading.x, .y = camera.pos.y + playerHeading.y, .z = camera.pos.z + playerHeading.z};

        if (key_held(KEY_START)) {
                camera.lookAt = (Vec3){.x= headPool.instances[0].state.pos.x, .y=0, .z=headPool.instances[0].state.pos.z};
        }
}

bool toggle = false;
void scene3dDraw(void) 
{
        drawBefore(&camera);
        memset32(vid_page, dup16(CLR_BLACK), ((M5_SCALED_H-0) * M5_SCALED_W)/2);

        ModelDrawLightingData lightDataPoint = {.type=LIGHT_POINT, .light.point=&camera.pos, .attenuation=&lightAttenuation100};
        ModelDrawLightingData lightDataDir = {.type=LIGHT_DIRECTIONAL, .light.directional=&lightDirection, .attenuation=NULL};

        if (key_hit(KEY_SELECT)) {
                toggle = !toggle;
        }

        ModelInstancePool pools[2] = {headPool, cubePool};
        if (!toggle)
                drawModelInstancePools(pools, 2, &camera, lightDataDir);

        else
                drawModelInstancePools(pools, 2, &camera, lightDataPoint);

}

void scene3dStart(void) {
        timerStart(&timer);
}

void scene3dPause(void) {
        timerStop(&timer);
}

void scene3dResume(void) {
       videoModeInit();
        timerResume(&timer);
}
