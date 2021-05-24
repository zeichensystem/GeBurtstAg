#ifndef DRAW_H
#define DRAW_H

#include "../math.h"
#include "../camera.h"
#include "../model.h"
#include "../raster_geometry.h"

void drawInit(void);
void resetDispScale(void);

// Mode 5 utils
void videoM5ScaledInit(void);
void setDispScaleM5Scaled(void);
void m5ScaledFill(COLOR clr);

// Mode 4 utils
void videoM4Init(void); 
void setM4Pal(COLOR *pal, int n);

/* drawBefore is assumed to be called every frame before the other draw functions are invoked. */
void drawBefore(Camera *cam);
void drawModelInstancePools(ModelInstancePool *pools, int numPools, Camera *cam, ModelDrawLightingData lightDat); 
void drawPoints(const Camera *cam, Vec3 *points, int num, COLOR clr);

#endif