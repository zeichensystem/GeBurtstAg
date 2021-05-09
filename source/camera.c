#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <tonc_bios.h>
#include <tonc_video.h>

#include "camera.h"
#include "math.h"
#include "logutils.h"
#include "model.h"
#include "globals.h"

static FIXED tmpmat[16];


Camera cameraNew(Vec3 pos, FIXED fov, FIXED near, FIXED far, int mode) 
{
    Camera new;
    // Read only: 
    int w, h;
    switch (mode) {
        case DCNT_MODE5:
            w = M5_SCALED_W;
            h = M5_SCALED_H;
        break;
        case DCNT_MODE4:
            w = M4_WIDTH;
            h = M4_HEIGHT;
        break;
        default: 
            panic("camera.c: No valid mode.");
            w = M5_SCALED_W;
            h = M5_SCALED_H;
        break;
    }
    new.canvasWidth = int2fx(w);
    new.canvasHeight = int2fx(h);
    new.viewportWidth = float2fx(w / 100.f);
    new.viewportHeight = float2fx(h / 100.f);
    new.aspect = fxdiv(new.viewportWidth, new.viewportHeight);
    new.fov = fov;
    new.near = near;
    new.far = far;
    matrix4x4setIdentity(new.cam2world);
    matrix4x4setIdentity(new.world2cam);
    // Read/write: 
    new.pos = pos;
    new.yaw = int2fx12(0);
    new.pitch = int2fx12(0);
    new.roll = int2fx12(0);
    new.lookAt = (Vec3){.x=0, .y=0, .z=0};
    cameraComputePerspectiveMatrix(&new);
    return new;
}


static void cameraComputeRotMatrix(Camera *cam, FIXED result[16]) 
{
    // TODO: precompute yawPitchRollmatrix
    // ROTMAT =  yaw * pitch * roll
    matrix4x4createRotY(result, cam->yaw);
    matrix4x4createRotX(tmpmat, cam->pitch);
    matrix4x4Mul(result, tmpmat);
    matrix4x4createRotZ(tmpmat, cam->roll);
    matrix4x4Mul(result, tmpmat);
}


void cameraComputeWorldToCamSpace(Camera *cam) 
{
    matrix4x4setIdentity(cam->world2cam);
    // Compute new basis of the matrix from out lookAt point:
    Vec3 forward = vecUnit(vecSub(cam->pos, cam->lookAt));
    Vec3 tmp = {.x = 0, .y = int2fx(1), .z =0};
    Vec3 right; 
    Vec3 up;
    FIXED eps = 25; // Is about 0.009; assumes FIX_SHIFT == 8
    // TODO/FIXME: Handling of this special case (camera looking completely down/up) *does not work* at all for non-static scenes (interpolation is messed up).
    if (ABS(forward.x - tmp.x) < eps && ABS(forward.y - tmp.y) < eps && ABS(forward.z - tmp.z) < eps ) { // Special case where the camera looks straight up/down and the cross product "fails".
        mgba_printf("camera.c: cameraComputeWorldToCamSpace: Handle singularity");
        right = vecUnit((Vec3){.x=forward.y, .y=0, .z=0});
        up = vecUnit(vecCross(forward, right));
    } else {
        right = vecUnit(vecCross(tmp, forward));
        up = vecUnit(vecCross(forward, right));
    }
    matrix4x4SetBasis(cam->world2cam, right, up, forward);

    // Compute rotation matrix_ 
    FIXED rotmat[16];
    cameraComputeRotMatrix(cam, rotmat);
    matrix4x4Mul(cam->world2cam, rotmat);

    // Invert the matrix so we get the world-to-camera matrix from our current camera-to-world matrix. 
    // (The transposition of square orthonormal matrices is equivalent to their inversion. If something goes wrong, our matrix is not orthonormal; try the numerical solution.)
    matrix4x4Transpose(cam->world2cam); 

    // Compute translation matrix and put it into 'transMat'
    FIXED transMat[16];
    matrix4x4setIdentity(transMat);
    matrix4x4SetTranslation(transMat, (Vec3){.x=-cam->pos.x, .y=-cam->pos.y, .z=-cam->pos.z} ); 

    // Post multiply the translation matrix with the rotation matrix.
    matrix4x4Mul(cam->world2cam, transMat); // world2cam' = world2cam  * transMat 
}


void cameraComputePerspectiveMatrix(Camera *cam) 
{ 
    FIXED near = cam->near;
    FIXED far = cam->far;
    FIXED top =  fxmul(float2fx(tan(fx2float(cam->fov) / 2. )), near);
    FIXED bottom = -top;
    FIXED right = fxmul(top, cam->aspect);
    FIXED left = -right;

    FIXED persp[16] = {
        fxdiv( fxmul(near, int2fx(2)), right - left), 0, fxdiv(right + left, right - left), 0,
        0, fxdiv(fxmul(near, int2fx(2)), top - bottom), fxdiv(top + bottom, top - bottom), 0,
        0, 0, -fxdiv(far + near, far - near), -fxdiv(fxmul(near, fxmul(int2fx(2), far)), far - near),
        0, 0, int2fx(-1), 0
    };

    FIXED viewport2image[16] = {
        // fxmul(cam->aspect): not sure, but seems to work for now?
        fxmul(cam->aspect, fxmul(float2fx(0.5), fxdiv(cam->canvasWidth, cam->viewportWidth))), 0, 0,  fxdiv(cam->canvasWidth, int2fx(2)), 
        0, -fxmul(float2fx(0.5), fxdiv(cam->canvasHeight, cam->viewportHeight)), 0, fxdiv(cam->canvasHeight, int2fx(2)), 
        0, 0, int2fx(1), 0,
        0, 0, 0, int2fx(1)
    };
    matrix4x4Mul(viewport2image, persp);
    memcpy(cam->perspMat, viewport2image, sizeof(persp));
}