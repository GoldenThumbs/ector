#include "util/types.h"
#include "util/array.h"
#include "util/math.h"
#include "util/extra_types.h"
#include "util/vec3.h"
#include "util/vec4.h"

#include "physics.h"
#include "physics/internal.h"

#include <float.h>

void PHYS_RunEPA(PhysicsWorld* world, phys_Manifold* manifold)
{
   const f32 epa_epsilon = 0.005f;

   SET_ARRAY_LENGTH(world->epa_faces, 0);

   u32 closest_face_idx = 0;
   if (!PHYS_InitPolytope(world, &closest_face_idx))
      return;

   phys_EpaFace closest_face = { 0 };
   f32 closest_dist = FLT_MAX;

   phys_Support support = { 0 };
   for (u32 epa_iter=0; epa_iter<2048; ++epa_iter)
   {
      closest_face = world->epa_faces[closest_face_idx];
      support = PHYS_GetSupport(manifold, closest_face.normal.xyz);

      f32 support_dist = Util_DotVec3(support.world_point, closest_face.normal.xyz);
      
      bool close_enough = (M_ABS(support_dist - closest_face.normal.w) < epa_epsilon);
      
      closest_dist = closest_face.normal.w;
      if (epa_iter >= 2047)
         break;
      if (close_enough)
         break;

      closest_face_idx = PHYS_GrowPolytope(world, support);
   }

   vec3 barycentric = PHYS_BarycentricCoords(world, closest_face, VEC3(0, 0, 0));

   phys_Contact contact = { 0 };
   contact.object_point[0] = PHYS_ProjectLocalPoint(world, closest_face, barycentric, 0);
   contact.object_point[1] = PHYS_ProjectLocalPoint(world, closest_face, barycentric, 1);
   contact.world_point[0] = PHYS_ObjectPointInWorld(manifold->body[0], contact.object_point[0]);
   contact.world_point[1] = PHYS_ObjectPointInWorld(manifold->body[1], contact.object_point[1]);
   contact.basis = PHYS_MakeBasis(world, closest_face.normal.xyz);
   contact.depth = closest_face.normal.w;

   PHYS_AddContact(manifold, contact);
}

bool PHYS_InitPolytope(PhysicsWorld *world, u32* closest_face_idx)
{
   if (Util_ArrayLength(world->supports) < 4)
      false;

   f32 closest_dist = FLT_MAX;
   for (u32 i=0; i<4; i++)
   {
      u32 idx_a = i / 3;
      u32 idx_b = i + 1 - idx_a;
      u32 idx_c = idx_a + (idx_b % 3) + 1;

      phys_EpaFace face = PHYS_MakeFace(world, idx_a, idx_b, idx_c);
      if (face.normal.w < closest_dist)
      {
         *closest_face_idx = Util_ArrayLength(world->epa_faces);
         closest_dist = face.normal.w;
      }

      ADD_BACK_ARRAY(world->epa_faces, face);
   }

   return true;
}

phys_EpaFace PHYS_MakeFace(PhysicsWorld* world, u32 idx_a, u32 idx_b, u32 idx_c)
{
   vec3 a = world->supports[idx_a].world_point;
   vec3 b = world->supports[idx_b].world_point;
   vec3 c = world->supports[idx_c].world_point;

   vec3 b_a = Util_SubVec3(b, a);
   vec3 c_a = Util_SubVec3(c, a);

   vec3 centroid = Util_ScaleVec3(Util_AddVec3(a, Util_AddVec3(b, c)), 0.3333);

   vec3 normal = Util_NormalizeVec3(Util_Cross(b_a, c_a));
   f32 dist = Util_DotVec3(centroid, normal);

   return (phys_EpaFace){
      { idx_a, idx_b, idx_c },
      Util_VecF32Vec4(normal, dist)
   };
}

u32 PHYS_GrowPolytope(PhysicsWorld* world, phys_Support support)
{
   u32 idx = Util_ArrayLength(world->supports);
   PHYS_AddSupport(world, support, idx);

   PHYS_RemoveFaces(world, idx);

   u32 face_count = Util_ArrayLength(world->epa_faces);

   u32 min_idx = PHYS_RepairFaces(world, idx);
   f32 min_dist = world->epa_faces[min_idx].normal.w;

   for (u32 i=0; i<face_count; i++)
   {
      phys_EpaFace face = world->epa_faces[i];

      if (face.normal.w < min_dist)
      {
         min_idx = i;
         min_dist = face.normal.w;
      }
   }

   return min_idx;
}

vec3 PHYS_BarycentricCoords(PhysicsWorld* world, phys_EpaFace face, vec3 point)
{
   vec3 a = world->supports[face.idx[0]].world_point;
   vec3 b = world->supports[face.idx[1]].world_point;
   vec3 c = world->supports[face.idx[2]].world_point;

   return Util_BarycentricCoordinates(point, a, b, c);
}

vec3 PHYS_ProjectLocalPoint(PhysicsWorld* world, phys_EpaFace face, vec3 barycentric, u32 body_idx)
{
   body_idx = M_MIN(body_idx, 1);

   vec3 a = world->supports[face.idx[0]].object_point[body_idx];
   vec3 b = world->supports[face.idx[1]].object_point[body_idx];
   vec3 c = world->supports[face.idx[2]].object_point[body_idx];

   a = Util_ScaleVec3(a, barycentric.x);
   b = Util_ScaleVec3(b, barycentric.y);
   c = Util_ScaleVec3(c, barycentric.z);

   return Util_AddVec3(a, Util_AddVec3(b, c));
}

mat3x3 PHYS_MakeBasis(PhysicsWorld* world, vec3 normal)
{
   const f32 sqrt_third = 0.57735f; // sqrt(1 / 3)

   vec3 t = (M_ABS(normal.x) > sqrt_third) ?
      Util_NormalizeVec3(VEC3(normal.y,-normal.x, 0)) :
      Util_NormalizeVec3(VEC3(0, normal.z,-normal.y));
   
   vec3 b = Util_Cross(normal, t);

   return (mat3x3){ .v = { t, b, normal } };
}

void PHYS_RemoveFaces(PhysicsWorld* world, u32 point_idx)
{
   PHYS_ClearEdges(world);

   vec3 p = world->supports[point_idx].world_point;

   u32 face_count = Util_ArrayLength(world->epa_faces);
   for (u32 i=0; i<face_count;)
   {
      phys_EpaFace face = world->epa_faces[i];
      vec3 a = world->supports[face.idx[0]].world_point;

      if (PHYS_IsInDirection(Util_SubVec3(p, a), face.normal.xyz))
      {
         PHYS_AddEdge(world, (phys_EpaEdge){ .idx = { face.idx[0], face.idx[1] } });
         PHYS_AddEdge(world, (phys_EpaEdge){ .idx = { face.idx[1], face.idx[2] } });
         PHYS_AddEdge(world, (phys_EpaEdge){ .idx = { face.idx[2], face.idx[0] } });

         world->epa_faces[i] = world->epa_faces[--face_count];
      } else
         i++;
   }

   SET_ARRAY_LENGTH(world->epa_faces, face_count);
}

u32 PHYS_RepairFaces(PhysicsWorld* world, u32 point_idx)
{
   u32 min_idx = 0;
   f32 min_dist = FLT_MAX;

   u32 edge_count = Util_ArrayLength(world->epa_edges);
   for (u32 i=0; i<edge_count; i++)
   {
      phys_EpaEdge edge = world->epa_edges[i];

      phys_EpaFace face = PHYS_MakeFace(world, point_idx, edge.idx[1], edge.idx[0]);

      if (face.normal.w < 0)
      {
         face.idx[1] = edge.idx[0];
         face.idx[2] = edge.idx[1];
         face.normal = Util_ScaleVec4(face.normal,-1);
      }

      if (face.normal.w < min_dist)
      {
         min_idx = Util_ArrayLength(world->epa_faces);
         min_dist = face.normal.w;
      }

      ADD_BACK_ARRAY(world->epa_faces, face);
   }

   return min_idx;
}

void PHYS_AddEdge(PhysicsWorld* world, phys_EpaEdge edge)
{
   phys_EpaEdge edge_rev = { .idx = { edge.idx[1], edge.idx[0] } };

   u32 edge_count = Util_ArrayLength(world->epa_edges);
   for (u32 i=0; i<edge_count; i++)
   {
      u64 edge_id = world->epa_edges[i].edge_id;
      if (edge_rev.edge_id == edge_id)
      {
         PHYS_RemoveEdge(world, i);
         return;
      }
   }

   ADD_BACK_ARRAY(world->epa_edges, edge);
}

void PHYS_RemoveEdge(PhysicsWorld* world, u32 index)
{
   u32 edge_count = Util_ArrayLength(world->epa_edges);
   world->epa_edges[M_MIN(index, edge_count - 1)] = POP_BACK_ARRAY(world->epa_edges);
}

void PHYS_ClearEdges(PhysicsWorld* world)
{
   SET_ARRAY_LENGTH(world->epa_edges, 0);
}
