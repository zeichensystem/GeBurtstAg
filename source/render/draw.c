#include <string.h>
#include <stdlib.h>
#include <tonc.h>

#include "../globals.h"
#include "../commondefs.h"
#include "../math.h"
#include "../logutils.h"
#include "../model.h"

#include "draw.h"
#include "clipping.h"
#include "rasteriser.h"

#define RASTERPOINT_IN_BOUNDS_M5(vert) (vert.x >= 0 && vert.x < M5_SCALED_W && vert.y >= 0 && vert.y < M5_SCALED_H)
#define BEHIND_NEAR(vert) (vert.z > -cam->near ) // True if the Vec3 is behind the near plane of the camera (i.e. invisible).
#define BEYOND_FAR(vert) (vert.z < -cam->far)

#define DRAW_MAX_TRIANGLES 512
EWRAM_DATA static RasterTriangle screenTriangles[DRAW_MAX_TRIANGLES]; 
static int screenTriangleCount = 0;

/*
    With an ordering table, we can avoid expensive sorting. Basically just an array containing linked lists for each depth value.
    We sacrifice memory usage (and accuracy, i.e. Polygons which are a certain cutoff distance from each other are drawn in indeterminate order, but it should not matter) for speed. 
    cf. http://psx.arthus.net/sdk/Psy-Q/DOCS/TECHNOTE/ordtbl.pdf
*/
#define OT_SIZE 512
// We assume an .8 fixed point representation of our depth values; we calculate the index by converting to .1 fixed point, which means at a z-value of OT_SIZE/2 (integer) we have a index of OT_SIZE   
#define MAX_Z (OT_SIZE / 2 - 1)
static RasterTriangle *orderingTable[OT_SIZE]; // TODO: We might have to put this into EWRAM to save space in IWRAM...

static int perfFill, perfModelProcessing, perfTotal, perfProject;

/* 
    Scaling using the affine background capabilities of the GBA. 
    We use Mode 5 (160x128) with an "internal/logical" resolution of 160x100 scaled to fit the 
    240x160 (factor 1.5) screen of the GBA (with 5px letterboxes on the top and bottom). 
*/
void setDispScaleM5Scaled(void) 
{

    FIXED threeHalfInv = 170; // 170 is about (3/2)^-1 in .8 fixed point.
    AFF_SRC_EX asx= {
        .alpha=0,
        .sx=threeHalfInv,
        .sy=threeHalfInv,
        .scr_x=0,
        .scr_y=5, // Vertical letterboxing.
        .tex_x=0,
        .tex_y=0 };
    BG_AFFINE bgaff;
    bg_rotscale_ex(&bgaff, &asx);
    REG_BG_AFFINE[2]= bgaff;
}

void resetDispScale(void) 
{
    BG_AFFINE bgaff;
    bg_aff_identity(&bgaff);
    REG_BG_AFFINE[2]= bgaff;
}

void setM4Pal(COLOR *pal, int n) 
{
    u16 *dst= pal_bg_mem;
    for(int i=0; i < n; ++i)
        dst[i]= pal[i];
}

static void updateMode(void) 
{
    if (vid_page == vid_mem_front) { // If the front page (vid_mem_front) is the current write-page, we have to indicate that the back page is the displayed page by setting DCNT_PAGE.
        REG_DISPCNT = g_mode | DCNT_BG2 | DCNT_PAGE;  
    } else { // The current write page is the back page, so we display the front page (which happens by default if we don't set DCNT_PAGE).
        REG_DISPCNT = g_mode | DCNT_BG2;
    }
    // cf. https://gist.github.com/zeichensystem/0729edcddf8f24db14e5b1b4ef4c0c3f (last retrieved 2021-05-23)
}

void videoM5ScaledInit(void) 
{
    g_mode = DCNT_MODE5;
    updateMode();
    setDispScaleM5Scaled();
}

void videoM4Init(void) 
{
    g_mode = DCNT_MODE4;
    updateMode();
    resetDispScale();
}

void m5ScaledFill(COLOR clr) 
{
    memset32(vid_page, dup16(clr), ((M5_SCALED_H-0) * M5_SCALED_W)/2);
}


void drawInit(void) 
{
    REG_DISPCNT = g_mode | DCNT_BG2;
    txt_init_std();
    perfFill = performanceDataRegister("draw.c: rasterisation");
    perfModelProcessing = performanceDataRegister("draw:c pre-rasterisation");
    perfTotal = performanceDataRegister("draw.c: total");
    perfProject = performanceDataRegister("draw.c: drawModelInstance perspective");
}


IWRAM_CODE_ARM void drawBefore(Camera *cam) 
{ 
    cameraComputeWorldToCamSpace(cam);
}


IWRAM_CODE_ARM void drawPoints(const Camera *cam, Vec3 *points, int num, COLOR clr) 
{
    for (int i = 0; i < num; ++i) {
        Vec3 pointCamSpace = vecTransformed(cam->world2cam, points[i]);
        if (BEHIND_NEAR(pointCamSpace) || BEYOND_FAR(pointCamSpace)) { 
            continue;
        }
 
        FIXED const z = -pointCamSpace.z;
        FIXED pre_divide_x = fxmul(cam->perspFacX, pointCamSpace.x);
        if (pre_divide_x < -z || pre_divide_x > z ) {// Check if the point is to the left/right of the viewing frustum before dividing (to save unnecessary divisions in those cases).  
            continue;
        }
        FIXED pre_divide_y = fxmul(cam->perspFacY, pointCamSpace.y);
        if (pre_divide_y < -z|| pre_divide_y > z ) { // Check if the point is to the top/bottom of the viewing frustum. 
            continue;
        }
        RasterPoint rp = {
            .x=fx2int( fxmul(cam->viewportTransFacX, fxdiv(pre_divide_x, z)) + cam->viewportTransAddX ),
            .y=fx2int( fxmul(cam->viewportTransFacY, fxdiv(pre_divide_y, z)) + cam->viewportTransAddY )
        };
        if (RASTERPOINT_IN_BOUNDS_M5(rp)) { 
            m5_plot(rp.x, rp.y, clr);
        }
    }
}

INLINE void drawTriangleWireframe(const RasterTriangle *tri) 
{ 
    if (!RASTERPOINT_IN_BOUNDS_M5(tri->vert[0]) || !RASTERPOINT_IN_BOUNDS_M5(tri->vert[1]) || !RASTERPOINT_IN_BOUNDS_M5(tri->vert[2])) { // We have to clip against the screen.
        for (int j = 0; j < 3; ++j) {
            int nextIdx = (j + 1) < 3 ? j + 1 : 0;
            RasterPoint a = tri->vert[j];
            RasterPoint b = tri->vert[nextIdx];
            if (clipLineCohenSutherland(&a, &b)) {
                m5_line(a.x, a.y, b.x, b.y, tri->color);
            }
        }
    } else { // No clipping necessary.
        for (int j = 0; j < 3; ++j) {
            int nextIdx = (j + 1) < 3 ? j + 1 : 0;
            m5_line(tri->vert[j].x, tri->vert[j].y, tri->vert[nextIdx].x,tri-> vert[nextIdx].y, tri->color);
        }
    }
}


/* 
    C does not have nested functions, but we got macros! 
    I'm sorry. 
*/
#define INSTANCE_CALC_LIGHTDIR_AND_ATTENUATION()                                                                                                                            \
        PolygonShadingType instanceShading = instance->state.shading;                                                                                                       \
        Vec3 lightDir;                                                                                                                                                      \
        FIXED attenuation = -1;                                                                                                                                             \
        if (instanceShading == SHADING_FLAT_LIGHTING) {                                                                                                                     \
            if (lightDat.type == LIGHT_POINT) {                                                                                                                             \
                Vec3 dir = vecSub(*lightDat.light.point, instance->state.pos);                                                                                              \
                if (lightDat.attenuation != NULL) {                                                                                                                         \
                    /* http://wiki.ogre3d.org/tiki-index.php?page=-Point+Light+Attenuation (last retrieved 2021-05-12) */                                                   \
                    FIXED d = vecMag(dir);                                                                                                                                  \
                    attenuation = fxdiv(int2fx(1), int2fx(1) + fxmul(d, lightDat.attenuation->linear) + fxmul(fxmul(d, d), lightDat.attenuation->quadratic) );              \
                }                                                                                                                                                           \
                lightDir = vecUnit(dir);                                                                                                                                    \
            } else if (lightDat.type == LIGHT_DIRECTIONAL) {                                                                                                                \
                lightDir = *lightDat.light.directional;                                                                                                                     \
                lightDir.x = -lightDir.x; lightDir.y = -lightDir.y; lightDir.z = -lightDir.z; /* Invert the direction. */                                                   \
            } else {                                                                                                                                                        \
                panic("draw.c: drawModelInstaces: Missing lighting vectors.");                                                                                              \
            }                                                                                                                                                               \
        }                                                                                                                                                                   \

#define FACE_CALC_COLOR() {                                                                                                     \
    if (instanceShading == SHADING_FLAT_LIGHTING) {                                                                             \
        const FIXED lightAlpha = vecDot(lightDir, triNormal);                                                                  \
        if (lightAlpha > 0) {                                                                                                   \
            COLOR shade = fx2int(fxmul(lightAlpha, int2fx(31)));                                                                \
            if (attenuation != -1) {                                                                                            \
                shade = fx2int(fxmul(attenuation, int2fx(shade)));                                                              \
            }                                                                                                                   \
            shade = MIN(MAX(1, shade), 31);                                                                                     \
            screenTri.color = RGB15(shade, shade, shade);                                                                       \
        } else {                                                                                                                \
            screenTri.color = RGB15(1,1,1);                                                                                     \
        }                                                                                                                       \
    } else if (instanceShading == SHADING_FLAT || instanceShading == SHADING_WIREFRAME) {                                       \
        screenTri.color = face.color;                                                                                           \
    } else {                                                                                                                    \
        panic("draw.c: drawModelInstances: Unknown shading option.");                                                           \
    }                                                                                                                           \
}                                                                                                                               \


INLINE void otInsert(RasterTriangle *t) 
{
    int idx = ABS(t->centroidZ) >> (FIX_SHIFT - 1); // We have a granularity of 0.5; polygons that have a smaller z-distance will be drawn in indeterminate order.
    assertion(idx < OT_SIZE && idx >= 0, "draw.c: otInsert: idx < OT_SIZE");
    if (orderingTable[idx]) {
        t->next = orderingTable[idx];
    } else {
        t->next = NULL;
    }
    orderingTable[idx] = t;
}     

/* 
    Performs model to camera space transformations, perspective projection, and shading/lighting calculations.
    Calculates the screen-space triangles which can be drawn later. We put them into the ordering table, so we don't have to sort them. 
*/ 
IWRAM_CODE_ARM static void modelInstancesPrepareDraw(Camera* cam, ModelInstance *instances, int numInstances, ModelDrawLightingData lightDat) 
{ 
    for (int instanceNum = 0; instanceNum < numInstances; ++instanceNum) {
        ModelInstance *instance = instances + instanceNum;
        if (instance->isEmpty) {
            continue;
        }
        // TODO: Insert bounding-sphere culling here.
        FIXED instanceRotMat[16];
        matrix4x4createYawPitchRoll(instanceRotMat, instance->state.yaw, instance->state.pitch, instance->state.roll);
        Vec3 vertsCamSpace[MAX_MODEL_VERTS];
        Vec3 vertsWorldSpace[MAX_MODEL_VERTS];
        RasterPoint vertsProjected[MAX_MODEL_VERTS];

        for (int i = 0; i < instance->state.mod.numVerts; ++i) {
            // Model space to world space:
            vertsCamSpace[i].x = fxmul(instance->state.mod.verts[i].x, instance->state.scale.x); 
            vertsCamSpace[i].y = fxmul(instance->state.mod.verts[i].y, instance->state.scale.y);
            vertsCamSpace[i].z = fxmul(instance->state.mod.verts[i].z, instance->state.scale.z);
            vecTransform(instanceRotMat, vertsCamSpace + i );
            // We translate manually so that instanceRotMat stays as is (so we can rotate our normals with the instanceRotMat in model space to calculate lighting):
            vertsCamSpace[i].x += instance->state.pos.x;
            vertsCamSpace[i].y += instance->state.pos.y;
            vertsCamSpace[i].z += instance->state.pos.z;
            vertsWorldSpace[i] = vertsCamSpace[i];
            vecTransform(cam->world2cam, vertsCamSpace + i); // And finally, we're in camera space.
            if (BEHIND_NEAR(vertsCamSpace[i]) || BEYOND_FAR(vertsCamSpace[i])) {  
                vertsProjected[i].x = RASTER_POINT_NEAR_FAR_CULL;
                vertsProjected[i].y = RASTER_POINT_NEAR_FAR_CULL;
            } else {
                // Perspective projection and screen space transform; we do it manually instead of just calling vecTransformed(cam->perspMat, vertsCamSpace[i]) for performance (for my test case with 414 triangles: 20.2 ms vs 24.4 ms)
                const FIXED z = vertsCamSpace[i].z;
                // vertsProjected[i].x =  ( ((cam->viewportTransFacX * (cam->perspFacX * vertsCamSpace[i].x / -z)) >> FIX_SHIFT) + cam->viewportTransAddX) >> FIX_SHIFT; (not much faster)
                vertsProjected[i].x = fx2int( fxmul(cam->viewportTransFacX, fxdiv(fxmul(cam->perspFacX, vertsCamSpace[i].x), -z) ) + cam->viewportTransAddX );
                vertsProjected[i].y = fx2int( fxmul(cam->viewportTransFacY, fxdiv(fxmul(cam->perspFacY, vertsCamSpace[i].y), -z) ) + cam->viewportTransAddY );
            }
        }
 
        // Calculate lightDir and attenuation (which don't depend on the faces, only on the instance) so we don't have to re-compute them redundantly in the inner loop over the faces.
        INSTANCE_CALC_LIGHTDIR_AND_ATTENUATION();
            
        for (int faceNum = 0; faceNum < instance->state.mod.numFaces; ++faceNum) { // For each face (triangle, really) of the ModelInstace. 
            const Face face = instance->state.mod.faces[faceNum];

             // Backface culling (assumes a counter-clockwise winding order):
            // const Vec3 a = vecSub(vertsCamSpace[face.vertexIndex[1]], vertsCamSpace[face.vertexIndex[0]]);
            // const Vec3 b = vecSub(vertsCamSpace[face.vertexIndex[2]], vertsCamSpace[face.vertexIndex[0]]);
            // const Vec3 triNormal = vecCross(b, a);
            // const Vec3 camToTri = vertsCamSpace[face.vertexIndex[2]];

            const Vec3 triNormal = vecTransformedRot(instanceRotMat, &face.normal);
            const Vec3 camToTri = vecSub(cam->pos, vertsWorldSpace[face.vertexIndex[0]]); 
            if (vecDot(triNormal, camToTri) <= 0) { // If the angle between camera and normal is not between 90 degs and 270 degs, the face is invisible and to be culled.
                continue;
            }

            RasterTriangle screenTri; 
            for (int i = 0; i < 3; ++i) {
                screenTri.vert[i] = vertsProjected[face.vertexIndex[i]];
                if (screenTri.vert[i].x == RASTER_POINT_NEAR_FAR_CULL && screenTri.vert[i].y == RASTER_POINT_NEAR_FAR_CULL) { // If the face is partly behind the near or far plane, cull the whole (we don't bother with clipping).
                    goto skipFace;
                } 
            }
               
            // Check if all vertices of the face are to the "outside-side" of a given clipping plane. If so, the face is invisible and we can skip it.
            if (screenTri.vert[0].x < 0 && screenTri.vert[1].x < 0 && screenTri.vert[2].x < 0) { // All vertices are to the left of the left-plane.
                continue;
            } else if (screenTri.vert[0].x >= M5_SCALED_W && screenTri.vert[1].x >= M5_SCALED_W && screenTri.vert[2].x >= M5_SCALED_W ) { // All vertices are to the right of the right-plane.
                continue;
            } else if (screenTri.vert[0].y < 0 && screenTri.vert[1].y < 0 && screenTri.vert[2].y < 0) { // All vertices are to the top of the top-plane.
                continue;
            } else if (screenTri.vert[0].y >= M5_SCALED_H && screenTri.vert[1].y >= M5_SCALED_H && screenTri.vert[2].y >= M5_SCALED_H) { // All vertices are to the bottom of the bottom-plane.
                continue;
            }

            FACE_CALC_COLOR();
            screenTri.shading = instance->state.shading;
            screenTri.centroidZ = vertsCamSpace[face.vertexIndex[0]].z; // TODO HACK: That's not the actual centroid (we'd need an expensive division for that), but this is good enough for small faces.
            assertion(screenTriangleCount < DRAW_MAX_TRIANGLES, "draw.c: drawModelInstances: screenTriangleCount < DRAW_MAX_TRIANGLES");
            screenTriangles[screenTriangleCount++] = screenTri;
            otInsert(screenTriangles + (screenTriangleCount - 1));

            skipFace:;
        }
    }
}
#undef INSTANCE_CALC_LIGHTDIR_AND_ATTENUATION
#undef FACE_CALC_COLOR

static int triangleDepthCmp(const void *a, const void *b) 
{
        RasterTriangle *triA = (RasterTriangle*)a;
        RasterTriangle *triB = (RasterTriangle*)b;
        return triA->centroidZ - triB->centroidZ; // Smaller/"more negative" z values mean the triangle is farther away from the camera.
}

IWRAM_CODE_ARM void drawModelInstancePools(ModelInstancePool *pools, int numPools, Camera *cam, ModelDrawLightingData lightDat) 
{

    performanceStart(perfTotal);
    for (int i= 0; i < OT_SIZE; ++i) {
        orderingTable[i] = NULL;
    }

    screenTriangleCount = 0;
    performanceStart(perfModelProcessing);
    for (int i = 0; i < numPools; ++i) { 
        modelInstancesPrepareDraw(cam, pools[i].instances, pools[i].POOL_CAPACITY, lightDat);
    }
    performanceEnd(perfModelProcessing);

    if (screenTriangleCount == 0) {
        goto skipOT;
    }
    int trisToDraw = screenTriangleCount;
    for (int i = OT_SIZE - 1; i >= 0 && trisToDraw; --i) { // Draw triangles from back to front by iterating over the ordering-table. 
        for (RasterTriangle *t = orderingTable[i]; t != NULL; t = t->next) {
            --trisToDraw;
           if (t->shading == SHADING_FLAT || t->shading == SHADING_FLAT_LIGHTING) {
                drawTriangleFlatByggmastar(t);
            } else {
                drawTriangleWireframe(t);
            }
       }
    }
    skipOT:;
    
    performanceEnd(perfTotal);
    char dbg[64];
    snprintf(dbg, sizeof(dbg),  "tris: %d", screenTriangleCount);
    m5_puts(8, 24, dbg, CLR_FUCHSIA);
}

    // RasterTriangle tri;
    // tri.color = CLR_WHITE;
    // tri.vert[0] = (RasterPoint){.x=0, .y=0};
    // tri.vert[1] = (RasterPoint){.x=60, .y=100};
    // tri.vert[2] = (RasterPoint){.x=0, .y=100};
    // drawTriangleFlatByggmastar(&tri);