#ifndef MODEL_H
#define MODEL_H

#include <tonc.h>
#include "math.h"
#include "raster_geometry.h"

#define MAX_MODEL_VERTS 256
#define MAX_MODEL_FACES 256

/*
    We want to use object pools to manage our modelInstances, just a thin abstraction on top of static arrays with no dynamic allocations etc. 
    That means we can add and remove instances in constant time if we want that. 
    Only disadvantage would be that we have to iterate over empty slots then if we want to draw all and don't want to have a seperate array,
    (but if we just keep our pools full, or mostly full, they are equivalent to regular arrays in that regard). 
    cf. https://gameprogrammingpatterns.com/object-pool.html (last retrieved: 2021-05-11)
*/

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


typedef struct LightAttenuationParams {
    const FIXED linear, quadratic;
} LightAttenuationParams;

/* 
    Attenuation parameters for different light ranges. Note: in .8 fixed point. 
    cf. http://wiki.ogre3d.org/tiki-index.php?page=-Point+Light+Attenuation (last retrieved 2021-05-12)
*/
static const LightAttenuationParams lightAttenuation100 = {.linear=12, .quadratic=2};
static const LightAttenuationParams lightAttenuation160 = {.linear=7, .quadratic=1};
static const LightAttenuationParams lightAttenuation200 = {.linear=6, .quadratic=0};

typedef enum LightType {
    LIGHT_DIRECTIONAL, 
    LIGHT_POINT
} LightType;

typedef struct ModelDrawLightingData {
    const LightType type;
    union {
        const Vec3 *directional;
        const Vec3 *point;
    } light;
    const LightAttenuationParams *attenuation; // Attenuation only for point lights.

} ALIGN4 ModelDrawLightingData;


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
    } ALIGN4 state;

} ALIGN4 ModelInstance;

typedef struct ModelInstancePool {
    int POOL_CAPACITY; 
    int instanceCount;
    ModelInstance *instances;
    ModelInstance *firstAvailable;
} ModelInstancePool;


void modelInit(void);
ModelInstancePool modelInstancePoolNew(ModelInstance *buffer, int bufferCapacity);
void modelInstancePoolReset(ModelInstancePool *pool);
int modelInstanceRemove(ModelInstancePool *pool, ModelInstance* instance);
Model modelNew(Vec3 *verts, Face *faces, int numVerts, int numFaces);
ModelInstance* modelInstanceAdd(ModelInstancePool *pool,  Model model, const Vec3 *pos, FIXED scale, ANGLE_FIXED_12 yaw, ANGLE_FIXED_12 pitch, ANGLE_FIXED_12 roll, PolygonShadingType shading); 
ModelInstance *modelCubeNewInstance(ModelInstancePool *pool, Vec3 pos, FIXED scale, PolygonShadingType shading);

#endif