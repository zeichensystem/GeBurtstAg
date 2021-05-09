#ifndef DRAW_H
#define DRAW_H

#include "math.h"
#include "camera.h"
#include "model.h"
#include "raster_geometry.h"

void drawInit();
void drawBefore(Camera *cam);

void drawModelInstances(const Camera *cam, const ModelInstance *instances, int numInstances, Vec3 lightDir);
void drawPoints(const Camera *cam, Vec3 *points, int num, COLOR clr);

#endif