#ifndef ECT_MATH_H
#define ECT_MATH_H

#include "ect_types.h"

#include <assert.h>

#ifndef USE_CUSTOM_CONSTS
   #define ECT_PI32 3.141592f
   #define ECT_PI64 3.141592653589793
#endif

#ifndef USE_CUSTOM_MATH
   #include <math.h>
   #define ECT_SIN32 sinf
   #define ECT_SIN64 sin

   #define ECT_COS32 cosf
   #define ECT_COS64 cos

   #define ECT_SIN(v) _Generic((v), \
      f32: ECT_SIN32((v)), \
      f64: ECT_SIN64((v)), \
      default: assert(1) \
   )

   #define ECT_COS(v) _Generic((v), \
      f32: ECT_COS32((v)), \
      f64: ECT_COS64((v)), \
      default: assert(1) \
   )
#endif

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

   vec3 o = {{
      origin.x + f * distance,
      origin.y - c * distance,
      origin.z + h * distance }};

   f32 x = a * o.x - b * o.z;
   f32 y = e * o.x + c * o.y + g * o.z;
   f32 z = f * o.x - d * o.y + h * o.z;
   return (mat4x4) {{
       a, e, f, 0,
       0, c,-d, 0,
      -b, g, h, 0,
      -x,-y,-z, 1
   }};
}

#endif
