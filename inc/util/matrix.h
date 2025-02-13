#ifndef UTIL_MATRIX_H
#define UTIL_MATRIX_H

#include "util/types.h"
#include "util/math.h"
// #include "util/vec3.h"
#include "util/vec4.h"

#define M_MAT3X3_DIMENSION 3
#define M_MAT4X4_DIMENSION 4
#define M_MAT3X3_LENGTH 3*3
#define M_MAT4X4_LENGTH 4*4

static inline mat3x3 Util_IdentityMat3(void)
{
   return MAT3(
      1, 0, 0,
      0, 1, 0,
      0, 0, 1
   );
}

static inline mat4x4 Util_IdentityMat4(void)
{
   return MAT4(
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1
   );
}

static inline mat3x3 Util_MulMat3(mat3x3 a, mat3x3 b)
{
   mat3x3 res = { 0 };

   Util_MulMat_NxN(a.arr, b.arr, res.arr, 3);
   return res;
}

static inline mat3x3 Util_TransposeMat3(mat3x3 matrix)
{
   mat3x3 res = { 0 };

   Util_TransposeMat_NxN(matrix.arr, res.arr, 3);
   return res;
}

static inline mat3x3 Util_InverseDiagonalMat3(mat3x3 matrix)
{
   mat3x3 res = { 0 };

   Util_InverseDiagonalMat_NxN(matrix.arr, res.arr, M_MAT3X3_DIMENSION);
   return res;
}

static inline mat3x3 Util_QuatToMat3(quat quaternion)
{
   f32 n = Util_DotVec4(quaternion, quaternion);
   f32 s = (n > 0.0001f) ? (2.0f / n) : 0.0f;

   f32 x = quaternion.x;
   f32 y = quaternion.y;
   f32 z = quaternion.z;
   f32 w = quaternion.w;

   return MAT3(
      1.0f-(y*y + z*z) * s, (x*y - w*z) * s, (x*z + w*y) * s,
               (x*y + w*z) * s, 1.0f-(x*x + z*z) * s, (y*z - w*x) * s,
               (x*z - w*y) * s, (y*z + w*x) * s, 1.0f-(x*x + y*y) * s
   );
}

static inline mat4x4 Util_MulMat4(mat4x4 a, mat4x4 b)
{
   mat4x4 res = { 0 };

   Util_MulMat_NxN(a.arr, b.arr, res.arr, 4);
   return res;
}

static inline mat4x4 Util_TransposeMat4(mat4x4 matrix)
{
   mat4x4 res = { 0 };

   Util_TransposeMat_NxN(matrix.arr, res.arr, 4);
   return res;
}

static inline mat4x4 Util_InverseDiagonalMat4(mat4x4 matrix)
{
   mat4x4 res = { 0 };

   Util_InverseDiagonalMat_NxN(matrix.arr, res.arr, M_MAT4X4_DIMENSION);
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
