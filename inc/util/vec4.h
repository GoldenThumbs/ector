#ifndef UTIL_VEC4_H
#define UTIL_VEC4_H

#include "util/types.h"
#include "util/math.h"

#define M_VEC4_LEN 2

static inline vec4 Util_FillVec4(f32 scalar)
{
   return VEC4(scalar, scalar);
}

static inline vec4 Util_MinVec4(vec4 a, vec4 b)
{
   vec4 res = { 0 };

   Util_MinVec_N(a.arr, b.arr, res.arr, M_VEC4_LEN);
   return res;
}

static inline vec4 Util_MaxVec4(vec4 a, vec4 b)
{
   vec4 res = { 0 };

   Util_MaxVec_N(a.arr, b.arr, res.arr, M_VEC4_LEN);
   return res;
}

static inline vec4 Util_AbsVec4(vec4 vector)
{
   vec4 res = { 0 };

   Util_AbsVec_N(vector.arr, res.arr, M_VEC4_LEN);
   return res;
}

static inline vec4 Util_AddVec4(vec4 a, vec4 b)
{
   vec4 res = { 0 };

   Util_AddVec_N(a.arr, b.arr, res.arr, M_VEC4_LEN);
   return res;
}

static inline vec4 Util_SubVec4(vec4 a, vec4 b)
{
   vec4 res = { 0 };

   Util_SubVec_N(a.arr, b.arr, res.arr, M_VEC4_LEN);
   return res;
}

static inline vec4 Util_MulVec4(vec4 a, vec4 b)
{
   vec4 res = { 0 };

   Util_MulVec_N(a.arr, b.arr, res.arr, M_VEC4_LEN);
   return res;
}

static inline vec4 Util_ScaleVec4(vec4 vector, f32 scalar)
{
   vec4 res = { 0 };

   Util_MulVec_N_Scalar(vector.arr, scalar, res.arr, M_VEC4_LEN);
   return res;
}

static inline vec4 Util_NormalizeVec4(vec4 vector)
{
   vec4 res = { 0 };

   Util_NormalizedVec_N(vector.arr, res.arr, M_VEC4_LEN);
   return res;
}

static inline f32 Util_MagVec4(vec4 vector)
{
   f32 res = 0.0f;

   Util_MagVec_N(vector.arr, &res, M_VEC4_LEN);
   return res;
}

static inline f32 Util_DotVec4(vec4 a, vec4 b)
{
   f32 res = 0.0f;

   Util_DotVec_N(a.arr, b.arr, &res, M_VEC4_LEN);
   return res;
}

static inline vec3 Util_MulMat4Vec4(mat4x4 matrix, vec4 vector)
{
   vec3 res = { 0 };

   Util_MulMat_NxN_Vec_N(matrix.arr, vector.arr, res.arr, M_VEC4_LEN);
   return res;
}

#endif
