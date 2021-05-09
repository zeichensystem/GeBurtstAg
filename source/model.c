#include <tonc.h>
#include <string.h>
#include <stdlib.h>

#include "model.h"
#include "math.h"
#include "camera.h"
#include "logutils.h"
#include "timer.h"
#include "draw.h"
#include "globals.h"


static Vec3 cubeModelVerts[8];
static Face cubeModelFaces[12];
static Model cubeModel;

Model modelNew(Vec3 *verts, Face *faces, int numVerts, int numFaces) 
{
    assertion(numVerts <= MAX_MODEL_VERTS, "model.c: modelNew: numVert <= MAX");
    assertion(numFaces <= MAX_MODEL_FACES, "model.c: modelNew: numFaces <= MAX");
    Model m = {.faces=faces, .verts=verts, .numVerts=numVerts, .numFaces=numFaces};
    return m;
}


ModelInstance modelCubeNewInstance(Vec3 pos, FIXED scale) 
{
    ModelInstance cube = {.mod=cubeModel, .pos=pos, .scale=scale, .yaw=0, .pitch=0, .roll=0};
    return cube;
}


void modelInit(void) 
{
    FIXED half = int2fx(1) >> 2; // quarter?
    Vec3 verts[8] = {
        // front plane
        {.x = -half, .y = -half, .z = half},
        {.x = -half, .y = half, .z = half},
        {.x = half, .y = half, .z = half},
        {.x = half, .y = -half, .z = half},
        // back plane
        {.x = -half, .y = -half, .z = -half},
        {.x = -half, .y = half, .z = -half},
        {.x = half, .y = half,  .z = -half},
        {.x = half, .y = -half, .z = -half},
    };
    memcpy(cubeModelVerts, verts, sizeof(cubeModelVerts));

    Face trigs[12] = {
        // front
        {.vertexIndex = {0, 1, 2}, .color = CLR_CYAN, .normal={0, 0, int2fx(1)}, .type=TriangleFace}, // FIXME TODO no idea why i have to invert the normals for lighting
        {.vertexIndex = {2, 3, 0}, .color = CLR_CYAN, .normal={0, 0, int2fx(1)}, .type=TriangleFace},
        // back
        {.vertexIndex = {4, 7, 6}, .color = CLR_RED, .normal={0, 0, int2fx(-1)}, .type=TriangleFace},  
        {.vertexIndex = {6, 5, 4}, .color = CLR_RED, .normal={0, 0, int2fx(-1)}, .type=TriangleFace},  
        // right
        {.vertexIndex = {3, 2, 6}, .color = CLR_BLUE, .normal={int2fx(1), 0, 0}, .type=TriangleFace},
        {.vertexIndex = {6, 7, 3}, .color = CLR_BLUE, .normal={int2fx(1), 0, 0}, .type=TriangleFace},  
        // left
        {.vertexIndex = {4, 5, 1}, .color = CLR_MAG, .normal={int2fx(-1), 0, 0}, .type=TriangleFace},
        {.vertexIndex = {1, 0, 4}, .color = CLR_MAG, .normal={int2fx(-1), 0, 0}, .type=TriangleFace},
        // bottom
        {.vertexIndex = {0, 3, 7}, .color = CLR_GREEN, .normal={0, int2fx(-1), 0}, .type=TriangleFace}, // FIXME TODO no idea why i have to invert the normals for lighting
        {.vertexIndex = {7, 4, 0}, .color = CLR_GREEN, .normal={0, int2fx(-1), 0}, .type=TriangleFace},
        // top
        {.vertexIndex = {1, 5, 6}, .color = CLR_YELLOW, .normal={0, int2fx(1), 0}, .type=TriangleFace},
        {.vertexIndex = {6, 2, 1}, .color = CLR_YELLOW, .normal={0, int2fx(1), 0}, .type=TriangleFace},
    };
    
    memcpy(cubeModelFaces, trigs, 12 * sizeof(Face));
    cubeModel = modelNew(cubeModelVerts, cubeModelFaces, 8, 12);
}


