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
Model cubeModel;

void modelInstancePoolReset(ModelInstancePool *pool) 
{
    // Setup the free list of the ModelInstance object pool.
    for (int i = 0; i < pool->POOL_CAPACITY; ++i) {
        pool->instances[i].isEmpty = true;
        pool->instances[i].state.__next =  (i+1 < pool->POOL_CAPACITY) ? pool->instances + i + 1 : NULL;
    }
    pool->firstAvailable = pool->instances;
}

ModelInstancePool modelInstancePoolNew(ModelInstance *buffer, int bufferCapacity) {
    ModelInstancePool new = {.POOL_CAPACITY=bufferCapacity, .instances=buffer, .firstAvailable=NULL, .instanceCount=0};
    modelInstancePoolReset(&new);
    return new;
}

ModelInstance* modelInstanceAdd(ModelInstancePool *pool,  Model model, const Vec3 *pos, const Vec3 *scale, ANGLE_FIXED_12 yaw, ANGLE_FIXED_12 pitch, ANGLE_FIXED_12 roll, PolygonShadingType shading)
{
    assertion(pool != NULL, "model.c: modelInstanceAdd: pool != NULL");
    assertion(pool->firstAvailable != NULL, "model.c: modelInstanceAdd: pool capacity not exceeded (by pointer)");
    assertion(pool->instanceCount < pool->POOL_CAPACITY, "model.c: modelInstancePoolAdd: pool capacity not exceeded (by count)");
    assertion(pool->firstAvailable->isEmpty, "model.c: modelInstanceAdd: first availabe empty");

    ModelInstance *new = pool->firstAvailable;
    pool->firstAvailable = new->state.__next; // Update our free-list.

    new->isEmpty = false;
    new->state.mod = model;
    assertion(pos != NULL, "model.c: modelInstancePoolAdd: pos != NULL");
    assertion(scale != NULL, "model.c: modelInstancePoolAdd: scale != NULL");

    new->state.pos = *pos;
    new->state.yaw = yaw;
    new->state.pitch = pitch;
    new->state.roll = roll;
    new->state.scale.x = scale->x; new->state.scale.y = scale->y; new->state.scale.z = scale->z;
    new->state.shading = shading;

    pool->instanceCount++;
    return new;
}

/* Creates an instance with uniform scaling, and 0 rotation for convenience. */
ModelInstance* modelInstanceAddVanilla(ModelInstancePool *pool,  Model model, const Vec3 *pos, FIXED scale, PolygonShadingType shading) 
{ 
    Vec3 uniformScale = {.x = scale, .y=scale, .z=scale};
    return modelInstanceAdd(pool, model, pos, &uniformScale, 0, 0, 0, shading);
}

int modelInstanceRemove(ModelInstancePool *pool, ModelInstance* instance) 
{ 
    assertion(pool != NULL, "model.c: modelInstancePoolRemove: pool not NULL");
    assertion(instance != NULL, "model.c: modelInstancePoolRemove: instance not NULL");
    assertion(pool->instanceCount > 0, "model.c: modelInstancePoolRemove: instance pool not empty");
    // Prepend the removed/newly available element to the beginning of the free-list. 
    instance->isEmpty = true;
    instance->state.__next = pool->firstAvailable;
    pool->firstAvailable = instance;

    pool->instanceCount -= 1;
    return pool->instanceCount;
}


Model modelNew(Vec3 *verts, Face *faces, int numVerts, int numFaces) 
{
    assertion(numVerts <= MAX_MODEL_VERTS, "model.c: modelNew: numVert <= MAX");
    assertion(numFaces <= MAX_MODEL_FACES, "model.c: modelNew: numFaces <= MAX");
    Model m = {.faces=faces, .verts=verts, .numVerts=numVerts, .numFaces=numFaces};
    return m;
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
    Face trigs[12] = { // Counter-clockwise winding order.
        // front
        {.vertexIndex = {0, 3, 2}, .color = CLR_CYAN, .normal={0, 0, int2fx(1)}, .type=TriangleFace}, 
        {.vertexIndex = {2, 1, 0}, .color = CLR_CYAN, .normal={0, 0, int2fx(1)}, .type=TriangleFace},
        // back
        {.vertexIndex = {6, 7, 4}, .color = CLR_RED, .normal={0, 0, int2fx(-1)}, .type=TriangleFace},  
        {.vertexIndex = {4, 5, 6}, .color = CLR_RED, .normal={0, 0, int2fx(-1)}, .type=TriangleFace},  
        // right
        {.vertexIndex = {3, 7, 6}, .color = CLR_BLUE, .normal={int2fx(1), 0, 0}, .type=TriangleFace},
        {.vertexIndex = {6, 2, 3}, .color = CLR_BLUE, .normal={int2fx(1), 0, 0}, .type=TriangleFace},  
        // left
        {.vertexIndex = {1, 5, 4}, .color = CLR_MAG, .normal={int2fx(-1), 0, 0}, .type=TriangleFace},
        {.vertexIndex = {4, 0, 1}, .color = CLR_MAG, .normal={int2fx(-1), 0, 0}, .type=TriangleFace},
        // bottom
        {.vertexIndex = {7, 3, 0}, .color = CLR_GREEN, .normal={0, int2fx(-1), 0}, .type=TriangleFace}, 
        {.vertexIndex = {0, 4, 7}, .color = CLR_GREEN, .normal={0, int2fx(-1), 0}, .type=TriangleFace},
        // top
        {.vertexIndex = {6, 5, 1}, .color = CLR_YELLOW, .normal={0, int2fx(1), 0}, .type=TriangleFace},
        {.vertexIndex = {1, 2, 6}, .color = CLR_YELLOW, .normal={0, int2fx(1), 0}, .type=TriangleFace},
    };
    memcpy(cubeModelFaces, trigs, 12 * sizeof(Face));
    cubeModel = modelNew(cubeModelVerts, cubeModelFaces, 8, 12);
}
