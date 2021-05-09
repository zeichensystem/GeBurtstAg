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

typedef struct ModelInstance { // Different Instances share their vertex/face data, which saves us memory. 
    Model mod;
    Vec3 pos;
    FIXED scale;
    ANGLE_FIXED_12 yaw, pitch, roll;
} ModelInstance;


void modelInit(void);
Model modelNew(Vec3 *verts, Face *faces, int numVerts, int numFaces);
ModelInstance modelCubeNewInstance(Vec3 pos, FIXED scale);

#endif