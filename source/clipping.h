#ifndef CLIPPING_H
#define CLIPPING_H

#include "raster_geometry.h"

#define CLIPPING_MAX_POLY_LEN 12 // Very conservative ; when clipping a triangle against the screen, we should get at most 7 vertices for the clipped n-gon

int clipTriangleVerts2d(RasterPoint outputVertices[CLIPPING_MAX_POLY_LEN]); 
bool clipLineCohenSutherland(RasterPoint *a, RasterPoint *b);

#endif