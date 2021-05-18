#include <string.h>
#include <stdlib.h>
#include <tonc.h>

#include "globals.h"
#include "math.h"
#include "draw.h"
#include "logutils.h"
#include "model.h"
#include "clipping.h"

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
static RasterTriangle *orderingTable[OT_SIZE];

static int perfFill, perfModelProcessing, perfTotal, perfProject;

void setDispScaleM5Scaled() {
      /* 
        Scaling using the affine background capabilities of the GBA. 
        We use Mode 5 (160x128) with an "internal/logical" resolution of 160x100 scaled to fit the 
        240x160 (factor 1.5) screen of the GBA (with 5px letterboxes on the top and bottom). 
    */
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

void resetDispScale(void) {
    BG_AFFINE bgaff;
    REG_BG_AFFINE[2]= bgaff;
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

IWRAM_CODE void drawBefore(Camera *cam) 
{ 
    cameraComputeWorldToCamSpace(cam);
}


IWRAM_CODE void drawPoints(const Camera *cam, Vec3 *points, int num, COLOR clr) 
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
    Credits of the triangle rasterisation code: Mats Byggmastar (a.k.a. MRI / Doomsday). 
    who described it in his article "Fast affine texture mapping (fatmap.txt)". [1]
    I adapted it to draw flat polygons. 
    The parts of triangles outside of the screen are simply not drawn; this code was added by me, 
    and is not described anywhere in fatmap.txt; therefore, it might be less correct. 
    cf. http://ftp.lanet.lv/ftp/mirror/x2ftp/msdos/programming/theory/fatmap.txt (last retrieved 2021-05-14)
*/ 
#define FIXED_16_2_INT_CEIL(n) ((n + 0xffff) >> 16)
typedef int FIXED_16;
static const RasterPoint* left_array[3];
static const RasterPoint* right_array[3];
static int left_section_idx, right_section_idx;
static int left_section_height, right_section_height;
static FIXED_16 left_x, delta_left_x, right_x, delta_right_x; // Those are in .16 fixed point as opposed to our default .8 (Better accuracy).

INLINE int calcRightSection(void) {
    const RasterPoint *v1 = right_array[right_section_idx];
    const RasterPoint *v2 = right_array[right_section_idx - 1];
    int height = v2->y - v1->y;
    if (height == 0) {
        return 0;
    }
    delta_right_x = ((v2->x - v1->x) << 16) / height;

    right_x = (v1->x << 16);
    int dy = MAX(0, v1->y) - v1->y; // Vertical "clipping".
    if (dy) {
        right_x += dy * delta_right_x;
    }
    return right_section_height = MIN(M5_SCALED_H, v2->y) - MAX(0, v1->y);
}

INLINE int calcLeftSection(void) {
    const RasterPoint *v1 = left_array[left_section_idx];
    const RasterPoint *v2 = left_array[left_section_idx - 1];

    int height = v2->y - v1->y;
    if (height == 0) {
        return 0;
    }
    delta_left_x = ((v2->x - v1->x) << 16) / height;
    left_x = (v1->x << 16);
    int dy = MAX(0, v1->y) - v1->y; // Vertical "clipping".
    if (dy) {
        left_x += dy * delta_left_x;
    }
    return left_section_height = MIN(M5_SCALED_H, v2->y) - MAX(0, v1->y);
}

/* 
    A version of m5_hline (libtonc) which does not normalise x1 and x2, i.e. just assumes x1 < x2. 
    It is measurably faster because it's called so often, and we can guarantee x1 < x2 (If I'm not wrong).
    Dangerous: If the invariant is not met, it will lead to crashes or bugs, so don't use this if you're unsure. 
*/
INLINE void m5_hline_nonorm(int x1, int y, int x2, COLOR clr) 
{
    u16 *dstL= (u16*)((u8*)vid_page+y*(M5_WIDTH<<1) + x1*2);
    memset16(dstL, clr, x2-x1+1);
}

INLINE void drawTriangleFlatByggmastar(const RasterTriangle *tri) 
{
    const RasterPoint *v1 = tri->vert;
    const RasterPoint *v2 = tri->vert + 1;
    const RasterPoint *v3 = tri->vert + 2;

    // Sort vertices: v1 should be the top, v2 the middle, and v3 the bottom vertex. 
    if (v1->y > v2->y) {
        const RasterPoint *tmp = v1; v1 = v2; v2 = tmp;
    }
    if (v1->y > v3->y) {
        const RasterPoint *tmp = v1; v1 = v3; v3 = tmp;
    }
    if (v2->y > v3->y) {
        const RasterPoint *tmp = v2; v2 = v3; v3 = tmp;
    }

    if (v1->y >= M5_SCALED_H - 1) { // Triangle certainly invisible. 
        return;
    }

    int height = v3->y - v1->y;
    if (height == 0) { // Degenerate triangle.
        return;
    }
    // Calculate length of longest scanline (the lenght of that scanline "where the triangle has its bend, if applicable").
    // FIXED alpha = ((v2->y - v1->y) << 16) / height;
    // FIXED longest = alpha * (v3->x - v1->x) + ((v1->x - v2->x) << 16);
    // if(longest == 0) {
    //     return;
    // }
    /* 
        NOTE: Calculating the longest scanline (which requires a division) is not necessary for a non-textured fill. 
              We can just use the cross product to determine whether the middle vertex is on the right/left. 
    */
    const int dx1 = v2->x - v1->x;
    const int dy1 = v2->y - v1->y;
    const int dx2 = v3->x - v1->x;
    const int dy2 = v3->y - v1->y;
    const int cross = dx1 * dy2 - dy1 * dx2;

    if (cross > 0) { // Middle vertex is on the right side
        right_array[0] = v3;
        right_array[1] = v2;
        right_array[2] = v1;
        right_section_idx = 2;
        left_array[0] = v3;
        left_array[1] = v1;
        left_section_idx = 1;

        if (calcLeftSection() <= 0) { // There's only one left section, thus if its height is zero the whole triangle has zero-height
            return;
        }
        if (calcRightSection() <= 0) {
            // It's a top-flat triangle, use the next section.
            right_section_idx--;
            if (calcRightSection() <= 0) { // With top flat triangles, there's also only one right section, thus the above applies analogously.
                return;
            }
        }
    } else { // Middle vertex is on the left side
        left_array[0] = v3;
        left_array[1] = v2;
        left_array[2] = v1;
        left_section_idx = 2;
        right_array[0] = v3;
        right_array[1] = v1;
        right_section_idx = 1;

        if (calcRightSection() <= 0) { // There's only one right section, thus if its height is zero the whole triangle has zero-height
            return;
        }

        if (calcLeftSection() <= 0) {
            // It's a top-flat triangle, use the next section.
            left_section_idx--;
            if (calcLeftSection() <= 0) { // With top flat triangles, there's also only one right section, thus the above applies analogously.
                return;
            }
        }
    }
    int y = MAX(0, v1->y);
    while (1) {
        const int x1 = FIXED_16_2_INT_CEIL(left_x);
        const int x2 = FIXED_16_2_INT_CEIL(right_x) - 1;
        if (!(x1 < 0 &&  x2 < 0) && !(x1 >= M5_SCALED_W && x2 >= M5_SCALED_W)) { // Horizontal "clipping": Don't draw if *both* x-positions are either to the left, or both are to the right of the screen.
            m5_hline_nonorm(MAX(0, x1), y, MIN(M5_SCALED_W - 1, x2), tri->color);
        }
      
        if (--left_section_height <= 0) { // Check if we've reached the bottom of the left section. 
            if (--left_section_idx <= 0)
                return;
            if (calcLeftSection() <= 0)
                return;
        } else { // No? Step along the left side (DDA). 
            left_x += delta_left_x;
        }
        if (--right_section_height <= 0) { // Check if we've reached the bottom of the right section. 
            if (--right_section_idx <= 0)
                return;
            if (calcRightSection() <= 0)
                return;
        } else { // No? Step along the right side (DDA).
            right_x += delta_right_x;
        }
        ++y;
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
        const Vec3 faceNormal = vecTransformed(instanceRotMat, face.normal); /* Rotate the face's normal (in model space). */   \
        const FIXED lightAlpha = vecDot(lightDir, faceNormal);                                                                  \
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


INLINE void otInsert(RasterTriangle *t) {
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
    Calculates the screen-space triangles which can be sorted and drawn later. 
*/ 
 IWRAM_CODE static void modelInstancesPrepareDraw(Camera* cam, ModelInstance *instances, int numInstances, ModelDrawLightingData lightDat) 
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
            const Vec3 a = vecSub(vertsCamSpace[face.vertexIndex[1]], vertsCamSpace[face.vertexIndex[0]]);
            const Vec3 b = vecSub(vertsCamSpace[face.vertexIndex[2]], vertsCamSpace[face.vertexIndex[0]]);
            const Vec3 triNormal = vecCross(b, a);
            // const Vec3 triNormal = vecTransformed(instanceRotMat, face.normal);
            const Vec3 camToTri = vertsCamSpace[face.vertexIndex[2]]; // Remember, vertsCamSpace[] is in camera space already, so it doesn't make sense subtract the camera's wolrd position!
                        
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

IWRAM_CODE void drawModelInstancePools(ModelInstancePool *pools, int numPools, Camera *cam, ModelDrawLightingData lightDat) 
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

    // RasterTriangle tri;
    // tri.color = CLR_WHITE;
    // tri.vert[0] = (RasterPoint){.x=0, .y=0};
    // tri.vert[1] = (RasterPoint){.x=60, .y=100};
    // tri.vert[2] = (RasterPoint){.x=0, .y=100};
    // drawTriangleFlatByggmastar(&tri);
    performanceEnd(perfTotal);
    char dbg[64];
    snprintf(dbg, sizeof(dbg),  "tris: %d", screenTriangleCount);
    m5_puts(8, 24, dbg, CLR_FUCHSIA);
}
