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
   
   PHYS_PreStep(world, delta);

   PHYS_Broadphase(world);
   PHYS_Narrowphase(world);
   PHYS_Solver(world, delta);

   PHYS_PostStep(world, delta);
}

PhysicsBody Physics_AddBody(PhysicsWorld* world, PhysicsBodyDesc* desc)
{
   phys_PhysicsBody body = { 0 };
   body.inv_mass = M_RCP(desc->mass, M_FLOAT_FUZZ);
   body.origin = desc->transform.origin;
   body.rotation = Util_QuatToMat3(desc->transform.rotation);
   body.bounds = desc->bounds;
   body.compare.ref = world->ref;
   body.is_static = (u32)desc->is_static;

   return Util_AddResource(&world->ref, REF(world->bodies), &body);
}

Transform3D Physics_GetBodyTransform(PhysicsWorld* world, PhysicsBody res_body)
{
   phys_PhysicsBody* body = &world->bodies[res_body.handle];
   if (body->compare.ref != res_body.ref)
      return (Transform3D){ 0 };

   return (Transform3D){
      .origin = body->origin,
      .rotation = Util_MakeQuatMat3(body->rotation),
      .scale = VEC3(1, 1, 1)
   };
}

void Physics_SetBodyTransform(PhysicsWorld* world, PhysicsBody res_body, Transform3D transform)
{
   phys_PhysicsBody* body = &world->bodies[res_body.handle];
   if (body->compare.ref != res_body.ref)
      return;

   body->origin = transform.origin;
   body->rotation = Util_QuatToMat3(transform.rotation);
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

void PHYS_PreStep(PhysicsWorld* world, f32 delta)
{
   const vec3 gravity = VEC3(0,-9.8f, 0);
   u32 body_count = Util_ArrayLength(world->bodies);
   for (u32 i=0; i<body_count; i++)
   {
      phys_PhysicsBody* body = world->bodies + (uS)i;
      PHYS_BodyUpdateRotation(body);

      if (body->is_static != 0)
         continue;

      PHYS_BodyApplyForce(body, gravity, body->aabb.center);

      f32 dampen = 1.0f / (1.0f + delta);
      body->linear_velocity = Util_ScaleVec3(body->linear_velocity, dampen);
      body->angular_velocity = Util_ScaleVec3(body->angular_velocity, dampen);
   }
}

void PHYS_PostStep(PhysicsWorld* world, f32 delta)
{
   u32 body_count = Util_ArrayLength(world->bodies);
   for (u32 i=0; i<body_count; i++)
   {
      phys_PhysicsBody* body = world->bodies + (uS)i;

      if (body->is_static != 0)
         continue;

      body->linear_velocity = Util_AddVec3(
         body->linear_velocity,
         Util_ScaleVec3(body->total_force, delta * body->inv_mass)
      );

      body->total_force = VEC3(0, 0, 0);
      body->origin = Util_AddVec3(
         body->origin,
         Util_ScaleVec3(body->linear_velocity, delta)
      );
   }
}

void PHYS_Solver(PhysicsWorld* world, f32 delta)
{
   const f32 b_slop = 0.00002f;
   const f32 b_fact = 0.5f;

   u32 manifold_count = Util_ArrayLength(world->manifolds);
   for (u32 i=0; i<manifold_count; i++)
   {
      phys_Manifold* manifold = world->manifolds + (uS)i;

      phys_PhysicsBody* body_1 = manifold->body[0];
      phys_PhysicsBody* body_2 = manifold->body[1];

      for (u32 j=0; j<manifold->contact_count; j++)
      {
         phys_Contact contact = manifold->contacts[j];
         
         vec3 impulse = Util_ScaleVec3(contact.basis.v[2], contact.depth);
         if ((body_1->is_static == 1) || (body_2->is_static == 1))
            impulse = Util_ScaleVec3(impulse, 2);
         //f32 bias = b_fact * (M_MAX(contact.depth - b_slop, 0.0f));


         if (body_1->is_static == 0)
            body_1->origin = Util_SubVec3(body_1->origin, impulse);
         if (body_2->is_static == 0)
            body_2->origin = Util_AddVec3(body_2->origin, impulse);
      }
   }
}

bool PHYS_TestCollisonCoarse(phys_PhysicsBody* body_1, phys_PhysicsBody* body_2, phys_Manifold* result)
{
   const vec3 margin = VEC3(PHYS_MARGIN, PHYS_MARGIN, PHYS_MARGIN);
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

         if ((body_1->is_static == 1) && (body_2->is_static == 1))
            continue;

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
         // printf("Collision Hit: %i\n", i);
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
