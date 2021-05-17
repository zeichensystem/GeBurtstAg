#ifndef CAMERA_H
#define CAMERA_H

#include "math.h"
#include "raster_geometry.h"

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
IWRAM_CODE void cameraComputePerspectiveMatrix(Camera *cam);
IWRAM_CODE void cameraComputeWorldToCamSpace(Camera *cam);

#endif