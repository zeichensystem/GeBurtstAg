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

/* 
    Calculates line intersections against the screen borders, used for 2d clipping.
    Invariant: To be called only when the given intersection actually exists, which is the case for our use case. 
    (But otherwise, the assertions should catch the divisions by zero in those cases).  
    cf. https://www.cs.helsinki.fi/group/goa/viewing/leikkaus/intro2.html
    (Their code doesn't take into account division by zero explicity, as they use 1/slope for the top/bottom intersect; my code does by calculating the invslope directly.)
*/
static RasterPoint calcIntersect2d(ClipEdges edge, RasterPoint current, RasterPoint prev) 
{ 
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


/* 
    Used by the Sutherland Hodgman Clipping against the Screen. 
    Returns true if the given point lies on the same side of the given screen's edge as the remainder of the screen. 
    This does *not* imply the point actually lies within the screen, that would be incorrect for the purposes of the Sutherland-Hodgman algorithm. 
*/
INLINE bool inside_clip(RasterPoint point, ClipEdges edge) 
{

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

/*  
    cf. https://en.wikipedia.org/wiki/Sutherland–Hodgman_algorithm
*/
int clipTriangleVerts2d(RasterPoint outputVertices[CLIPPING_MAX_POLY_LEN]) 
{
    int outputLen = 3;
    RasterPoint inputList[CLIPPING_MAX_POLY_LEN];

    for (int edge = 0; edge < 4; ++edge) { 
        memcpy(inputList, outputVertices, sizeof(RasterPoint) * outputLen);
        int inputLen = outputLen;
        outputLen = 0;
        for (int i = 0; i < inputLen; ++i) {
            RasterPoint prev = inputList[i];
            RasterPoint current = inputList[(i + 1) % inputLen];
            if (inside_clip(current, edge)) {
                if (!inside_clip(prev, edge) ) {
                    assertion(outputLen < CLIPPING_MAX_POLY_LEN, "output len 1");
                    outputVertices[outputLen++] = calcIntersect2d(edge, current, prev);
                }
                assertion(outputLen < CLIPPING_MAX_POLY_LEN, "output len 1");
                outputVertices[outputLen++] = current;
            } else if (inside_clip(prev, edge)) { 
                assertion(outputLen < CLIPPING_MAX_POLY_LEN, "output len 2");
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


/* 
    The following code is copied more or less line by line (no pun intended) from the English wikipedia article
    on Cohen-Sutherland line-cliping (with minor modifications), and therefore licensed under the
    "Creative Commons Attribution-ShareAlike 3.0 Unported License".
    cf. https://en.wikipedia.org/wiki/Cohen–Sutherland_algorithm (retrieved 2021-05-10)
 */
typedef int OutCode;
static const int INSIDE = 0; // 0000
static const int LEFT = 1;   // 0001
static const int RIGHT = 2;  // 0010
static const int BOTTOM = 4; // 0100
static const int TOP = 8;    // 1000

INLINE OutCode computeOutcode(const RasterPoint *a) 
{
    OutCode code = INSIDE; 
    if (a->x < 0) {
        code |= LEFT;
    } else if (a->x >= M5_SCALED_W) {
        code |= RIGHT;
    }
    if (a->y < 0) {
        code |= TOP;
    } else if (a->y >= M5_SCALED_H) {
        code |= BOTTOM;
    }
    return code;
}

bool clipLineCohenSutherland(RasterPoint *a, RasterPoint *b) 
{  
    OutCode outcodeA = computeOutcode(a);
    OutCode outcodeB = computeOutcode(b);

    int count = 0;
    while (1) {
        if (!(outcodeA | outcodeB)) { // Both vertices within the screen.
            return true;
        } else if (outcodeA & outcodeB) { // Both vertices share an outside zone (and therefore are invisible.)
            return false;
        } 
        OutCode outside = outcodeA > outcodeB ? outcodeA : outcodeB; // One of the vertices must be outside, select that one. 
        int x, y;
        if (outside & TOP) {
            // (x2 - x1) / (y2 - y1) = (x - x1) / (y - y1)  
            y = 0;
            x = a->x + fx2int(fxmul(fxdiv(int2fx(b->x - a->x), int2fx(b->y - a->y)), 0 - int2fx(a->y)));
        } else if (outside & BOTTOM) {
            y = M5_SCALED_H - 1;
            x = a->x + fx2int(fxmul(fxdiv(int2fx(b->x - a->x), int2fx(b->y - a->y)), int2fx(y) - int2fx(a->y)));
        } else if (outside & LEFT) {
            // (y2 - y1) / (x2 - x1) = (y - y1) / (x - x1) 
            x = 0;
            y = a->y + fx2int(fxmul(fxdiv(int2fx(b->y - a->y), int2fx(b->x - a->x)), int2fx(x) - int2fx(a->x)));
        } else if (outside & RIGHT) {
            x = M5_SCALED_W - 1;
            y = a->y + fx2int(fxmul(fxdiv(int2fx(b->y - a->y), int2fx(b->x - a->x)), int2fx(x) - int2fx(a->x)));
        } else {
            panic("clipping.c: clipLineCohenSutherland: Programming error; no clip intersection.");
        }

        if (outside == outcodeA) { 
            a->x = x;
            a->y = y;
            outcodeA = computeOutcode(a);
        } else { 
            b->x = x;
            b->y = y;
            outcodeB = computeOutcode(b);
        }

        if (++count > 32) {
            panic("clipping.c: clipLineCohenSutherland: infinite loop");
            return false;
        }
    }
}
