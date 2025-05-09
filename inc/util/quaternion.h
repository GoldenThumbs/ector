#ifndef UTIL_QUATERNION_H
#define UTIL_QUATERNION_H

#include "util/types.h"
#include "util/math.h"
#include "util/vec3.h"

static inline quat Util_IdentityQuat(void)
{
   return QUAT(0, 0, 0, 1);
}

static inline quat Util_MakeQuat(vec3 axis, f32 angle)
{
   f32 a = angle * 0.5f;
   f32 b = M_SIN(a);
   f32 c = M_COS(a);

   quat res = { 0 };
   res.xyz = Util_ScaleVec3(axis, b);
   res.w = c;
   return res;
}

static inline quat Util_MulQuat(quat a, quat b)
{
   quat res = { 0 };
   res.xyz = Util_AddVec3(
      Util_AddVec3(Util_ScaleVec3(b.xyz, a.w), Util_ScaleVec3(a.xyz, b.w)),
      Util_Cross(a.xyz, b.xyz)
   );
   res.w = a.w * b.w - Util_DotVec3(a.xyz, b.xyz);
   return res;
}

// TODO: too lazy to properly write this out so this is perfectly functional but not optimal
static inline quat Util_MakeQuatEuler(vec3 euler)
{
   quat yaw = Util_MakeQuat(VEC3(0, 1, 0), euler.y);
   quat pitch = Util_MakeQuat(VEC3(1, 0, 0), euler.x);
   quat roll = Util_MakeQuat(VEC3(0, 0, 1), euler.z);
   return Util_MulQuat(roll, Util_MulQuat(pitch, yaw));
}

static inline quat Util_MakeQuatMat3(mat3x3 matrix)
{
   f32 t = matrix.m[0][0] + matrix.m[1][1] + matrix.m[2][2];
   if (t > 0)
   {
      f32 r = M_SQRT(1.0f + t);
      f32 s = M_RCP(2.0f * r, M_FLOAT_FUZZ);
      return QUAT(
         (matrix.m[2][1] - matrix.m[1][2]) * s,
         (matrix.m[0][2] - matrix.m[2][0]) * s,
         (matrix.m[1][0] - matrix.m[0][1]) * s,
         r * 0.5f
      );
   } else {
      f32 r = M_SQRT(1.0f + matrix.m[0][0] - matrix.m[1][1] - matrix.m[2][2]);
      f32 s = M_RCP(2.0f * r, M_FLOAT_FUZZ);
      return QUAT(
         r * 0.5f,
         (matrix.m[0][1] - matrix.m[1][0]) * s,
         (matrix.m[2][0] - matrix.m[0][2]) * s,
         (matrix.m[2][1] - matrix.m[1][2]) * s
      );
   }
}

#endif
