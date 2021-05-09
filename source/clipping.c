#include <string.h>
#include <stdlib.h>
#include <tonc.h>

#include "clipping.h"
#include "globals.h"
#include "math.h"
#include "logutils.h"
#include "model.h"

#define RASTERPOINT_IN_BOUNDS_M5(vert) (vert.x >= 0 && vert.x < M5_SCALED_W && vert.y >= 0 && vert.y < M5_SCALED_H)

typedef enum ClipEdges {
    TOP_EDGE=0,
    BOTTOM_EDGE, 
    LEFT_EDGE, 
    RIGHT_EDGE
} ClipEdges;


RasterPoint calcIntersect2d(ClipEdges edge, RasterPoint current, RasterPoint prev) 
{ 
    /* 
        Calculates line intersections against the screen borders, used for 2d clipping.
        Invariant: To be called only when the given intersection actually exists, which is the case for our use case. 
        (But otherwise, the assertions should catch the divisions by zero in those cases).  

        cf. https://www.cs.helsinki.fi/group/goa/viewing/leikkaus/intro2.html
        (Their code doesn't take into account division by zero explicity, as they use 1/slope for the top/bottom intersect; my code does by calculating the invslope directly.)
    */
    RasterPoint inter;
    FIXED slope, invslope;
    switch (edge) {
        case LEFT_EDGE:
            assertion(current.x - prev.x !=0, "calcIntersect div 0");
            slope = fxdiv(int2fx(current.y - prev.y), int2fx(current.x - prev.x));
            inter.x = 0;
            inter.y = fx2int(fxmul(slope, 0 - int2fx(prev.x)) + int2fx(prev.y));
            break;
        case RIGHT_EDGE:
            assertion(current.x - prev.x !=0, "calcIntersect div 0");
            slope = fxdiv(int2fx(current.y - prev.y), int2fx(current.x - prev.x));
            inter.x = M5_SCALED_W - 1;
            inter.y = fx2int(fxmul(slope, int2fx(M5_SCALED_W - 1) - int2fx(prev.x)) + int2fx(prev.y));
            break;
        case TOP_EDGE:
            assertion(current.y - prev.y !=0, "calcIntersect div 0");
            invslope = fxdiv(int2fx(current.x - prev.x), int2fx(current.y - prev.y));
            inter.y = 0;
            inter.x = fx2int(fxmul(0 - int2fx(prev.y), invslope) + int2fx(prev.x));
            break;
        case BOTTOM_EDGE:
            assertion(current.y - prev.y !=0, "calcIntersect div 0");
            invslope = fxdiv(int2fx(current.x - prev.x), int2fx(current.y - prev.y));
            inter.y = M5_SCALED_H - 1;
            inter.x = fx2int(fxmul(int2fx(M5_SCALED_H - 1) - int2fx(prev.y), invslope) + int2fx(prev.x));
            break;
        default:
            panic("calcIntersect: unknown edge");
    }
    return inter;
}


INLINE bool inside_clip(RasterPoint point, ClipEdges edge) 
{
    /* Used by the Sutherland Hodgman Clipping against the Screen. 
       Returns true if the given point lies on the same side of the given screen's edge as the remainder of the screen. 
       This does *not* imply the point actually lies within the screen, that would be incorrect for the purposes of the Sutherland-Hodgman algorithm. 
    */
    switch(edge) {
        case TOP_EDGE:
            return point.y >= 0; 
            break;
        case BOTTOM_EDGE:
            return point.y < M5_SCALED_H;
            break;
        case LEFT_EDGE: 
            return point.x >= 0;
            break;
        case RIGHT_EDGE:
            return point.x < M5_SCALED_W;
            break;
        default:
            panic("unknown clipping edge");
            return false;
    }
}

int clipTriangleVerts2d(RasterPoint outputVertices[DRAW_MAX_CLIPPED_POLY_LEN]) 
{
    /*  
        Takes the outputVertices (with index 0 to 3 filled by the triangle which we want to clip against the screen), and modifies it to contain the vertices of the n-gon clipped to the screen from 0 to n - 1.  
        Returns the number of vertices n of the clipped n-gon that are contained in outputVertices from 0 to n -1 (can be zero if the triangle is not visible on the screen after clipping). 
        cf. https://en.wikipedia.org/wiki/Sutherland–Hodgman_algorithm
    */
    int outputLen = 3;
    RasterPoint inputList[DRAW_MAX_CLIPPED_POLY_LEN];

    for (int edge = 0; edge < 4; ++edge) { 
        memcpy(inputList, outputVertices, sizeof(RasterPoint) * outputLen);
        int inputLen = outputLen;
        outputLen = 0;
        for (int i = 0; i < inputLen; ++i) {
            RasterPoint prev = inputList[i];
            RasterPoint current = inputList[(i + 1) % inputLen];
            if (inside_clip(current, edge)) {
                if (!inside_clip(prev, edge) ) {
                    assertion(outputLen < DRAW_MAX_CLIPPED_POLY_LEN, "output len 1");
                    outputVertices[outputLen++] = calcIntersect2d(edge, current, prev);
                }
                assertion(outputLen < DRAW_MAX_CLIPPED_POLY_LEN, "output len 1");
                outputVertices[outputLen++] = current;
            } else if (inside_clip(prev, edge)) { 
                assertion(outputLen < DRAW_MAX_CLIPPED_POLY_LEN, "output len 2");
                outputVertices[outputLen++] = calcIntersect2d(edge, current, prev);
            }
        }  
    }     
    /* 
        This is just a sanity check/assertion; the algorithm should not yield out of bounds vertices.
        Note that we can only check this after the algorithm has terminated, as the intermediate steps can yield intermediary out-of-screen vertices. 
        Checking for out-of-screen vertices while we haven't constructed the final outputVertices array *does not make sense* due to the nature of S-H-clipping. 
    */ 
    for (int i = 0; i < outputLen; ++i) { 
        assertion(RASTERPOINT_IN_BOUNDS_M5(outputVertices[i]), "draw.c: clipTriangleVerts2d: Vertex within screen bounds");
    }
    return outputLen; 
}
