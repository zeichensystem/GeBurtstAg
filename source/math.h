#ifndef MATH_H
#define MATH_H

#include <tonc.h>

#include "commondefs.h"

// #define PI_FLT 3.14159265f
#define TAU 0xFFFF
#define PI 0x8000 // round(TAU / 2)
#define ONE_DEGREE  0xB6
#define FIXED_12_SCALE (1 << 12)

typedef struct Vec3 {
     FIXED x, y, z;
} ALIGN4 Vec3; 

typedef s32 FIXED_12;
typedef FIXED_12 ANGLE_FIXED_12;


IWRAM_CODE_ARM Vec3 vecScaled(Vec3 vec, FIXED factor);
IWRAM_CODE_ARM void vecScale(Vec3 *vec, FIXED factor);

IWRAM_CODE_ARM Vec3 vecAdd(Vec3 a, Vec3 b);
IWRAM_CODE_ARM Vec3 vecSub(Vec3 a, Vec3 b);
IWRAM_CODE_ARM Vec3 vecCross(Vec3 a, Vec3 b);
IWRAM_CODE_ARM FIXED vecDot(Vec3 a, Vec3 b);
IWRAM_CODE_ARM Vec3 vecUnit(Vec3 a);
IWRAM_CODE_ARM FIXED vecMag(Vec3 a);

IWRAM_CODE_ARM Vec3 vecTransformed(const FIXED matrix[16], Vec3 vec);
IWRAM_CODE_ARM void vecTransform(const FIXED matrix[16], Vec3 *vec);
IWRAM_CODE_ARM void vecTranformAffine(const FIXED matrix[16], Vec3 *vec);
IWRAM_CODE_ARM Vec3 vecTransformedRot(FIXED rotmat[16], const Vec3 *v);

IWRAM_CODE_ARM void matrix4x4setIdentity(FIXED matrix[16]);
IWRAM_CODE_ARM void matrix4x4SetTranslation(FIXED matrix[16], Vec3 translation);
IWRAM_CODE_ARM void matrix4x4AddTranslation(FIXED matrix[16], Vec3 translation);
IWRAM_CODE_ARM Vec3 matrix4x4GetTranslation(const FIXED matrix[16]);
IWRAM_CODE_ARM void matrix4x4SetScale(FIXED matrix[16], FIXED scalar);
IWRAM_CODE_ARM void matrix4x4Scale(FIXED matrix[16], FIXED scalar);
IWRAM_CODE_ARM void matrix4x4SetBasis(FIXED matrix[16],  Vec3 x, Vec3 y, Vec3 z);
IWRAM_CODE_ARM void matrix4x4Transpose(FIXED mat[16]);
IWRAM_CODE_ARM bool matrix4x4Inverse(const FIXED *m, FIXED *out);

IWRAM_CODE_ARM void matrix4x4createYawPitchRoll(FIXED matrix[16], ANGLE_FIXED_12 yaw, ANGLE_FIXED_12 pitch, ANGLE_FIXED_12 roll);
IWRAM_CODE_ARM void matrix4x4createRotX(FIXED matrix[16], ANGLE_FIXED_12 angle);
IWRAM_CODE_ARM void matrix4x4createRotY(FIXED matrix[16], ANGLE_FIXED_12 angle);
IWRAM_CODE_ARM void matrix4x4createRotZ(FIXED matrix[16], ANGLE_FIXED_12 angle);
IWRAM_CODE_ARM void matrix4x4Mul(FIXED a[16], const FIXED b[16]);
IWRAM_CODE_ARM void matrix4x4createMul(const FIXED a[16], const FIXED b[16], FIXED result[16]);

void mathInit(void);

INLINE FIXED_12 int2fx12(int num) {
    return (ANGLE_FIXED_12) num << 12;
}

INLINE FIXED_12 fx2fx12(FIXED num) { // invariant: FIX_SHIFT <= 12
    return num << (12 - FIX_SHIFT);
}
INLINE FIXED fx12Tofx(FIXED_12 num) {  // invariant: FIX_SHIFT <= 12
      return (FIXED)num >> (12 - FIX_SHIFT); // CAREFUL: usual arithmetic conversions promote the signed int to unsigned
 }

INLINE int fx12ToInt(FIXED_12 num) {
     return num >> 12;
 }

INLINE float fx12ToFloat(FIXED_12 num) {
     return num / (float)FIXED_12_SCALE;
 }

INLINE FIXED_12 float2fx12(float num) {
    return (FIXED_12) (num * FIXED_12_SCALE);
}

INLINE FIXED_12 fx12mul(FIXED_12 a, FIXED_12 b) {
    return (a * b) >> 12;
}

INLINE FIXED_12 fx12div(FIXED_12 a, FIXED_12 b) {
    return (a * FIXED_12_SCALE) / b;
}

INLINE FIXED cosFx(ANGLE_FIXED_12 alpha) {
    return fx12Tofx(lu_cos(ABS(alpha) & 0xffff) );
}

INLINE FIXED sinFx(FIXED_12 alpha) {
    return alpha < 0 ? -fx12Tofx(lu_sin(ABS(alpha) & 0xffff) ) : fx12Tofx(lu_sin(alpha & 0xffff));
}


// INLINE FIXED fxDivFast(FIXED num, FIXED denom) { // IMPORTANT: has limited range, up to RECIPROCAL_MAX_RANGE; should only be used for perspective divide
//     panic("fxdivfast is deprecated");
//     assertion(denom != 0, "lutfxDiv: denom != 0");
//     assertion(fx2int(denom) < RECIPROCAL_MAX_INT, "fxDivFast: lut out of range");
//     FIXED invDenom = reciprocalLUT[(ABS(denom) - 1) & (RECIPROCAL_LUT_SIZE - 1)]; // abs instead of uint
//     FIXED u_result = (num * invDenom) >> (RECIPROCAL_FRACT_SHIFT) >> (RECIPROCAL_FRACT_SHIFT - FIX_SHIFT);
//     return denom < 0 ? -u_result: u_result; // don't assume denom is positive
// }

INLINE FIXED_12 freq(FIXED_12 hz) {
    return fx12mul(hz, TAU);
}


INLINE FIXED_12 deg2fxangle(int angle_degrees) {
    return (ONE_DEGREE * (angle_degrees));
}

#endif