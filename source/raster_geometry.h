#ifndef RASTER_GEOMETRY_H
#define RASTER_GEOMETRY_H

#include <tonc.h>
#include <inttypes.h>
#include "math.h"

typedef enum PolygonShadingType { 
    SHADING_FLAT_LIGHTING,
    SHADING_FLAT,
    SHADING_WIREFRAME
} PolygonShadingType;


/* 
    We use RASTER_POINT_NEAR_FAR_CULL as a special value for x and y when the raster point is behind the near plane or beyond the far plane. 
*/
#define RASTER_POINT_NEAR_FAR_CULL INT16_MAX

typedef struct RasterPoint { 
    s16 x, y; 
} ALIGN4 RasterPoint; 

typedef struct RasterTriangle {
    RasterPoint vert[3];
    FIXED centroidZ;
    COLOR color;
    PolygonShadingType shading;
} ALIGN4 RasterTriangle;

typedef struct Triangle {
    Vec3 vert[3];
    FIXED centroidZ;
    COLOR color;
    PolygonShadingType shading;
} ALIGN4 Triangle;

#endif