#include <math.h>
#include <string.h>
#include <tonc.h>

#include "math.h"
#include "logutils.h"
#include "timer.h"

// #define MATH_FAST_DIVISION (TODO: We don't use a LUT for divisions for now (which is not ideal); I ran into issues since the range of divisors was quite limited with the LUT, so I'll figure it out later)

/*
    NOTE: The matrices and vectors are assumed to be in "column major order" conceptually throughout our whole codebase. 
    (We store matrices/vectors flat (one-dimensionally) in memory row after row, but that's an implementation detail, and should not matter conceptually.)
    Therefore, we post-multiply matrices with vectors, e.g. v' = M * v where M is a (4x4) matrix, and v and v' are (4x1) column vectors. 
*/

static int perfID;

void mathInit(void) 
{
    perfID = performanceDataRegister("math: func");
}


Vec3 vecTransformed(const FIXED matrix[16], Vec3 vec) 
{
    Vec3 transformed;
    transformed.x = fxmul(vec.x, matrix[0]) + fxmul(vec.y, matrix[1]) + fxmul(vec.z, matrix[2])  + matrix[3];
    transformed.y = fxmul(vec.x, matrix[4]) + fxmul(vec.y, matrix[5]) + fxmul(vec.z, matrix[6])  + matrix[7];
    transformed.z = fxmul(vec.x, matrix[8]) + fxmul(vec.y, matrix[9]) + fxmul(vec.z, matrix[10]) + matrix[11];
    FIXED w = fxmul(vec.x, matrix[12]) + fxmul(vec.y, matrix[13]) + fxmul(vec.z, matrix[14]) + matrix[15];
    
    if (w != int2fx(1)) { // If it's not an affine transform (e.g. perspective projection), we have to explicitly convert homogenous coordinates back to cartesian. 
        assertion(w != 0,  "w != 0");
        #ifdef MATH_FAST_DIVISION
        transformed.x = fxDivFast(transformed.x, w);
        transformed.y = fxDivFast(transformed.y, w);
        transformed.z = fxDivFast(transformed.z, w); 
        #else
        transformed.x = fxdiv(transformed.x, w);
        transformed.y = fxdiv(transformed.y, w);
        transformed.z = fxdiv(transformed.z, w); 
        #endif
    }
    return transformed;
}

void vecTransform(const FIXED matrix[16], Vec3 *vec) 
{
    Vec3 transformed;
    transformed.x = fxmul(vec->x, matrix[0]) + fxmul(vec->y, matrix[1]) + fxmul(vec->z, matrix[2])  + matrix[3];
    transformed.y = fxmul(vec->x, matrix[4]) + fxmul(vec->y, matrix[5]) + fxmul(vec->z, matrix[6])  + matrix[7];
    transformed.z = fxmul(vec->x, matrix[8]) + fxmul(vec->y, matrix[9]) + fxmul(vec->z, matrix[10]) + matrix[11];
    FIXED w = fxmul(vec->x, matrix[12]) + fxmul(vec->y, matrix[13]) + fxmul(vec->z, matrix[14]) + matrix[15];
    *vec = transformed;
    if (w != int2fx(1)) { // If it's not an affine transform (e.g. perspective projection), we have to explicitly convert homogenous coordinates back to cartesian. 
        assertion(w != 0,  "w != 0");
        #ifdef MATH_FAST_DIVISION
        vec->x = fxDivFast(transformed.x, w);
        vec->y = fxDivFast(transformed.y, w);
        vec->z = fxDivFast(transformed.z, w); 
        #else
        vec->x = fxdiv(transformed.x, w);
        vec->y = fxdiv(transformed.y, w);
        vec->z = fxdiv(transformed.z, w); 
        #endif
    }
}

Vec3 vecScaled(Vec3 vec, FIXED factor) 
{
    vec.x = fxmul(vec.x, factor);
    vec.y = fxmul(vec.y, factor);
    vec.z = fxmul(vec.z, factor);
    return vec;
}

void vecScale(Vec3 *vec, FIXED factor) 
{
    vec->x = fxmul(vec->x, factor);
    vec->y = fxmul(vec->y, factor);
    vec->z = fxmul(vec->z, factor);
}

Vec3 vecAdd(Vec3 a, Vec3 b) 
{
    a.x = fxadd(a.x, b.x);
    a.y = fxadd(a.y, b.y);
    a.z = fxadd(a.z, b.z);
    return a;
}

Vec3 vecSub(Vec3 a, Vec3 b) 
{
    a.x = fxsub(a.x, b.x);
    a.y = fxsub(a.y, b.y);
    a.z = fxsub(a.z, b.z);
    return a;
}

Vec3 vecUnit(Vec3 a) 
{
    FIXED mag =  Sqrt((fxmul(a.x, a.x) + fxmul(a.y, a.y) + fxmul(a.z, a.z)) << (FIX_SHIFT) ); // sqrt(2**8) * sqrt(2**8) = 2**8
    return (Vec3){.x = fxdiv(a.x, mag), .y=fxdiv(a.y, mag), .z=fxdiv(a.z, mag) };
}

FIXED vecMag(Vec3 a) {
     return Sqrt((fxmul(a.x, a.x) + fxmul(a.y, a.y) + fxmul(a.z, a.z)) << (FIX_SHIFT) ); // sqrt(2**8) * sqrt(2**8) = 2**8
}


Vec3 vecCross(Vec3 a, Vec3 b) 
{
    Vec3 cross;
    cross.x = fxmul(a.y, b.z) - fxmul(a.z, b.y);
    cross.y = fxmul(a.z, b.x) - fxmul(a.x, b.z);
    cross.z = fxmul(a.x, b.y) - fxmul(a.y, b.x);
    return cross;
}

FIXED vecDot(Vec3 a, Vec3 b) 
{
    return fxmul(a.x, b.x) + fxmul(a.y, b.y) + fxmul(a.z, b.z);
}


FIXED matrix4x4Get(const FIXED matrix[16], int row, int col) 
{
    return matrix[row * 4 + col];
}

void matrix4x4Set(FIXED matrix[16], int row, int col, FIXED value) 
{
    matrix[row * 4 + col] = value;
}

void matrix4x4setIdentity(FIXED matrix[16]) 
{
    memset(matrix, 0, sizeof(*matrix) * 16);
    matrix[0] = int2fx(1);
    matrix[5] = int2fx(1);
    matrix[10] = int2fx(1);
    matrix[15] = int2fx(1);
}

void matrix4x4SetScale(FIXED matrix[16], FIXED scalar) 
{
    matrix[0]  = scalar;
    matrix[5]  = scalar;
    matrix[10] = scalar;
    matrix[15] = scalar;
}

void matrix4x4Scale(FIXED matrix[16], FIXED scalar) 
{
    matrix[0]  = fxmul(matrix[0], scalar);
    matrix[5]  = fxmul(matrix[5], scalar);
    matrix[10] = fxmul(matrix[10], scalar);
    matrix[15] = fxmul(matrix[15], scalar);
}

void matrix4x4SetTranslation(FIXED matrix[16], Vec3 translation) 
{
    matrix[3] = translation.x;
    matrix[7] = translation.y;
    matrix[11] = translation.z;
}

void matrix4x4AddTranslation(FIXED matrix[16], Vec3 translation) 
{
    matrix[3] += translation.x;
    matrix[7] += translation.y;
    matrix[11] += translation.z;
}

void matrix4x4SetBasis(FIXED matrix[16], Vec3 x, Vec3 y, Vec3 z) 
{
    matrix[0] = x.x;
    matrix[1] = y.x;
    matrix[2] = z.x;

    matrix[4] = x.y;
    matrix[5] = y.y;
    matrix[6] = z.y;

    matrix[8] = x.z;
    matrix[9] = y.z;
    matrix[10] = z.z;
}

Vec3 matrix4x4GetTranslation(const FIXED matrix[16]) 
{
    Vec3 pos = {.x = matrix[3], .y = matrix[7], .z = matrix[11]};
    return pos;
}

void matrix4x4createRotX(FIXED matrix[16], ANGLE_FIXED_12 angle) 
{
    matrix4x4setIdentity(matrix);
    // x y z tx
    // x y z ty
    // x y z tz
    // 0 0 0 w
    // y-basis
    matrix[5] = cosFx(angle);
    matrix[9] = sinFx(angle); // minus
    // z-basis
    matrix[6] = -sinFx(angle); // plus
    matrix[10] = cosFx(angle);
}

void matrix4x4createRotY(FIXED matrix[16], ANGLE_FIXED_12 angle) 
{
    matrix4x4setIdentity(matrix);
    // x-basis
    matrix[0] = cosFx(angle);
    matrix[8] = -sinFx(angle);
    // z-basis
    matrix[2] = sinFx(angle);
    matrix[10] = cosFx(angle);
}

void matrix4x4createRotZ(FIXED matrix[16], ANGLE_FIXED_12 angle) 
{
    matrix4x4setIdentity(matrix);
    // x-basis
    matrix[0] = cosFx(angle);
    matrix[4] = sinFx(angle);
    // y-basis
    matrix[1] = -sinFx(angle);
    matrix[5] = cosFx(angle);
}

void matrix4x4createYawPitchRoll(FIXED matrix[16], ANGLE_FIXED_12 yaw, ANGLE_FIXED_12 pitch, ANGLE_FIXED_12 roll) 
{
    matrix4x4setIdentity(matrix);
    // M = rotYaw * rotPitch * rotRoll (to think of the rotations in local space, read from left to right (we use coloum major vectors/matrices))
    matrix[0] = fxmul(cosFx(roll), cosFx(yaw)) + fxmul(fxmul(sinFx(roll), sinFx(yaw)), sinFx(pitch));
    matrix[1] = -fxmul(sinFx(roll), cosFx(yaw)) + fxmul(fxmul(cosFx(roll), sinFx(yaw)), sinFx(pitch));
    matrix[2] = fxmul(sinFx(yaw), cosFx(pitch));
    // matrix[3] = 0;
    matrix[4] = fxmul(sinFx(roll), cosFx(pitch));
    matrix[5] = fxmul(cosFx(roll), cosFx(pitch));
    matrix[6] = -sinFx(pitch);
    // matrix[7] = 0;
    matrix[8] = fxmul(-cosFx(roll), sinFx(yaw)) + fxmul(fxmul(sinFx(roll), cosFx(yaw)), sinFx(pitch));
    matrix[9] = fxmul(sinFx(roll), sinFx(yaw)) + fxmul(fxmul(cosFx(roll), cosFx(yaw)), sinFx(pitch));
    matrix[10] = fxmul(cosFx(pitch), cosFx(yaw));
}

void matrix4x4Mul(FIXED a[16], const FIXED b[16]) 
{
    FIXED result[16];
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            FIXED value = int2fx(0);
            for (int i = 0; i < 4; ++i) {
                value = value + fxmul(matrix4x4Get(a, row, i), matrix4x4Get(b, i, col));
            }
            matrix4x4Set(result, row, col, value);
        }
    }
    memcpy(a, result, sizeof(result));
}

void matrix4x4createMul(const FIXED a[16], const FIXED b[16], FIXED result[16]) 
{
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            FIXED value = int2fx(0);
            for (int i = 0; i < 4; ++i) {
                value = fxadd(value, fxmul(matrix4x4Get(a, row, i), matrix4x4Get(b, i, col)) );
            }
            matrix4x4Set(result, row, col, value);
        }
    }
}

void matrix4x4Transpose(FIXED mat[16]) 
{ // Useful, as the inversion of a square orthonormal matrix is equivalent to its transposition. We don't really need to invert other matrices so far. 
    FIXED tmp[16];
    memcpy(tmp, mat, sizeof(tmp[0]) * 16);
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            matrix4x4Set(mat, row, col, matrix4x4Get(tmp, col, row));
        }
    }
}


// Not my code, also not used anymore , and only included for reference. Numerical matrix inversion code: https://stackoverflow.com/questions/1148309/inverting-a-4x4-matrix
FIXED invf(int i,int j,const FIXED* m) 
{
    int o = 2+(j-i);
    i += 4+o;
    j += 4-o;
    #define e(a,b) m[ ((j+b)%4)*4 + ((i+a)%4) ]

    FIXED inv = 
     + fxmul(fxmul(e(+1,-1), e(+0,+0)), e(-1,+1) )
     + fxmul(fxmul(e(+1,+1), e(+0,-1)), e(-1,+0))
     + fxmul(fxmul(e(-1,-1), e(+1,+0)), e(+0,+1))
     - fxmul(fxmul(e(-1,-1), e(+0,+0)), e(+1,+1))
     - fxmul(fxmul(e(-1,+1), e(+0,-1)), e(+1,+0))
     - fxmul(fxmul(e(+1,-1), e(-1,+0)), e(+0,+1));

    return (o%2) ? inv : -inv;
    #undef e
}

bool matrix4x4Inverse(const FIXED *m, FIXED *out)
{
    FIXED inv[16];

    for(int i=0;i<4;i++)
        for(int j=0;j<4;j++)
            inv[j*4+i] = invf(i,j,m);

    FIXED D = int2fx(0);

    for(int k=0;k<4;k++) D = fxadd(D, fxmul(m[k], inv[k*4]));

    if (D == int2fx(0)) return false;

    D = fxdiv(int2fx(1), D);

    for (int i = 0; i < 16; i++)
        out[i] = fxmul(inv[i], D);

    return true;
}
