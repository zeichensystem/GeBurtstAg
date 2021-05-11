#ifndef MODEL_H
#define MODEL_H

#include <tonc.h>
#include "math.h"
#include "raster_geometry.h"

#define MAX_MODEL_VERTS 128
#define MAX_MODEL_FACES 32


typedef enum FaceType {
    TriangleFace, 
    ConvexPlanarQuadFace
} FaceType;

typedef struct Face {
    FaceType type;
    int vertexIndex[4];  // The faces don't save the vertices explicity, but indices to them (as vertices are usually shared among different faces, we save memory.)
    Vec3 normal;
    COLOR color;
} Face;

typedef struct Model {
    Vec3 *verts;
    Face *faces;
    int numVerts, numFaces;
} Model;


typedef struct ModelDrawLightingData {
    bool distanceAttenuation;
    const Vec3 *directional;
    const Vec3 *point;

} ModelDrawLightingData;


typedef struct ModelInstance { // Different Instances share their vertex/face data, which saves us memory. 
    bool isEmpty;
    union { 
        struct ModelInstance *__next; // Only neeeded if empty. Note: We could also use ModelInstanceIDs instead of pointers.
        struct { // Only need those when the instance is not empty. 
            Model mod;
            Vec3 pos;
            FIXED scale;
            ANGLE_FIXED_12 yaw, pitch, roll;
            PolygonShadingType shading;
            FIXED camSpaceDepth;
        }; 
    } state;

} ModelInstance;


typedef struct ModelInstancePool {
    int POOL_CAPACITY; 
    int instanceCount;
    ModelInstance *instances;
    ModelInstance *firstAvailable;
} ModelInstancePool;

ModelInstancePool modelInstancePoolNew(ModelInstance *buffer, int bufferCapacity);
void modelInstancePoolReset(ModelInstancePool *pool);
int modelInstanceRemove(ModelInstancePool *pool, ModelInstance* instance);


void modelInit(void);
Model modelNew(Vec3 *verts, Face *faces, int numVerts, int numFaces);
ModelInstance *modelCubeNewInstance(ModelInstancePool *pool, Vec3 pos, FIXED scale, PolygonShadingType shading);

#endif