#ifndef ECT_MATH_H
#define ECT_MATH_H

#include "ect_types.h"

#include <stdlib.h>

#ifndef USE_CUSTOM_CONSTS
#define ECT_PI32 3.141592f
#define ECT_PI64 3.141592653589793
#define ECT_INVPI32 (1.0f / ECT_PI32)
#define ECT_INVPI64 (1.0 / ECT_PI64)
#endif

#define ECT_TURN32 (ECT_PI32 * 0.02f)
#define ECT_TURN64 (ECT_PI64 * 0.02)
#define ECT_TO_TURN32 (50.0f * ECT_INVPI32)
#define ECT_TO_TURN64 (50.0 * ECT_INVPI64)

#define ECT_TURN(a) _Generic((a), \
   f32: (ECT_TURN32 * (a)), \
   f64: (ECT_TURN64 * (a)), \
   default: abort() \
)

#define ECT_TO_TURN(a) _Generic((a), \
   f32: (ECT_TO_TURN32 * (a)), \
   f64: (ECT_TO_TURN64 * (a)), \
   default: abort() \
)

#ifndef USE_CUSTOM_MATH
#include <math.h>
#define ECT_SIN32(a) sinf(ECT_TURN32 * (a))
#define ECT_SIN64(a) sin(ECT_TURN64 * (a))

#define ECT_COS32(a) cosf(ECT_TURN32 * (a))
#define ECT_COS64(a) cos(ECT_TURN64 * (a))

#define ECT_TAN32(a) tanf(ECT_TURN32 * (a))
#define ECT_TAN64(a) tan(ECT_TURN64 * (a))

#define ECT_ATAN32(x) (ECT_TO_TURN32 * atanf(x))
#define ECT_ATAN64(x) (ECT_TO_TURN64 * atan(x))

#define ECT_SIN(a) _Generic((a), \
   f32: ECT_SIN32((a)), \
   f64: ECT_SIN64((a)), \
   default: abort() \
)

#define ECT_COS(a) _Generic((a), \
   f32: ECT_COS32((a)), \
   f64: ECT_COS64((a)), \
   default: abort() \
)

#define ECT_TAN(a) _Generic((a), \
   f32: ECT_TAN32((a)), \
   f64: ECT_TAN64((a)), \
   default: abort() \
)

#define ECT_ATAN(x) _Generic((x), \
   f32: ECT_ATAN32((x)), \
   f64: ECT_ATAN64((x)), \
   default: abort() \
)
#endif

static inline mat4x4 EctMulMat4(mat4x4 a, mat4x4 b)
{
   mat4x4 res = { 0 };
   for (i32 COL=0; COL<4; COL++)
      for (i32 ROW=0; ROW<4; ROW++)
            for (i32 k=0; k<4; k++)
      res.m[COL][ROW] += a.m[k][ROW] * b.m[COL][k];
   return res;
}

static inline mat4x4 EctTransposeMatrix(mat4x4 matrix)
{
   mat4x4 res = { 0 };
   for (i32 COL=0; COL<4; COL++)
      for (i32 ROW=0; ROW<4; ROW++)
      res.m[COL][ROW] = matrix.m[ROW][COL];
   return res;
}

static inline mat4x4 EctViewMatrix(vec3 origin, vec3 euler, f32 distance)
{
f32 a = ECT_COS(euler.y);
f32 b = ECT_SIN(euler.y);
f32 c = ECT_COS(euler.x);
f32 d = ECT_SIN(euler.x);
f32 e = b * d;
f32 f = b * c;
f32 g = a * d;
f32 h = a * c;

vec3 o = VEC3(
   origin.x + f * distance,
   origin.y - d * distance,
   origin.z + h * distance);

f32 x = a * o.x - b * o.z;
f32 y = e * o.x + c * o.y + g * o.z;
f32 z = f * o.x - d * o.y + h * o.z;
return MAT4(
    a, e, f, 0,
    0, c,-d, 0,
   -b, g, h, 0,
   -x,-y,-z, 1);
}

static inline mat4x4 EctPerspectiveMatrix(f32 fov, f32 aspect_ratio, f32 near, f32 far)
{
   f32 R = 1.0f / (far - near);
   f32 Y = 1.0f / ECT_TAN(fov * 0.5f);
   f32 X = Y * aspect_ratio;
   f32 a = far * R;
   f32 b = far * near * R;
   return MAT4(
      X, 0, 0, 0,
      0, Y, 0, 0,
      0, 0,-a,-1,
      0, 0,-b, 0
   );
}

#endif
