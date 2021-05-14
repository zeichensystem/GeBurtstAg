#ifndef RASTER_GEOMETRY_H
#define RASTER_GEOMETRY_H

#include <tonc.h>
#include "math.h"

typedef enum PolygonShadingType { 
    SHADING_FLAT_LIGHTING=1,
    SHADING_FLAT=2,
    SHADING_WIREFRAME=4,
} PolygonShadingType;


typedef struct RasterPoint {
    int x, y;
    // bool invisible;
} RasterPoint; 

typedef struct RasterTriangle {
    RasterPoint vert[3];
    FIXED centroidZ;
    COLOR color;
    PolygonShadingType shading;
} RasterTriangle;

typedef struct Triangle {
    Vec3 vert[3];
    FIXED centroidZ;
    COLOR color;
    PolygonShadingType shading;

} Triangle;

#endif