#include "util/types.h"
#include "util/array.h"
#include "util/math.h"
#include "util/extra_types.h"
#include "util/vec3.h"
#include "util/vec4.h"
#include "util/quaternion.h"
#include "util/matrix.h"
#include "util/resource.h"

#include "physics.h"
#include "physics/internal.h"

#include <stdlib.h>
#include <stdio.h>
#include <float.h>

PhysicsWorld* Physics_Init(void)
{
   PhysicsWorld* world = malloc(sizeof(PhysicsWorld));
   world->bodies = NEW_ARRAY_N(phys_PhysicsBody, 8);
   world->manifolds = NEW_ARRAY_N(phys_Manifold, 8);
   world->manifold_root = -1;

   world->supports = NEW_ARRAY_N(phys_Support, 16);
   world->epa_faces = NEW_ARRAY_N(phys_EpaFace, 16);
   world->epa_edges = NEW_ARRAY_N(phys_EpaEdge, 16);

   world->debug.points = NEW_ARRAY_N(phys_DebugVertex, 512);
   world->debug.lines = NEW_ARRAY_N(phys_DebugVertex, 512);
   world->debug.faces = NEW_ARRAY_N(phys_DebugVertex, 512);

   return world;
}

void Physics_Free(PhysicsWorld* world)
{
   FREE_ARRAY(world->debug.points);
   FREE_ARRAY(world->debug.lines);
   FREE_ARRAY(world->debug.faces);
   FREE_ARRAY(world->supports);
   FREE_ARRAY(world->epa_faces);
   FREE_ARRAY(world->epa_edges);
   FREE_ARRAY(world->manifolds);
   FREE_ARRAY(world->bodies);

   free(world);
}

void Physics_Update(PhysicsWorld* world, f32 delta)
{
   SET_ARRAY_LENGTH(world->debug.points, 0);
   SET_ARRAY_LENGTH(world->debug.lines, 0);
   SET_ARRAY_LENGTH(world->debug.faces, 0);

   phys_DebugVertex o_v = { Util_FillVec3(0), 12 };
   ADD_BACK_ARRAY(world->debug.points, o_v);

   u32 body_count = Util_ArrayLength(world->bodies);
   for (u32 i=0; i<body_count; i++)
   {
      phys_PhysicsBody* body = world->bodies + (uS)i;
      PHYS_BodyUpdateRotation(body);

      f32 wx = body->aabb.extents.x + PHYS_BIAS;
      f32 wy = body->aabb.extents.y + PHYS_BIAS;
      f32 wz = body->aabb.extents.z + PHYS_BIAS;
      phys_DebugVertex v[8] = {
         { Util_AddVec3(body->aabb.center, VEC3( wx, wy, wz)), 0 },
         { Util_AddVec3(body->aabb.center, VEC3(-wx, wy, wz)), 0 },
         { Util_AddVec3(body->aabb.center, VEC3(-wx, wy,-wz)), 0 },
         { Util_AddVec3(body->aabb.center, VEC3( wx, wy,-wz)), 0 },
         { Util_AddVec3(body->aabb.center, VEC3( wx,-wy, wz)), 0 },
         { Util_AddVec3(body->aabb.center, VEC3(-wx,-wy, wz)), 0 },
         { Util_AddVec3(body->aabb.center, VEC3(-wx,-wy,-wz)), 0 },
         { Util_AddVec3(body->aabb.center, VEC3( wx,-wy,-wz)), 0 }
      };

      ADD_BACK_ARRAY(world->debug.lines, v[0]);
      ADD_BACK_ARRAY(world->debug.lines, v[1]);

      ADD_BACK_ARRAY(world->debug.lines, v[1]);
      ADD_BACK_ARRAY(world->debug.lines, v[2]);

      ADD_BACK_ARRAY(world->debug.lines, v[2]);
      ADD_BACK_ARRAY(world->debug.lines, v[3]);

      ADD_BACK_ARRAY(world->debug.lines, v[3]);
      ADD_BACK_ARRAY(world->debug.lines, v[0]);

      ADD_BACK_ARRAY(world->debug.lines, v[4]);
      ADD_BACK_ARRAY(world->debug.lines, v[5]);

      ADD_BACK_ARRAY(world->debug.lines, v[5]);
      ADD_BACK_ARRAY(world->debug.lines, v[6]);

      ADD_BACK_ARRAY(world->debug.lines, v[6]);
      ADD_BACK_ARRAY(world->debug.lines, v[7]);

      ADD_BACK_ARRAY(world->debug.lines, v[7]);
      ADD_BACK_ARRAY(world->debug.lines, v[4]);

      ADD_BACK_ARRAY(world->debug.lines, v[0]);
      ADD_BACK_ARRAY(world->debug.lines, v[4]);

      ADD_BACK_ARRAY(world->debug.lines, v[1]);
      ADD_BACK_ARRAY(world->debug.lines, v[5]);

      ADD_BACK_ARRAY(world->debug.lines, v[2]);
      ADD_BACK_ARRAY(world->debug.lines, v[6]);

      ADD_BACK_ARRAY(world->debug.lines, v[3]);
      ADD_BACK_ARRAY(world->debug.lines, v[7]);

      f32 ox = body->bounds.extents.x;
      f32 oy = body->bounds.extents.y;
      f32 oz = body->bounds.extents.z;
      phys_DebugVertex c[8] = {
         { PHYS_ObjectPointInWorld(body, VEC3( ox, oy, oz)), 2 },
         { PHYS_ObjectPointInWorld(body, VEC3(-ox, oy, oz)), 2 },
         { PHYS_ObjectPointInWorld(body, VEC3(-ox, oy,-oz)), 2 },
         { PHYS_ObjectPointInWorld(body, VEC3( ox, oy,-oz)), 2 },
         { PHYS_ObjectPointInWorld(body, VEC3( ox,-oy, oz)), 2 },
         { PHYS_ObjectPointInWorld(body, VEC3(-ox,-oy, oz)), 2 },
         { PHYS_ObjectPointInWorld(body, VEC3(-ox,-oy,-oz)), 2 },
         { PHYS_ObjectPointInWorld(body, VEC3( ox,-oy,-oz)), 2 }
      };

      ADD_BACK_ARRAY(world->debug.lines, c[0]);
      ADD_BACK_ARRAY(world->debug.lines, c[1]);

      ADD_BACK_ARRAY(world->debug.lines, c[1]);
      ADD_BACK_ARRAY(world->debug.lines, c[2]);

      ADD_BACK_ARRAY(world->debug.lines, c[2]);
      ADD_BACK_ARRAY(world->debug.lines, c[3]);

      ADD_BACK_ARRAY(world->debug.lines, c[3]);
      ADD_BACK_ARRAY(world->debug.lines, c[0]);

      ADD_BACK_ARRAY(world->debug.lines, c[4]);
      ADD_BACK_ARRAY(world->debug.lines, c[5]);

      ADD_BACK_ARRAY(world->debug.lines, c[5]);
      ADD_BACK_ARRAY(world->debug.lines, c[6]);

      ADD_BACK_ARRAY(world->debug.lines, c[6]);
      ADD_BACK_ARRAY(world->debug.lines, c[7]);

      ADD_BACK_ARRAY(world->debug.lines, c[7]);
      ADD_BACK_ARRAY(world->debug.lines, c[4]);

      ADD_BACK_ARRAY(world->debug.lines, c[0]);
      ADD_BACK_ARRAY(world->debug.lines, c[4]);

      ADD_BACK_ARRAY(world->debug.lines, c[1]);
      ADD_BACK_ARRAY(world->debug.lines, c[5]);

      ADD_BACK_ARRAY(world->debug.lines, c[2]);
      ADD_BACK_ARRAY(world->debug.lines, c[6]);

      ADD_BACK_ARRAY(world->debug.lines, c[3]);
      ADD_BACK_ARRAY(world->debug.lines, c[7]);
   }

   PHYS_Broadphase(world);
   PHYS_Narrowphase(world);
}

PhysicsBody Physics_AddBody(PhysicsWorld* world, PhysicsBodyDesc* desc)
{
   phys_PhysicsBody body = { 0 };
   body.inv_mass = M_RCP(desc->mass, M_FLOAT_FUZZ);
   body.origin = desc->transform.origin;
   body.rotation = Util_QuatToMat3(desc->transform.rotation);
   body.bounds = desc->bounds;
   body.compare.ref = world->ref;

   PHYS_BodyUpdateRotation(&body);

   return Util_AddResource(&world->ref, REF(world->bodies), &body);
}

vec3 Physics_GetBodyOrigin(PhysicsWorld* world, PhysicsBody res_body)
{
   phys_PhysicsBody* body = &world->bodies[res_body.handle];
   if (body->compare.ref != res_body.ref)
      return VEC3(0, 0, 0);

   return body->origin;
}

void Physics_MoveBody(PhysicsWorld* world, PhysicsBody res_body, vec3 translate)
{
   phys_PhysicsBody* body = &world->bodies[res_body.handle];
   if (body->compare.ref != res_body.ref)
      return;

   body->origin = Util_AddVec3(body->origin, translate);
}

PhysicsDebugInfo Physics_GetDebugInfo(PhysicsWorld* world)
{
   PhysicsDebugInfo debug_info = { 0 };

   debug_info.point_count = Util_ArrayLength(world->debug.points);
   debug_info.line_count = Util_ArrayLength(world->debug.lines);
   debug_info.face_count = Util_ArrayLength(world->debug.faces);

   debug_info.points = world->debug.points;
   debug_info.lines = world->debug.lines;
   debug_info.faces = world->debug.faces;

   return debug_info;
}

bool PHYS_TestCollisonCoarse(phys_PhysicsBody* body_1, phys_PhysicsBody* body_2, phys_Manifold* result)
{
   const vec3 margin = VEC3(PHYS_BIAS, PHYS_BIAS, PHYS_BIAS);
   BBox aabb_1 = body_1->aabb;
   aabb_1.extents = Util_AddVec3(aabb_1.extents, margin);

   BBox aabb_2 = body_2->aabb;
   aabb_2.extents = Util_AddVec3(aabb_2.extents, margin);

   if (Util_OverlapBBox(body_1->aabb, body_2->aabb))
   {
      *result = (phys_Manifold){
         .body[0] = body_1,
         .body[1] = body_2,
         .aabb = Util_UnionBBox(body_1->aabb, body_2->aabb),
         .contact_count = 0,
         .contacts = { 0 },
         .next = -1
      };
      return true;
   }

   return false;
}

bool PHYS_TestGJK(PhysicsWorld* world, phys_Manifold* manifold)
{
   PHYS_ClearSupports(world);

   vec3 direction = VEC3(1, 0, 0);

   //direction = VEC3(
   //   (direction.x >= 0) ? manifold->aabb.extents.x :-manifold->aabb.extents.x,
   //   (direction.y >= 0) ? manifold->aabb.extents.y :-manifold->aabb.extents.y,
   //   (direction.z >= 0) ? manifold->aabb.extents.z :-manifold->aabb.extents.z
   //);

   phys_Support support = PHYS_GetSupport(manifold, direction);
   
   PHYS_AddSupport(world, support, 0);
   direction = Util_ScaleVec3(support.world_point,-1);

   while(true)
   {
      support = PHYS_GetSupport(manifold, direction);
      
      if (!PHYS_IsInDirection(support.world_point, direction))
         break;
   
      PHYS_AddSupport(world, support, 0);
      
      if (PHYS_EvolveGJK(world, manifold, &direction))
      {
         PHYS_RunEPA(world, manifold);
         return true;
      }
   }
   
   return false;
}

void PHYS_Broadphase(PhysicsWorld* world)
{
   u32 body_count = Util_ArrayLength(world->bodies);
   if (body_count < 2)
      return;
   
   SET_ARRAY_LENGTH(world->manifolds, 0);

   for (u32 i=0; i<body_count; i++)
   {
      phys_PhysicsBody* body_1 = world->bodies + (uS)i;
      for (u32 j=i+1; j<body_count; j++)
      {
         phys_PhysicsBody* body_2 = world->bodies + (uS)j;

         phys_Manifold manifold = { 0 };
         if (PHYS_TestCollisonCoarse(body_1, body_2, &manifold))
            ADD_BACK_ARRAY(world->manifolds, manifold);
      }
   }
}

void PHYS_Narrowphase(PhysicsWorld* world)
{
   u32 manifold_count = Util_ArrayLength(world->manifolds);
   for (u32 i=0; i<manifold_count; i++)
   {
      phys_Manifold* manifold = world->manifolds + (uS)i;

      if (PHYS_TestGJK(world, manifold))
      {
         printf("Collision Hit: %i\n", i);
      }
   }
}

void PHYS_BodyApplyForce(phys_PhysicsBody* body, vec3 force, vec3 point)
{
   body->total_force = Util_AddVec3(body->total_force, force);
   body->total_torque = Util_AddVec3(
      body->total_torque,
      Util_Cross(
         Util_SubVec3(point, body->aabb.center),
         force
      )
   );
}

void PHYS_BodyUpdateRotation(phys_PhysicsBody* body)
{
   quat q = Util_MakeQuatMat3(body->rotation);
   body->rotation = Util_QuatToMat3(Util_NormalizeVec4(q));
   body->inv_rotation = Util_TransposeMat3(body->rotation);

   body->inv_inertia_world = Util_MulMat3(
      body->rotation,
      Util_MulMat3(
         body->inv_inertia_object,
         body->inv_rotation
      )
   );

   body->aabb = Util_ResizeBBox(body->bounds, body->rotation);
   body->aabb.center = Util_AddVec3(body->origin, body->bounds.center);
}

vec3 PHYS_ObjectPointInWorld(phys_PhysicsBody* body, vec3 point)
{
   return Util_AddVec3(Util_MulMat3Vec3(body->rotation, point), body->origin);
}

vec3 PHYS_WorldPointInObject(phys_PhysicsBody* body, vec3 point)
{
   return Util_MulMat3Vec3(body->inv_rotation, Util_SubVec3(point, body->origin));
}

vec3 PHYS_ObjectVectorInWorld(phys_PhysicsBody* body, vec3 vector)
{
   return Util_MulMat3Vec3(body->rotation, vector);
}

vec3 PHYS_WorldVectorInObject(phys_PhysicsBody* body, vec3 vector)
{
   return Util_MulMat3Vec3(body->inv_rotation, vector);
}

mat3x3 PHYS_BoxInertia(vec3 size, f32 mass)
{
   const f32 d = 1.0f / 12.0f;
   f32 m = mass * d;
   vec3 s = Util_MulVec3(size, size);
   vec3 a = VEC3(
      m * (s.y + s.z),
      m * (s.x + s.z),
      m * (s.x + s.y)
   );

   return MAT3(
      a.x, 0.f, 0.f,
      0.f, a.y, 0.f,
      0.f, 0.f, a.z
   );
}

void PHYS_AddSupport(PhysicsWorld* world, phys_Support support, u32 index)
{
   INSERT_ARRAY(world->supports, index, support);
}

void PHYS_ShrinkSupports(PhysicsWorld* world, u32 count)
{
   u32 support_count = Util_ArrayLength(world->supports);
   SET_ARRAY_LENGTH(world->supports, support_count - count);
}

void PHYS_ClearSupports(PhysicsWorld* world)
{
   SET_ARRAY_LENGTH(world->supports, 0);
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

bool PHYS_EvolveGJK(PhysicsWorld* world, phys_Manifold* manifold, vec3* direction)
{
   u32 support_count = Util_ArrayLength(world->supports);
   switch (support_count)
   {
      case 2:
         return PHYS_EvolveGJK_2(world, manifold, direction);
      case 3:
         return PHYS_EvolveGJK_3(world, manifold, direction);
      case 4:
         return PHYS_EvolveGJK_4(world, manifold, direction);

      default:
         break;
   }

   return false;
}

bool PHYS_EvolveGJK_2(PhysicsWorld* world, phys_Manifold* manifold, vec3* direction)
{
   phys_Support support_a = world->supports[0];
   phys_Support support_b = world->supports[1];

   vec3 a = support_a.world_point;
   vec3 b = support_b.world_point;

   vec3 b_a = Util_SubVec3(b, a);
   vec3 o_a = Util_ScaleVec3(a,-1);

   if (PHYS_IsInDirection(b_a, o_a))
   {
      *direction = Util_Cross(Util_Cross(b_a, o_a), b_a);
   } else {
      PHYS_ShrinkSupports(world, 1);
      *direction = o_a;
   }

   return false;
}

bool PHYS_EvolveGJK_3(PhysicsWorld* world, phys_Manifold* manifold, vec3* direction)
{
   phys_Support support_a = world->supports[0];
   phys_Support support_b = world->supports[1];
   phys_Support support_c = world->supports[2];
   
   vec3 a = support_a.world_point;
   vec3 b = support_b.world_point;
   vec3 c = support_c.world_point;

   vec3 b_a = Util_SubVec3(b, a);
   vec3 c_a = Util_SubVec3(c, a);
   vec3 o_a = Util_ScaleVec3(a,-1);

   vec3 b_axc_a = Util_Cross(b_a, c_a);

   if (PHYS_IsInDirection(Util_Cross(b_axc_a, c_a), o_a))
   {
      if (PHYS_IsInDirection(c_a, o_a))
      {
         world->supports[1] = support_c;

         PHYS_ShrinkSupports(world, 1);
         *direction = Util_Cross(Util_Cross(c_a, o_a), c_a);
      } else {
         PHYS_ShrinkSupports(world, 1);
         return PHYS_EvolveGJK_2(world, manifold, direction);
      }
   } else {
      if (PHYS_IsInDirection(Util_Cross(b_a, b_axc_a), o_a))
      {
         PHYS_ShrinkSupports(world, 1);
         return PHYS_EvolveGJK_2(world, manifold, direction);
      } else {
         if (PHYS_IsInDirection(b_axc_a, o_a))
            *direction = b_axc_a;
         else {
            world->supports[1] = support_c;
            world->supports[2] = support_b;

            *direction = Util_ScaleVec3(b_axc_a,-1);
         }
      }
   }

   return false;
}

bool PHYS_EvolveGJK_4(PhysicsWorld* world, phys_Manifold* manifold, vec3* direction)
{
   phys_Support support_a = world->supports[0];
   phys_Support support_b = world->supports[1];
   phys_Support support_c = world->supports[2];
   phys_Support support_d = world->supports[3];

   vec3 a = support_a.world_point;
   vec3 b = support_b.world_point;
   vec3 c = support_c.world_point;
   vec3 d = support_d.world_point;

   vec3 b_a = Util_SubVec3(b, a);
   vec3 c_a = Util_SubVec3(c, a);
   vec3 d_a = Util_SubVec3(d, a);
   vec3 o_a = Util_ScaleVec3(a,-1);

   vec3 b_axc_a = Util_Cross(b_a, c_a);
   vec3 c_axd_a = Util_Cross(c_a, d_a);
   vec3 d_axb_a = Util_Cross(d_a, b_a);

   if (PHYS_IsInDirection(b_axc_a, o_a))
   {
      PHYS_ShrinkSupports(world, 1);
      return PHYS_EvolveGJK_3(world, manifold, direction);
   }

   if (PHYS_IsInDirection(c_axd_a, o_a))
   {
      world->supports[1] = support_c;
      world->supports[2] = support_d;

      PHYS_ShrinkSupports(world, 1);
      return PHYS_EvolveGJK_3(world, manifold, direction);
   }

   if (PHYS_IsInDirection(d_axb_a, o_a))
   {
      world->supports[1] = support_d;
      world->supports[2] = support_b;

      PHYS_ShrinkSupports(world, 1);
      return PHYS_EvolveGJK_3(world, manifold, direction);
   }

   return true;
}

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

   for (u32 i=0; i<Util_ArrayLength(world->epa_edges); i++)
   {
      phys_EpaEdge edge = world->epa_edges[i];

      phys_DebugVertex v1 = {
         .position = world->supports[edge.idx[0]].world_point,
         .render_type = 13
      };

      phys_DebugVertex v2 = {
         .position = world->supports[edge.idx[1]].world_point,
         .render_type = 13
      };

      ADD_BACK_ARRAY(world->debug.lines, v1);
      ADD_BACK_ARRAY(world->debug.lines, v2);
   }

   vec3 barycentric = PHYS_BarycentricCoords(world, closest_face, VEC3(0, 0, 0));

   phys_Contact contact = { 0 };
   contact.object_point[0] = PHYS_ProjectLocalPoint(world, closest_face, barycentric, 0);
   contact.object_point[1] = PHYS_ProjectLocalPoint(world, closest_face, barycentric, 1);
   contact.world_point[0] = PHYS_ObjectPointInWorld(manifold->body[0], contact.object_point[0]);
   contact.world_point[1] = PHYS_ObjectPointInWorld(manifold->body[1], contact.object_point[1]);
   contact.basis = PHYS_MakeBasis(world, closest_face.normal.xyz);
   contact.depth = closest_face.normal.w + epa_epsilon;

   manifold->contact_count = 1;
   manifold->contacts[0] = contact;

   phys_DebugVertex v1 = { .position = contact.world_point[0], .render_type = 13 };
   phys_DebugVertex v2 = { .position = contact.world_point[1], .render_type = 13 };
   ADD_BACK_ARRAY(world->debug.points, v1);
   ADD_BACK_ARRAY(world->debug.points, v2);

   u32 face_count = Util_ArrayLength(world->epa_faces);
   for (u32 i=0; i<face_count; i++)
   {
      phys_EpaFace face = world->epa_faces[i];
      for (u32 j=0; j<3; j++)
      {
         phys_DebugVertex dbg_vertex = { .render_type = 9 };

         dbg_vertex.position = world->supports[face.idx[j]].world_point;
         ADD_BACK_ARRAY(world->debug.faces, dbg_vertex);
      }
   }
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

phys_Support PHYS_GetSupport(phys_Manifold* manifold, vec3 direction)
{
   phys_Support support_point = { 0 };

   vec3 object_dir_1 = PHYS_WorldVectorInObject(manifold->body[0], direction);
   vec3 object_dir_2 = PHYS_WorldVectorInObject(manifold->body[1], Util_ScaleVec3(direction,-1));

   support_point.object_point[0] = PHYS_BBoxSupportPoint(manifold->body[0], object_dir_1);
   support_point.object_point[1] = PHYS_BBoxSupportPoint(manifold->body[1], object_dir_2);

   vec3 world_point_1 = PHYS_ObjectPointInWorld(manifold->body[0], support_point.object_point[0]);
   vec3 world_point_2 = PHYS_ObjectPointInWorld(manifold->body[1], support_point.object_point[1]);

   support_point.world_point = Util_SubVec3(world_point_1, world_point_2);

   return support_point;
}

vec3 PHYS_BBoxSupportPoint(phys_PhysicsBody* body, vec3 direction)
{
   vec3 s = VEC3(
      (direction.x >= 0) ? body->bounds.extents.x :-body->bounds.extents.x,
      (direction.y >= 0) ? body->bounds.extents.y :-body->bounds.extents.y,
      (direction.z >= 0) ? body->bounds.extents.z :-body->bounds.extents.z
   );

   vec3 object_point = Util_AddVec3(
      body->bounds.center,
      s
   );

   return object_point;
}
