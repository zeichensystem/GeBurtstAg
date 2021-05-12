#ifndef CLIPPING_H
#define CLIPPING_H

#include "raster_geometry.h"

#define CLIPPING_MAX_POLY_LEN 12 // Very conservative; when clipping a triangle against the screen, we should get at most 7 vertices for the clipped n-gon.

/*
    Sutherland-Hodgman Polygon clipping against the screen. 
    Takes outputVertices (with index 0 to 3 filled by the triangle which we want to clip against the screen), and modifies it to contain the vertices of the n-gon clipped to the screen from 0 to n - 1.  
    Returns the number of vertices n of the clipped n-gon that are contained in outputVertices from 0 to n -1 (can be zero if the triangle is not visible on the screen after clipping). 
*/
int clipTriangleVerts2d(RasterPoint outputVertices[CLIPPING_MAX_POLY_LEN]); 

/* Returns true if the resulting line is visible on the screen. */
bool clipLineCohenSutherland(RasterPoint *a, RasterPoint *b);

#endif