#ifndef UTIL_MATH_H
#define UTIL_MATH_H

#include "util/types.h"

#include <stdlib.h>

#ifndef USE_CUSTOM_CONSTS
   #define M_PI32 3.141592f
   #define M_PI64 3.141592653589793
   #define M_INVPI32 (1.0f / M_PI32)
   #define M_INVPI64 (1.0 / M_PI64)
#endif

#define M_TURN32 (M_PI32 * 0.02f)
#define M_TURN64 (M_PI64 * 0.02)
#define M_TO_TURN32 (50.0f * M_INVPI32)
#define M_TO_TURN64 (50.0 * M_INVPI64)

#define M_TURN(a) _Generic((a), \
   f32: (M_TURN32 * (a)), \
   f64: (M_TURN64 * (a)), \
   default: abort() \
)

#define M_TO_TURN(a) _Generic((a), \
   f32: (M_TO_TURN32 * (a)), \
   f64: (M_TO_TURN64 * (a)), \
   default: abort() \
)

#ifndef USE_CUSTOM_MATH

   #include <math.h>

   #define M_SIN32(a) sinf(M_TURN32 * (a))
   #define M_SIN64(a) sin(M_TURN64 * (a))

   #define M_COS32(a) cosf(M_TURN32 * (a))
   #define M_COS64(a) cos(M_TURN64 * (a))

   #define M_TAN32(a) tanf(M_TURN32 * (a))
   #define M_TAN64(a) tan(M_TURN64 * (a))

   #define M_ATAN32(x) (M_TO_TURN32 * atanf(x))
   #define M_ATAN64(x) (M_TO_TURN64 * atan(x))

   #define M_SQRT32 sqrtf
   #define M_SQRT64 sqrt

   #define M_SIN(a) _Generic((a), \
      f32: M_SIN32((a)), \
      f64: M_SIN64((a)), \
      default: abort() \
   )

   #define M_COS(a) _Generic((a), \
      f32: M_COS32((a)), \
      f64: M_COS64((a)), \
      default: abort() \
   )

   #define M_TAN(a) _Generic((a), \
      f32: M_TAN32((a)), \
      f64: M_TAN64((a)), \
      default: abort() \
   )

   #define M_ATAN(x) _Generic((x), \
      f32: M_ATAN32((x)), \
      f64: M_ATAN64((x)), \
      default: abort() \
   )

   #define M_SQRT(x) _Generic((x), \
      f32: M_SQRT32((x)), \
      f64: M_SQRT64((x)), \
      default: abort() \
   )

#endif

#define M_ADD_VECN(N) { \
   vec##N res = a; \
   for (i32 i=0; i<N; i++) \
      res.arr[i] += b.arr[i]; \
   return res; \
}

#define M_SUB_VECN(N) { \
   vec##N res = a; \
   for (i32 i=0; i<N; i++) \
      res.arr[i] -= b.arr[i]; \
      return res; \
}

#define M_MUL_VECN(N) { \
   vec##N res = a; \
   for (i32 i=0; i<N; i++) \
      res.arr[i] *= b.arr[i]; \
      return res; \
}

#define M_SCALE_VECN(N) { \
   vec##N res = vector; \
   for (i32 i=0; i<N; i++) \
      res.arr[i] *= scalar; \
      return res; \
}

#define M_NORMALIZE_VECN(N) { \
   f32 len_sqr = 0.0f; \
   for (i32 i=0; i<N; i++) \
      len_sqr += vector.arr[i] * vector.arr[i]; \
   if(len_sqr < 0.00001f) \
      return vector; \
   f32 len_inv = 1.0f / M_SQRT(len_sqr); \
   vec##N res = vector; \
   for (i32 i=0; i<N; i++) \
      res.arr[i] *= len_inv; \
      return res; \
}
#define M_DOT_VECN(N) \
{ \
   f32 res = 0.0f; \
   for (i32 i=0; i<N; i++) \
      res += a.arr[i] * b.arr[i]; \
   return res; \
}

typedef struct Spatial3D_t
{
   vec3 origin;
   quat rotation;
   vec3 scale;
} Spatial3D;

typedef struct BoundingBox_t
{
   vec3 center;
   vec3 extents;
} BoundingBox;

static inline vec2 Util_AddVec2(vec2 a, vec2 b)
M_ADD_VECN(2)

static inline vec2 Util_SubVec2(vec2 a, vec2 b)
M_SUB_VECN(2)

static inline vec2 Util_MulVec2(vec2 a, vec2 b)
M_MUL_VECN(2)

static inline vec2 Util_ScaleVec2(vec2 vector, f32 scalar)
M_SCALE_VECN(2)

static inline vec2 Util_NormalizeVec2(vec2 vector)
M_NORMALIZE_VECN(2)

static inline f32 Util_DotVec2(vec2 a, vec2 b)
M_DOT_VECN(2)

static inline vec3 Util_AddVec3(vec3 a, vec3 b)
M_ADD_VECN(3)

static inline vec3 Util_SubVec3(vec3 a, vec3 b)
M_SUB_VECN(3)

static inline vec3 Util_MulVec3(vec3 a, vec3 b)
M_MUL_VECN(3)

static inline vec3 Util_ScaleVec3(vec3 vector, f32 scalar)
M_SCALE_VECN(3)

static inline vec3 Util_NormalizeVec3(vec3 vector)
M_NORMALIZE_VECN(3)

static inline f32 Util_DotVec3(vec3 a, vec3 b)
M_DOT_VECN(3)

static inline vec4 Util_AddVec4(vec4 a, vec4 b)
M_ADD_VECN(4)

static inline vec4 Util_SubVec4(vec4 a, vec4 b)
M_SUB_VECN(4)

static inline vec4 Util_MulVec4(vec4 a, vec4 b)
M_MUL_VECN(4)

static inline vec4 Util_ScaleVec4(vec4 vector, f32 scalar)
M_SCALE_VECN(4)

static inline vec4 Util_NormalizeVec4(vec4 vector)
M_NORMALIZE_VECN(4)

static inline f32 Util_DotVec4(vec4 a, vec4 b)
M_DOT_VECN(4)

static inline vec4 Util_MulMat4Vec4(mat4x4 matrix, vec4 vector)
{
   vec4 res = { 0 };
   for (i32 COL=0; COL<4; COL++)
      res.arr[COL] = Util_DotVec4(matrix.v[COL], vector);
   return res;
}

static inline mat4x4 Util_MulMat4(mat4x4 a, mat4x4 b)
{
   mat4x4 res = { 0 };
   for (i32 COL=0; COL<4; COL++)
      for (i32 ROW=0; ROW<4; ROW++)
         for (i32 k=0; k<4; k++)
            res.m[COL][ROW] += a.m[k][ROW] * b.m[COL][k];

   return res;
}

static inline mat4x4 Util_TransposeMatrix(mat4x4 matrix)
{
   mat4x4 res = { 0 };
   for (i32 COL=0; COL<4; COL++)
      for (i32 ROW=0; ROW<4; ROW++)
         res.m[COL][ROW] = matrix.m[ROW][COL];

   return res;
}

static inline mat4x4 Util_TranslationMatrix(vec3 translation)
{
   f32 x = translation.x;
   f32 y = translation.y;
   f32 z = translation.z;

   return MAT4(
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, 1, 0,
      x, y, z, 1
   );
}

static inline mat4x4 Util_ScalingMatrix(vec3 scale)
{
   f32 x = scale.x;
   f32 y = scale.y;
   f32 z = scale.z;

   return MAT4(
      x, 0, 0, 0,
      0, y, 0, 0,
      0, 0, z, 0,
      0, 0, 0, 1
   );
}

static inline mat4x4 Util_ViewMatrix(vec3 origin, vec3 euler, f32 distance)
{
   f32 a = M_COS(euler.y);
   f32 b = M_SIN(euler.y);
   f32 c = M_COS(euler.x);
   f32 d = M_SIN(euler.x);
   f32 e = b * d;
   f32 f = b * c;
   f32 g = a * d;
   f32 h = a * c;

   vec3 o = VEC3(
      origin.x + f * distance,
      origin.y - d * distance,
      origin.z + h * distance
   );

   f32 x = a * o.x - b * o.z;
   f32 y = e * o.x + c * o.y + g * o.z;
   f32 z = f * o.x - d * o.y + h * o.z;

   return MAT4(
       a, e, f, 0,
       0, c,-d, 0,
      -b, g, h, 0,
      -x,-y,-z, 1
   );
}

static inline mat4x4 Util_PerspectiveMatrix(f32 fov, f32 aspect_ratio, f32 near, f32 far)
{
   f32 R = 1.0f / (near - far);
   f32 Y = 1.0f / M_TAN(fov * 0.5f);
   f32 X = Y * aspect_ratio;
   f32 a = (near + far) * R;
   f32 b = (2.0f * near * far) * R;

   return MAT4(
      X, 0, 0, 0,
      0, Y, 0, 0,
      0, 0, a,-1,
      0, 0, b, 0
   );
}

#endif
