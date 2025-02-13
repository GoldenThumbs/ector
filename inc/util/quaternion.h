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
      Util_CrossVec3(a.xyz, b.xyz)
   );
   res.w = a.w * b.w - Util_DotVec3(a.xyz, b.xyz);
   return res;
}

#endif
