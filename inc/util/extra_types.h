#ifndef ECT_EXTRA_TYPES_H
#define ECT_EXTRA_TYPES_H

#include "util/types.h"
#include "util/vec3.h"

typedef struct BBox_t
{
   vec3 center;
   vec3 extents;
} BBox;

static inline u64 Util_MakeID(u32 a, u32 b)
{
   union {
      u32 id;
      u8 words[4];
   } id_a, id_b;
   const u64 m = 37;

   id_a.id = a;
   id_b.id = b;

   u32 res = 0;
   res = ((u64)id_a.words[0] + (u64)id_b.words[0]);
   res = ((u64)id_a.words[1] + (u64)id_b.words[1]) + (m * res);
   res = ((u64)id_a.words[2] + (u64)id_b.words[2]) + (m * res);
   res = ((u64)id_a.words[3] + (u64)id_b.words[3]) + (m * res);

   return res;
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

#endif
