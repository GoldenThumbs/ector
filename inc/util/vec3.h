#ifndef UTIL_VEC3_H
#define UTIL_VEC3_H

#include "util/types.h"
#include "util/math.h"

#define M_VEC3_LEN 3

static inline vec3 Util_FillVec3(f32 scalar)
{
   return VEC3(scalar, scalar, scalar);
}

static inline f32 Util_MinElmVec3(vec3 vector)
{
   return M_MIN(vector.z, M_MIN(vector.y, vector.x));
}

static inline f32 Util_MaxElmVec3(vec3 vector)
{
   return M_MAX(vector.z, M_MAX(vector.y, vector.x));
}

static inline vec3 Util_MinVec3(vec3 a, vec3 b)
{
   vec3 res = { 0 };

   Util_MinVec_N(a.arr, b.arr, res.arr, M_VEC3_LEN);
   return res;
}

static inline vec3 Util_MaxVec3(vec3 a, vec3 b)
{
   vec3 res = { 0 };

   Util_MaxVec_N(a.arr, b.arr, res.arr, M_VEC3_LEN);
   return res;
}

static inline vec3 Util_AbsVec3(vec3 vector)
{
   vec3 res = { 0 };

   Util_AbsVec_N(vector.arr, res.arr, M_VEC3_LEN);
   return res;
}

static inline vec3 Util_AddVec3(vec3 a, vec3 b)
{
   vec3 res = { 0 };

   Util_AddVec_N(a.arr, b.arr, res.arr, M_VEC3_LEN);
   return res;
}

static inline vec3 Util_SubVec3(vec3 a, vec3 b)
{
   vec3 res = { 0 };

   Util_SubVec_N(a.arr, b.arr, res.arr, M_VEC3_LEN);
   return res;
}

static inline vec3 Util_MulVec3(vec3 a, vec3 b)
{
   vec3 res = { 0 };

   Util_MulVec_N(a.arr, b.arr, res.arr, M_VEC3_LEN);
   return res;
}

static inline vec3 Util_ScaleVec3(vec3 vector, f32 scalar)
{
   vec3 res = { 0 };

   Util_MulVec_N_Scalar(vector.arr, scalar, res.arr, M_VEC3_LEN);
   return res;
}

static inline vec3 Util_NormalizeVec3(vec3 vector)
{
   vec3 res = { 0 };

   Util_NormalizedVec_N(vector.arr, res.arr, M_VEC3_LEN);
   return res;
}

static inline f32 Util_MagVec3(vec3 vector)
{
   f32 res = 0.0f;

   Util_MagVec_N(vector.arr, &res, M_VEC3_LEN);
   return res;
}

static inline f32 Util_DotVec3(vec3 a, vec3 b)
{
   f32 res = 0.0f;

   Util_DotVec_N(a.arr, b.arr, &res, M_VEC3_LEN);
   return res;
}

static inline vec3 Util_CrossVec3(vec3 a, vec3 b)
{
   return VEC3(
      a.y*b.z - b.y*a.z,
      a.z*b.x - b.z*a.x,
      a.x*b.y - b.x*a.y
   );
}

static inline vec3 Util_MulMat3Vec3(mat3x3 matrix, vec3 vector)
{
   vec3 res = { 0 };

   Util_MulMat_NxN_Vec_N(matrix.arr, vector.arr, res.arr, M_VEC3_LEN);
   return res;
}

#endif
