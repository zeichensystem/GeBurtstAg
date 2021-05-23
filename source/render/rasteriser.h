#ifndef RASTERISER_H
#define RASTERISER_H

#include <tonc.h>
#include "../globals.h"
#include "../raster_geometry.h"

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

#endif