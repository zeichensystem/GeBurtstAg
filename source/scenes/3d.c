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

#define NUM_CUBES 9

EWRAM_DATA static ModelInstance cubes[NUM_CUBES];

static Camera camera;
static Vec3 lightDirection;
static Timer timer;

static Vec3 playerHeading;
static ANGLE_FIXED_12 playerAngle;

static int perfDrawID, perfProjectID, perfSortID;


void scene3dInit(void) 
{     
        camera = cameraNew((Vec3){.x=int2fx(0), .y=int2fx(0), .z=int2fx(64)}, float2fx(M_PI / 180. * 43), float2fx(1.f), float2fx(54.f), g_mode);
        timer = timerNew(TIMER_MAX_DURATION, TIMER_REGULAR);
        perfDrawID = performanceDataRegister("Drawing");
        perfProjectID = performanceDataRegister("3d-math");
        perfSortID = performanceDataRegister("Polygon depth sort");
        lightDirection = (Vec3){.x=int2fx(3), .y=int2fx(-4), .z=int2fx(-3)};
        lightDirection = vecUnit(lightDirection);
        
        int size = 8;
        for (int i = 0; i < NUM_CUBES; ++i) {
             cubes[i] = modelCubeNewInstance((Vec3){.x=int2fx(size * 1.5f * (i % 3) ), .y=int2fx(0), .z=int2fx(size * 1.5f * (i / 3)) }, int2fx(size));
        }
}        


void scene3dUpdate(void) 
{
        timerTick(&timer);

        for (int i = 0; i < NUM_CUBES; ++i) {
                // FIXED_12 dir = i % 2 ? int2fx12(-1) : int2fx12(1);
                // cubes[i].yaw -= fx12mul(dir, fx12mul(timer.deltatime, deg2fxangle(80)) );
                // cubes[i].pitch -= fx12mul(int2fx12(1), fx12mul(timer.deltatime, deg2fxangle(120)) );
                // cubes[i].roll -= fx12mul(int2fx12(1), fx12mul(timer.deltatime, deg2fxangle(110)) );
                // cubes[i].pos.y = fxmul(sinFx(cubes[i].pos.x + cubes[i].pos.z +  fx12mul(timer.time, deg2fxangle(360)  )), int2fx(5));
                // cubes[i].scale = int2fx(8) +  fxmul(sinFx( fx12mul(timer.time, deg2fxangle(360)  )), int2fx(2));
        }

        cubes[4].pos.y = fxmul(sinFx( fx12mul(timer.time, deg2fxangle(360)  )), int2fx(5));
        cubes[4].yaw -= fx12mul(int2fx12(1), fx12mul(timer.deltatime, deg2fxangle(100)) );
        cubes[4].pitch -= fx12mul(int2fx12(1), fx12mul(timer.deltatime, deg2fxangle(120)) );
        cubes[4].roll -= fx12mul(int2fx12(1), fx12mul(timer.deltatime, deg2fxangle(110)) );

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

        if (key_is_down(KEY_START)) {
                camera.lookAt = (Vec3){.x= cubes[4].pos.x, .y=0, .z=cubes[4].pos.z};
        }
}


void scene3dDraw(void) 
{
        drawBefore(&camera);
        memset32(vid_page, dup16(CLR_BLACK), ((M5_SCALED_H-0) * M5_SCALED_W)/2);
        const ModelDrawOptions opts = {.lightDirectional=NULL , .lightPoint=&camera.pos, .lightPointAttenuation=true, .shading=SHADING_WIREFRAME, .wireframeColor=CLR_MAG};
        drawModelInstances(&camera, cubes, NUM_CUBES, &opts);
}


void scene3dStart(void) {
        timerStart(&timer);
}

void scene3dPause(void) {
        timerStop(&timer);
}

void scene3dResume(void) {
        timerResume(&timer);
}
