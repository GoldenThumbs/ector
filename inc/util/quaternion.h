#ifndef UTIL_QUATERNION_H
#define UTIL_QUATERNION_H

#include "util/types.h"
#include "util/math.h"
#include "util/vec3.h"
#include "util/vec4.h"

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
      Util_CrossVec3(a.xyz, b.xyz)
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

static inline quat Util_MakeQuatLookingAt(vec3 origin, vec3 target, vec3 front, vec3 up)
{
   vec3 pointing_dir = Util_NormalizeVec3(Util_SubVec3(target, origin));
   vec3 tan_dir = Util_NormalizeVec3(Util_SubVec3(pointing_dir, Util_ScaleVec3(up, Util_DotVec3(pointing_dir, up))));

   quat yaw_q = Util_IdentityQuat();
   quat pitch_q = Util_IdentityQuat();

   vec3 yaw_axis = Util_NormalizeVec3(Util_CrossVec3(front, tan_dir));

   if (Util_DotVec3(yaw_axis, yaw_axis) > M_FLOAT_FUZZ)
   {
      f32 cos_angle = Util_DotVec3(front, tan_dir);
      f32 angle = -M_ACOS(cos_angle);

      yaw_q = Util_MakeQuat(yaw_axis, angle);
   } else {
      yaw_axis = Util_ScaleVec3(up, M_SIGN(Util_DotVec3(tan_dir, front)));
      yaw_q = Util_VecF32Vec4(yaw_axis, 0);
   }
   
   if (M_ABS(Util_DotVec3(pointing_dir, up)) > M_FLOAT_FUZZ)
   {
      vec3 pitch_axis = Util_NormalizeVec3(Util_CrossVec3(tan_dir, up));
      if (Util_DotVec3(pitch_axis, pitch_axis) > M_FLOAT_FUZZ)
      {
         f32 cos_angle = Util_DotVec3(pointing_dir, tan_dir);
         f32 angle = M_ACOS(cos_angle);

         pitch_q = Util_MakeQuat(pitch_axis, angle);
      }
   }

   return Util_MulQuat(yaw_q, pitch_q);
}

static inline quat Util_SphericalLerp(quat a, quat b, f32 fac)
{
   f32 cos_angle = Util_DotVec4(a, b);
   if (M_ABS(cos_angle) >= 1.0f)
      return a;

   if (cos_angle < 0)
   {
      cos_angle = -cos_angle;
      a = Util_ScaleVec4(a, -1);
   }

   f32 sin_angle = M_SQRT(1.0f - cos_angle * cos_angle);
   if (M_ABS(sin_angle) < M_FLOAT_FUZZ)
   {
      quat q1 = Util_ScaleVec4(a, (1.0f - fac));
      quat q2 = Util_ScaleVec4(b, fac);
      return Util_AddVec4(q1, q2);
   }

   f32 angle = M_ACOS(cos_angle);
   f32 denom = M_RCP(sin_angle, M_FLOAT_FUZZ);
   quat q1 = Util_ScaleVec4(a, M_SIN(angle * (1.0f - fac)));
   quat q2 = Util_ScaleVec4(b, M_SIN(angle * fac));
   return Util_ScaleVec4(Util_AddVec4(q1, q2), denom);
}

#endif
