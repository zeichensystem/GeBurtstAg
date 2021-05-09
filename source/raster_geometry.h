#ifndef RASTER_GEOMETRY_H
#define RASTER_GEOMETRY_H

#include <tonc.h>
#include "math.h"

typedef struct RasterPoint {
    int x, y;
    // bool invisible;
} RasterPoint; 

typedef struct RasterTriangle {
    RasterPoint vert[3];
    FIXED centroidZ;
    COLOR color;
} RasterTriangle;

typedef struct Triangle {
    Vec3 vert[3];
    FIXED centroidZ;
    COLOR color;
} Triangle;

#endif