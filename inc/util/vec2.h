#ifndef UTIL_VEC2_H
#define UTIL_VEC2_H

#include "util/types.h"
#include "util/math.h"

#define M_VEC2_LEN 2

static inline vec2 Util_FillVec2(f32 scalar)
{
   return VEC2(scalar, scalar);
}

static inline f32 Util_MinElmVec2(vec2 vector)
{
   return M_MIN(vector.y, vector.x);
}

static inline f32 Util_MaxElmVec2(vec2 vector)
{
   return M_MAX(vector.y, vector.x);
}

static inline vec2 Util_MinVec2(vec2 a, vec2 b)
{
   vec2 res = { 0 };

   Util_MinVec_N(a.arr, b.arr, res.arr, M_VEC2_LEN);
   return res;
}

static inline vec2 Util_MaxVec2(vec2 a, vec2 b)
{
   vec2 res = { 0 };

   Util_MaxVec_N(a.arr, b.arr, res.arr, M_VEC2_LEN);
   return res;
}

static inline vec2 Util_AbsVec2(vec2 vector)
{
   vec2 res = { 0 };

   Util_AbsVec_N(vector.arr, res.arr, M_VEC2_LEN);
   return res;
}

static inline vec2 Util_AddVec2(vec2 a, vec2 b)
{
   vec2 res = { 0 };

   Util_AddVec_N(a.arr, b.arr, res.arr, M_VEC2_LEN);
   return res;
}

static inline vec2 Util_SubVec2(vec2 a, vec2 b)
{
   vec2 res = { 0 };

   Util_SubVec_N(a.arr, b.arr, res.arr, M_VEC2_LEN);
   return res;
}

static inline vec2 Util_MulVec2(vec2 a, vec2 b)
{
   vec2 res = { 0 };

   Util_MulVec_N(a.arr, b.arr, res.arr, M_VEC2_LEN);
   return res;
}

static inline vec2 Util_ScaleVec2(vec2 vector, f32 scalar)
{
   vec2 res = { 0 };

   Util_MulVec_N_Scalar(vector.arr, scalar, res.arr, M_VEC2_LEN);
   return res;
}

static inline vec2 Util_NormalizeVec2(vec2 vector)
{
   vec2 res = { 0 };

   Util_NormalizedVec_N(vector.arr, res.arr, M_VEC2_LEN);
   return res;
}

static inline f32 Util_MagVec2(vec2 vector)
{
   f32 res = 0.0f;

   Util_MagVec_N(vector.arr, &res, M_VEC2_LEN);
   return res;
}

static inline f32 Util_DotVec2(vec2 a, vec2 b)
{
   f32 res = 0.0f;

   Util_DotVec_N(a.arr, b.arr, &res, M_VEC2_LEN);
   return res;
}

#endif
