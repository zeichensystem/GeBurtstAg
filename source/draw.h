#ifndef DRAW_H
#define DRAW_H

#include "math.h"
#include "camera.h"
#include "model.h"
#include "raster_geometry.h"

typedef enum PolygonShading { 
    SHADING_FLAT_LIGHTING,
    SHADING_FLAT, 
    SHADING_WIREFRAME
} PolygonShading;

typedef struct ModelDrawOptions {
    const PolygonShading shading;
    const COLOR wireframeColor;
    const Vec3 *lightDirectional;
    const Vec3 *lightPoint;
} ModelDrawOptions;

void drawInit();
void drawBefore(Camera *cam);

void drawModelInstances(const Camera *cam, const ModelInstance *instances, int numInstances, const ModelDrawOptions *options);
void drawPoints(const Camera *cam, Vec3 *points, int num, COLOR clr);

#endif