#ifndef CAMERA_H
#define CAMERA_H

#include "math.h"
#include "raster_geometry.h"

// 43 degrees converted to radians (float2fx(PI_FLT / 180. * 43)) and .8 fixed point.
#define CAMERA_VERTICAL_FOV_43_DEG 192

typedef struct Camera {
    FIXED perspMat[16];
    FIXED viewport2imageMat[16];
    FIXED perspFacX, perspFacY, viewportTransFacX, viewportTransFacY, viewportTransAddX, viewportTransAddY;
    FIXED cam2world[16]; 
    FIXED world2cam[16];
    Vec3 pos;
    ANGLE_FIXED_12 yaw, pitch, roll;
    Vec3 lookAt;

    FIXED canvasWidth, canvasHeight;
    FIXED viewportWidth, viewportHeight;
    FIXED aspect;
    FIXED fov, near, far;
} ALIGN4 Camera;

Camera cameraNew(Vec3 pos, FIXED fov, FIXED near, FIXED far, int mode);
IWRAM_CODE_ARM void cameraComputePerspectiveMatrix(Camera *cam);
IWRAM_CODE_ARM void cameraComputeWorldToCamSpace(Camera *cam);

#endif