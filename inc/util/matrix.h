#ifndef UTIL_MATRIX_H
#define UTIL_MATRIX_H

#include "util/types.h"
#include "util/math.h"
#include "util/vec3.h"
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
   f32 s = 2.0f * M_RCP(n, M_FLOAT_FUZZ);

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

// Taken from HandmadeMath
// https://github.com/HandmadeMath/HandmadeMath/blob/142ba3cd9da700e3e599d301bbd27d415ad14626/HandmadeMath.h#L1696
static inline mat4x4 Util_InverseMat4(mat4x4 matrix)
{
   vec3 c0_1 = Util_CrossVec3(matrix.v[0].xyz, matrix.v[1].xyz);
   vec3 c2_3 = Util_CrossVec3(matrix.v[2].xyz, matrix.v[3].xyz);
   vec3 b1_0 = Util_SubVec3(Util_ScaleVec3(matrix.v[0].xyz, matrix.v[1].w), Util_ScaleVec3(matrix.v[1].xyz, matrix.v[0].w));
   vec3 b3_2 = Util_SubVec3(Util_ScaleVec3(matrix.v[2].xyz, matrix.v[3].w), Util_ScaleVec3(matrix.v[3].xyz, matrix.v[2].w));

   f32 rcp_determinant = M_RCP(Util_DotVec3(c0_1, b3_2) + Util_DotVec3(c2_3, b1_0), M_FLOAT_FUZZ);
   c0_1 = Util_ScaleVec3(c0_1, rcp_determinant);
   c2_3 = Util_ScaleVec3(c2_3, rcp_determinant);
   b1_0 = Util_ScaleVec3(b1_0, rcp_determinant);
   b3_2 = Util_ScaleVec3(b3_2, rcp_determinant);

   vec3 v0 = Util_AddVec3(Util_CrossVec3(matrix.v[1].xyz, b3_2), Util_ScaleVec3(c2_3, matrix.v[1].w));
   f32 w0 =-Util_DotVec3(matrix.v[1].xyz, c2_3);

   vec3 v1 = Util_SubVec3(Util_CrossVec3(b3_2, matrix.v[0].xyz), Util_ScaleVec3(c2_3, matrix.v[0].w));
   f32 w1 = Util_DotVec3(matrix.v[0].xyz, c2_3);

   vec3 v2 = Util_AddVec3(Util_CrossVec3(matrix.v[3].xyz, b1_0), Util_ScaleVec3(c0_1, matrix.v[3].w));
   f32 w2 =-Util_DotVec3(matrix.v[3].xyz, c0_1);

   vec3 v3 = Util_SubVec3(Util_CrossVec3(b1_0, matrix.v[2].xyz), Util_ScaleVec3(c0_1, matrix.v[2].w));
   f32 w3 = Util_DotVec3(matrix.v[2].xyz, c0_1);

   return MAT4(
      v0.x, v1.x, v2.x, v3.x,
      v0.y, v1.y, v2.y, v3.y,
      v0.z, v1.z, v2.z, v3.z,
        w0,   w1,   w2,   w3
   );
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

static inline mat4x4 Util_RotationMatrix(vec3 axis, f32 angle)
{
   f32 c = M_COS(angle);
   f32 s = M_SIN(angle);

   vec3 ca = Util_ScaleVec3(axis, 1.0f - c);
   vec3 v1 = Util_MulVec3(axis, ca);
   vec3 v2 = Util_MulVec3(VEC3(axis.y, axis.z, axis.z), VEC3(ca.x, ca.x, ca.y));

   f32 x = axis.x * s;
   f32 y = axis.y * s;
   f32 z = axis.z * s;

   return MAT4(
      v1.x + c, v2.x - z, v2.y + y, 0,
      v2.x + z, v1.y + c, v2.z - x, 0,
      v2.y - y, v2.z + x, v1.z + c, 0,
      0, 0, 0, 1
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

static inline mat4x4 Util_InverseViewMatrix(mat4x4 view_matrix)
{
   mat4x4 res = {
      view_matrix.m[0][0], view_matrix.m[1][0], view_matrix.m[2][0], 0,
      view_matrix.m[0][1], view_matrix.m[1][1], view_matrix.m[2][1], 0,
      view_matrix.m[0][2], view_matrix.m[1][2], view_matrix.m[2][2], 0,
      0, 0, 0, 1
   };

   res.v[3].xyz = Util_ScaleVec3(Util_MulMat4Vec4(res, view_matrix.v[3]).xyz,-1);

   return res;
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

static inline mat4x4 Util_InversePerspectiveMatrix(mat4x4 perspective_matrix)
{
   mat4x4 res = { 0 };
   res.m[0][0] = 1.0f / perspective_matrix.m[0][0];
   res.m[1][1] = 1.0f / perspective_matrix.m[1][1];
   res.m[2][2] = 0.0f;
   
   res.m[2][3] = 1.0f / perspective_matrix.m[3][2];
   res.m[3][3] = perspective_matrix.m[2][2] * res.m[2][3];
   res.m[3][2] = perspective_matrix.m[2][3];

   return res;
}

#endif
