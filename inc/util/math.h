#ifndef UTIL_MATH_H
#define UTIL_MATH_H

#include "util/types.h"

#include <stdlib.h>
#include <string.h>

#ifndef USE_CUSTOM_CONSTS
   #define M_PI32 3.141592f
   #define M_PI64 3.141592653589793
   #define M_INVPI32 (1.0f / M_PI32)
   #define M_INVPI64 (1.0 / M_PI64)
#endif

#define M_FLOAT_FUZZ 0.0001f

#define M_ABS(x) (((x) <= 0) ? (x) : -(x))
#define M_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define M_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define M_RCP(x, eps) ((M_ABS(x) > (eps)) ? (1.0f / (x)) : 0.0f)

#define M_TURN32 (M_PI32 * 0.01f)
#define M_TURN64 (M_PI64 * 0.01)
#define M_TO_TURN32 (100.0f * M_INVPI32)
#define M_TO_TURN64 (100.0 * M_INVPI64)

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

static inline void Util_AddVec_N(f32 a[], f32 b[], f32* res_vec, const u32 N)
{
   for (u32 i=0; i<N; i++)
      res_vec[i] = a[i] + b[i];
}

static inline void Util_SubVec_N(f32 a[], f32 b[], f32* res_vec, const u32 N)
{
   for (u32 i=0; i<N; i++)
      res_vec[i] = a[i] - b[i];
}

static inline void Util_MulVec_N(f32 a[], f32 b[], f32* res_vec, const u32 N)
{
   for (u32 i=0; i<N; i++)
      res_vec[i] = a[i] * b[i];
}

static inline void Util_DivVec_N(f32 a[], f32 b[], f32* res_vec, const u32 N)
{
   for (u32 i=0; i<N; i++)
      res_vec[i] = a[i] * M_RCP(b[i], M_FLOAT_FUZZ);
}

static inline void Util_MulVec_N_Scalar(f32 vector[], f32 scalar, f32* res_vec, const u32 N)
{
   for (u32 i=0; i<N; i++)
      res_vec[i] = vector[i] * scalar;
}

static inline void Util_DotVec_N(f32 a[], f32 b[], f32* res_scalar, const u32 N)
{
   for (u32 i=0; i<N; i++)
      res_scalar[0] += a[i] * b[i];
}

static inline void Util_MagVec_N(f32 vector[], f32* res_scalar, const u32 N)
{
   for (u32 i=0; i<N; i++)
      res_scalar[0] += vector[i] * vector[i];

   res_scalar[0] = M_SQRT(res_scalar[0]);
}

static inline void Util_NormalizedVec_N(f32 vector[], f32* res_vec, const u32 N)
{
   f32 mag = 0.0f;
   Util_MagVec_N(vector, &mag, N);

   Util_MulVec_N_Scalar(vector, M_RCP(mag, M_FLOAT_FUZZ), res_vec, N);
}

static inline void Util_MinVec_N(f32 a[], f32 b[], f32* res_vec, const u32 N)
{
   for (u32 i=0; i<N; i++)
      res_vec[i] = M_MIN(a[i], b[i]);
}

static inline void Util_MaxVec_N(f32 a[], f32 b[], f32* res_vec, const u32 N)
{
   for (u32 i=0; i<N; i++)
      res_vec[i] = M_MAX(a[i], b[i]);
}

static inline void Util_AbsVec_N(f32 vector[], f32* res_vec, const u32 N)
{
   for (u32 i=0; i<N; i++)
      res_vec[i] = M_ABS(vector[i]);
}

static inline void Util_TransposeMat_NxN(f32 matrix[], f32* res_mat, const u32 N)
{
   for (u32 ROW=0; ROW<N; ROW++)
   for (u32 COL=0; COL<N; COL++)
   {
      res_mat[ROW * N + COL] = matrix[COL * N + ROW];
   }
}

static inline void Util_MulMat_NxN(f32 a[], f32 b[], f32* res_mat, const u32 N)
{
   for (u32 ROW=0; ROW<N; ROW++)
   for (u32 COL=0; COL<N; COL++)
   for (u32 k=0; k<N; k++)
   {
      res_mat[COL * N + ROW] += a[k * N + ROW] * b[COL * N + k];
   }
}

static inline void Util_MulMat_NxN_Vec_N(f32 matrix[], f32 vector[], f32* res_vec, const u32 N)
{
   for (u32 ROW=0; ROW<N; ROW++)
   for (u32 COL=0; COL<N; COL++)
   {
      res_vec[ROW] += matrix[COL * N + ROW] * vector[COL];
   }
}

static inline void Util_InverseDiagonalMat_NxN(f32 matrix[], f32* res_mat, const u32 N)
{
   memcpy(res_mat, matrix, (uS)(N*N) * sizeof(f32));
   for (u32 i=0; i<N; i++)
      res_mat[i * N + i] = M_RCP(matrix[i * N + i], M_FLOAT_FUZZ);
}

#endif
