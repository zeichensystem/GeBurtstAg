#include <string.h>
#include <stdlib.h>
#include <tonc.h>

#include "globals.h"
#include "math.h"
#include "draw.h"
#include "logutils.h"
#include "model.h"
#include "clipping.h"

#define DRAW_MAX_TRIANGLES 321
EWRAM_DATA static RasterTriangle screenTriangles[DRAW_MAX_TRIANGLES]; 
static int screenTriangleCount = 0;

#define RASTERPOINT_IN_BOUNDS_M5(vert) (vert.x >= 0 && vert.x < M5_SCALED_W && vert.y >= 0 && vert.y < M5_SCALED_H)
#define BEHIND_CAM(vert) (vert.z > -cam->near ) // True if the Vec3 is behind the near plane of the camera (i.e. invisible).

static int perfFill, perfModelProcessing, perfPolygonSort, perfProject;


void drawInit() 
{
    REG_DISPCNT = g_mode | DCNT_BG2;
    txt_init_std();
    perfFill = performanceDataRegister("draw.c: rasterisation");
    perfModelProcessing = performanceDataRegister("draw:c pre-rasterisation");
    perfPolygonSort = performanceDataRegister("draw.c: Polygon-sort");
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
        if (BEHIND_CAM(pointCamSpace)) {
            continue;
        }
        vecTransform(cam->perspMat, &pointCamSpace);
        RasterPoint rp = {.x = fx2int(pointCamSpace.x), .y=fx2int(pointCamSpace.y)};
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
                lightDir = vecUnit(vecSub((Vec3){0, 0, 0}, *lightDat.light.directional)); /* Invert the sign. */                                                            \
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
        for (int i = 0; i < instance->state.mod.numVerts; ++i) {
            // Model space to world space:
            vertsCamSpace[i] = vecScaled(instance->state.mod.verts[i], instance->state.scale);
            vecTransform(instanceRotMat, vertsCamSpace + i );
            // We translate manually so that instanceRotMat stays as is (so we can rotate our normals with the instanceRotMat in model space to calculate lighting):
            vertsCamSpace[i].x += instance->state.pos.x;
            vertsCamSpace[i].y += instance->state.pos.y;
            vertsCamSpace[i].z += instance->state.pos.z;
            vecTransform(cam->world2cam, vertsCamSpace + i); // And finally, we're in camera space.
        }

        // Calculate lightDir and attenuation (which don't depend on the faces, only on the instance) so we don't have to re-compute them redundantly in the inner loop over the faces.
        INSTANCE_CALC_LIGHTDIR_AND_ATTENUATION();
            
        for (int faceNum = 0; faceNum < instance->state.mod.numFaces; ++faceNum) { // For each face (triangle, really) of the ModelInstace. 
            const Face face = instance->state.mod.faces[faceNum];

             // Backface culling (assumes a clockwise winding order):
            const Vec3 a = vecSub(vertsCamSpace[face.vertexIndex[1]], vertsCamSpace[face.vertexIndex[0]]);
            const Vec3 b = vecSub(vertsCamSpace[face.vertexIndex[2]], vertsCamSpace[face.vertexIndex[0]]);
            const Vec3 triNormal = vecCross(b, a);
            const Vec3 camToTri = vertsCamSpace[face.vertexIndex[0]]; // Remember, vertsCamSpace[] is in camera space already, so it doesn't make sense subtract the camera's wolrd position!
            if (vecDot(triNormal, camToTri) <= 0) { // If the angle between camera and normal is not between 90 degs and 270 degs, the face is invisible and to be culled.
                continue;
            }

            Vec3 triVerts[3] = {vertsCamSpace[face.vertexIndex[0]], vertsCamSpace[face.vertexIndex[1]], vertsCamSpace[face.vertexIndex[2]]};
            // TODO: Proper 3d clipping against the near plane (so far, triangles are discared even if only one vertex is behind the near plane).
            if (BEHIND_CAM(triVerts[0]) || BEHIND_CAM(triVerts[1]) || BEHIND_CAM(triVerts[2])) {  // Triangle extends behind the near clipping plane -> reject the whole thing. 
                continue; 
            } else if (triVerts[0].z < -cam->far || triVerts[1].z < -cam->far || triVerts[1].z < -cam->far) { // Triangle extends beyond the far-plane -> reject the whole thing.
                continue;
            }
            RasterTriangle screenTri;
            performanceStart(perfProject);
            for (int i = 0; i < 3; ++i) { // Perspective projection, and conversion of the fixed point numbers to integers.
                assertion(triVerts[i].z <= -cam->near, "draw.c: drawModelInstances: Perspective division in front of near-plane");
                vecTransform(cam->perspMat, triVerts + i);
                screenTri.vert[i] = (RasterPoint){.x=fx2int(triVerts[i].x), .y=fx2int(triVerts[i].y)};   
            }
            performanceEnd(perfProject);
            // Check if all vertices of the face are to the "outside-side" of a given clipping plane. If so, the face is invisible and we can skip it.
            if (triVerts[0].x < 0 && triVerts[1].x < 0 && triVerts[2].x < 0) { // All vertices are to the left of the left-plane.
                continue;
            } else if (triVerts[0].x >= cam->canvasWidth && triVerts[1].x >= cam->canvasWidth && triVerts[2].x >= cam->canvasWidth ) { // All vertices are to the right of the right-plane.
                continue;
            } else if (triVerts[0].y < 0 && triVerts[1].y < 0 && triVerts[2].y < 0) { // All vertices are to the top of the top-plane.
                continue;
            } else if (triVerts[0].y >= cam->canvasHeight && triVerts[1].y >= cam->canvasHeight && triVerts[2].y >= cam->canvasHeight) { // All vertices are to the bottom of the bottom-plane.
                continue;
            }

            FACE_CALC_COLOR();
            screenTri.shading = instance->state.shading;

            screenTri.centroidZ = vertsCamSpace[face.vertexIndex[0]].z; // TODO HACK: That's not the actual centroid (we'd need an expensive division for that), but this is good enough for small faces.
            assertion(screenTriangleCount < DRAW_MAX_TRIANGLES, "draw.c: drawModelInstances: screenTriangleCount < DRAW_MAX_TRIANGLES");
            screenTriangles[screenTriangleCount++] = screenTri;
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
    screenTriangleCount = 0;
    performanceStart(perfModelProcessing);
    for (int i = 0; i < numPools; ++i) { 
        modelInstancesPrepareDraw(cam, pools[i].instances, pools[i].POOL_CAPACITY, lightDat);
    }
    performanceEnd(perfModelProcessing);

    qsort(screenTriangles, screenTriangleCount, sizeof(screenTriangles[0]), triangleDepthCmp);

    performanceStart(perfFill);
    for (int i = 0; i < screenTriangleCount; ++i) {
        if (screenTriangles[i].shading == SHADING_FLAT || screenTriangles[i].shading == SHADING_FLAT_LIGHTING) {
            drawTriangleFlatByggmastar(screenTriangles + i);
        } else {
            drawTriangleWireframe(screenTriangles + i);
        }
    }
    performanceEnd(perfFill);

    // RasterTriangle tri;
    // tri.color = CLR_WHITE;
    // tri.vert[0] = (RasterPoint){.x=0, .y=0};
    // tri.vert[1] = (RasterPoint){.x=60, .y=100};
    // tri.vert[2] = (RasterPoint){.x=0, .y=100};
    // drawTriangleFlatByggmastar(&tri);

    char dbg[64];
    snprintf(dbg, sizeof(dbg),  "tris: %d", screenTriangleCount);
    m5_puts(8, 24, dbg, CLR_FUCHSIA);
}




/*
    DDA triangle-filling (flat-shaded). Uses a top-left fill convention to avoid overdraw/gaps. 
    The parts of triangles outside of the screen are simply not drawn (we avoid having to do 2d-clipping this way; it's faster than the Sutherland-Hodgman 2d-clipping approach I've tested).
INLINE void drawTriangleFlat(const RasterTriangle *tri) 
{ 
    const RasterPoint *v1 = tri->vert;
    const RasterPoint *v2 = tri->vert + 1;
    const RasterPoint *v3 = tri->vert + 2;
    COLOR triColor = tri->color;
  
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
    if (v1->y == v3->y) { // Degenerate triangle, we don't draw those.
        return;
    }
 
    const int dy1 = v2->y - v1->y;
    const int dx1 = v2->x - v1->x;
    const int dx2 = v3->x - v1->x;
    const int dy2 = v3->y - v1->y;
    const int cross = dx1 * dy2 - dy1 * dx2;
    const bool middleLeft = cross < 0; // If middleLeft is true, the triangle's left side consists of two edges, otherwise, it's the right side which consists of two edges.
    // // First, we fill the top section of the triangle:
    FIXED invslopeLong = int2fx(v3->x - v1->x) / (v3->y - v1->y);
    FIXED invslopeShort; 
    if (v2->y - v1->y) { // For invslopeShort, it's important to avoid a division by zero in case we have a triangle with a flat top.
        invslopeShort = int2fx(v2->x - v1->x) / (v2->y - v1->y);
    } else {
        assertion(v3->y - v2->y, "draw.c: drawFillTris: v3.y - v2.y != 0");
        invslopeShort = int2fx(v3->x - v2->x) / (v3->y - v2->y); 
    }
    int yStart = MAX(0, v1->y);
    int yEnd = MIN(M5_SCALED_H - 1, v2->y);
    int dy = yStart - v1->y; 
    FIXED leftDeltaX = middleLeft ? invslopeShort : invslopeLong;
    FIXED rightDeltaX = middleLeft ? invslopeLong : invslopeShort;
    FIXED xLeft = int2fx(v1->x) + dy * leftDeltaX;
    FIXED xRight = int2fx(v1->x) + dy * rightDeltaX;
    for (int y = yStart; y < yEnd; ++y) { 
        xLeft += leftDeltaX;
        xRight += rightDeltaX;
        const int left = MIN(MAX(0, fx2int(xLeft)), M5_SCALED_W - 1);
        const int right = MIN(MAX(0, fx2int(xRight)), M5_SCALED_W - 1);
        m5_hline_nonorm(left, y, right - 1, triColor);
    }
    // Finally, we fill the bottom section of the triangle:
    if (v2->y < v3->y) { // Avoid division by zero in case there is no bottom half.
        invslopeShort = int2fx(v3->x - v2->x) / (v3->y - v2->y);
    } else {
        return;
    }
    leftDeltaX = middleLeft ? invslopeShort : invslopeLong;
    rightDeltaX = middleLeft ? invslopeLong : invslopeShort;
    yStart = MAX(0, v2->y);
    yEnd = MIN(M5_SCALED_H - 1, v3->y);
    dy = yStart - v2->y; 
    xLeft = middleLeft ? int2fx(v2->x) + dy * leftDeltaX : xLeft + leftDeltaX;
    xRight = middleLeft ? xRight + rightDeltaX : int2fx(v2->x) + dy * rightDeltaX;
    for (int y = yStart; y < yEnd; ++y) { 
        const int left = MIN(MAX(0, fx2int(xLeft)), M5_SCALED_W - 1);
        const int right = MIN(MAX(0, fx2int(xRight)), M5_SCALED_W - 1);
        m5_hline(left, y, right - 1, triColor);
        xLeft += leftDeltaX;
        xRight += rightDeltaX;
    }
}
*/