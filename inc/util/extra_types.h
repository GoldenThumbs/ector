#ifndef ECT_EXTRA_TYPES_H
#define ECT_EXTRA_TYPES_H

#include "util/types.h"
#include "util/math.h"
#include "util/vec3.h"
#include "util/matrix.h"

#define READ_HEAD(PTR, TYPE) *((TYPE*)Util_ReadThenMove(&(PTR), sizeof(TYPE)))

typedef struct BBox_t
{
   vec3 center;
   vec3 extents;
} BBox;

typedef struct Transform3D_t
{
   vec3 origin;
   quat rotation;
   vec3 scale;
} Transform3D;

static inline color8 Util_MakeRGBE(vec3 hdr_color)
{
   i32 e = 0;
   f32 max_elm = Util_MaxElmVec3(hdr_color);
   if (max_elm < M_FLOAT_FUZZ)
      return (color8){ .hex = 0 };

   max_elm = frexpf(max_elm, &e) * 256.0f / max_elm;
   vec3 scaled = Util_ScaleVec3(hdr_color, max_elm);
   return (color8){
      (u8)scaled.r,
      (u8)scaled.g,
      (u8)scaled.b,
      (u8)(e + 128)
   };
}

static inline f32 Util_AreaBBox(BBox bbox)
{
   vec3 d = Util_ScaleVec3(bbox.extents, 2.0f);
   return 2.0f * (d.x*d.y + d.y*d.z + d.z*d.x);
}

static inline f32 Util_VolumeBBox(BBox bbox)
{
   vec3 size = Util_ScaleVec3(bbox.extents, 2.0f);
   return size.x * size.y * size.z;
}

static inline BBox Util_ResizeBBox(BBox bbox, mat3x3 rotation)
{
   bbox.extents = Util_AddVec3(Util_AddVec3(
      Util_AbsVec3(Util_ScaleVec3(rotation.v[0], bbox.extents.x)),
      Util_AbsVec3(Util_ScaleVec3(rotation.v[1], bbox.extents.y))),
      Util_AbsVec3(Util_ScaleVec3(rotation.v[2], bbox.extents.z))
   );
   return bbox;
}

static inline BBox Util_MinkowskiBBox(BBox a, BBox b)
{
   vec3 top_left = Util_SubVec3(Util_SubVec3(a.center, a.extents), Util_AddVec3(b.center, b.extents));
   vec3 extents = Util_ScaleVec3(Util_AddVec3(a.extents, b.extents), 0.5f);
   return (BBox){
      Util_AddVec3(top_left, extents),
      extents
   };
}

static inline BBox Util_UnionBBox(BBox a, BBox b)
{
   vec3 bound_min = Util_MinVec3(Util_SubVec3(a.center, a.extents), Util_SubVec3(b.center, b.extents));
   vec3 bound_max = Util_MaxVec3(Util_AddVec3(a.center, a.extents), Util_AddVec3(b.center, b.extents));
   vec3 center = Util_ScaleVec3(Util_AddVec3(bound_min, bound_max), 0.5f);
   vec3 extents = Util_SubVec3(bound_max, center);
   return (BBox){
      center,
      extents
   };
}

static inline bool Util_OverlapBBox(BBox a, BBox b)
{
   vec3 a_min = Util_SubVec3(a.center, a.extents);
   vec3 a_max = Util_AddVec3(a.center, a.extents);
   vec3 b_min = Util_SubVec3(b.center, b.extents);
   vec3 b_max = Util_AddVec3(b.center, b.extents);

   bool min_ab_max = (a_min.x <= b_max.x) && (a_min.y <= b_max.y) && (a_min.z <= b_max.z);
   bool max_ab_min = (a_max.x >= b_min.x) && (a_max.y >= b_min.y) && (a_max.z >= b_min.z);

   return min_ab_max && max_ab_min;
}

static inline bool Util_ContainsBBox(BBox a, BBox b)
{
   vec3 a_min = Util_SubVec3(a.center, a.extents);
   vec3 a_max = Util_AddVec3(a.center, a.extents);
   vec3 b_min = Util_SubVec3(b.center, b.extents);
   vec3 b_max = Util_AddVec3(b.center, b.extents);

   bool min_ab = (a_min.x < b_min.x) && (a_min.y < b_min.y) && (a_min.z < b_min.z);
   bool max_ab = (a_max.x > b_max.x) && (a_max.y > b_max.y) && (a_max.z > b_max.z);

   return min_ab && max_ab;
}

static inline bool Util_ContainsBBoxPoint(BBox bbox, vec3 point)
{
   vec3 diff = Util_AbsVec3(Util_SubVec3(bbox.center, point));
   diff = Util_SubVec3(diff, bbox.extents);

   return (diff.x <= 0) && (diff.y <= 0) && (diff.z <= 0);
}

static inline mat4x4 Util_TransformationMatrix(Transform3D transform)
{
   mat4x4 t = Util_TranslationMatrix(transform.origin);
   mat4x4 r = Util_Mat3ToMat4(Util_QuatToMat3(transform.rotation));
   mat4x4 s = Util_ScalingMatrix(transform.scale);

   return Util_MulMat4(s, Util_MulMat4(r, t));
}

static inline u8* Util_ReadThenMove(void** read_head, uS read_size)
{
   u8* cached = (u8*)(*read_head);
   *read_head = (void*)(cached + read_size);
   return cached;
}

#endif
