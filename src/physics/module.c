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

#include <float.h>
#include <stdlib.h>
// #include <stdio.h>

PhysicsWorld* Physics_Init(void)
{
   PhysicsWorld* world = malloc(sizeof(PhysicsWorld));
   world->bodies = NEW_ARRAY_N(phys_PhysicsBody, 8);
   world->manifolds = NEW_ARRAY_N(phys_Manifold, 8);
   world->warmed_root = -1;

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
   
   PHYS_Broadphase(world);
   
   u32 manifold_count = Util_ArrayLength(world->manifolds);
   for (u32 i=0; i<manifold_count; i++)
   {
      phys_Manifold* manifold = world->manifolds + (uS)i;
      for (u32 i=0; i<manifold->contact_count; i++)
      {
         phys_Contact contact = manifold->contacts[i];

         vec3 obj_in_world_1 = PHYS_ObjectPointInWorld(manifold->body[0], contact.object_point[0]);
         vec3 obj_in_world_2 = PHYS_ObjectPointInWorld(manifold->body[1], contact.object_point[1]);

         vec3 diff_1 = Util_SubVec3(obj_in_world_1, contact.world_point[0]);
         vec3 diff_2 = Util_SubVec3(obj_in_world_2, contact.world_point[1]);

         bool close_enough_1 = (Util_DotVec3(diff_1, diff_1) < 0.005f);
         bool close_enough_2 = (Util_DotVec3(diff_2, diff_2) < 0.005f);

         if (!close_enough_1 || !close_enough_2)
            manifold->contacts[i] = manifold->contacts[--manifold->contact_count];
      }
   }
   
   PHYS_Narrowphase(world);

   PHYS_PreStep(world, delta);
   PHYS_Solver(world, delta);
   PHYS_BiasSolver(world, delta);
   PHYS_PostStep(world, delta);
}

PhysicsBody Physics_AddBody(PhysicsWorld* world, PhysicsBodyDesc* desc)
{
   phys_PhysicsBody body = { 0 };
   body.inv_mass = M_RCP(desc->mass, M_FLOAT_FUZZ);
   body.origin = desc->transform.origin;
   body.rotation = Util_QuatToMat3(desc->transform.rotation);
   body.bounds = desc->bounds;
   body.inv_inertia_object = Util_InverseDiagonalMat3(
      PHYS_BoxInertia(
         Util_ScaleVec3(body.bounds.extents, 2),
         desc->mass)
   );
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

      const f32 lin_dampen = 0.0f;
      const f32 ang_dampen = 0.1f;

      body->linear_velocity = Util_ScaleVec3(body->linear_velocity, 1.0f / (1.0f + delta * lin_dampen));
      body->angular_velocity = Util_ScaleVec3(body->angular_velocity, 1.0f / (1.0f + delta * ang_dampen));
   }

   u32 manifold_count = Util_ArrayLength(world->manifolds);
   for (u32 i=0; i<manifold_count; i++)
   {
      phys_Manifold* manifold = world->manifolds + (uS)i;

      phys_PhysicsBody* body_1 = manifold->body[0];
      phys_PhysicsBody* body_2 = manifold->body[1];

      vec3 lin_1 = body_1->linear_velocity;
      vec3 lin_2 = body_2->linear_velocity;
      vec3 ang_1 = body_1->angular_velocity;
      vec3 ang_2 = body_2->angular_velocity;

      for (u32 j=0; j<manifold->contact_count; j++)
      {
         phys_Contact contact = manifold->contacts[j];
         vec3 normal = contact.basis.v[2];

         f32 mass_n = ((!body_1->is_static) ? body_1->inv_mass : 0) + ((!body_2->is_static) ? body_2->inv_mass : 0);
         f32 mass_t[2] = { mass_n, mass_n };

         // vector pointing from contact point to center of mess
         vec3 rel_1 = Util_SubVec3(body_1->origin, contact.world_point[0]);
         vec3 rel_2 = Util_SubVec3(body_2->origin, contact.world_point[1]);

         vec3 rel_x_normal_1 = Util_Cross(rel_1, normal);
         vec3 rel_x_normal_2 = Util_Cross(rel_2, normal);

         mass_n += Util_DotVec3(rel_x_normal_1, Util_MulMat3Vec3(body_1->inv_inertia_world, rel_x_normal_1));
         mass_n += Util_DotVec3(rel_x_normal_2, Util_MulMat3Vec3(body_2->inv_inertia_world, rel_x_normal_2));
         contact.mass.n = M_RCP(mass_n, M_FLOAT_FUZZ);

         for (u32 k=0; k<2; k++)
         {
            vec3 tangent = contact.basis.v[k];
            vec3 rel_x_tangent_1 = Util_Cross(rel_1, tangent);
            vec3 rel_x_tangent_2 = Util_Cross(rel_2, tangent);

            mass_t[k] += Util_DotVec3(rel_x_tangent_1, Util_MulMat3Vec3(body_1->inv_inertia_world, rel_x_tangent_1));
            mass_t[k] += Util_DotVec3(rel_x_tangent_2, Util_MulMat3Vec3(body_2->inv_inertia_world, rel_x_tangent_2));
            contact.mass.t[k] = M_RCP(mass_t[k], M_FLOAT_FUZZ);
         }

         const f32 b_slop = 0.05f;
         const f32 b_fact = 0.2f;
         contact.bias = b_fact * M_MAX(contact.depth - b_slop, 0) * (1.0f / delta);

         vec3 impulse = Util_ScaleVec3(normal, contact.impulse.n);
         impulse = Util_AddVec3(impulse, Util_ScaleVec3(contact.basis.v[0], contact.impulse.t[0]));
         impulse = Util_AddVec3(impulse, Util_ScaleVec3(contact.basis.v[1], contact.impulse.t[1]));

         if (!body_1->is_static)
         {
            lin_1 = Util_SubVec3(
               lin_1,
               Util_ScaleVec3(impulse, body_1->inv_mass)
            );

            ang_1 = Util_SubVec3(
               ang_1,
               Util_MulMat3Vec3(
                  body_1->inv_inertia_world,
                  Util_Cross(rel_1, impulse)
               )
            );
         }

         if (!body_2->is_static)
         {
            lin_2 = Util_AddVec3(
               lin_2,
               Util_ScaleVec3(impulse, body_2->inv_mass)
            );

            ang_2 = Util_AddVec3(
               ang_2,
               Util_MulMat3Vec3(
                  body_2->inv_inertia_world,
                  Util_Cross(rel_2, impulse)
               )
            );
         }

         vec3 delta_vel = Util_SubVec3(
            Util_AddVec3(lin_2, Util_Cross(ang_2, rel_2)),
            Util_AddVec3(lin_1, Util_Cross(ang_1, rel_1))
         );

         // NOTE: hardcoded for the time being (a bad idea)
         const f32 restitution = 0.0f;

         f32 delta_speed = Util_DotVec3(delta_vel, normal);
         if (delta_speed < -1)
            contact.bias -= restitution * delta_speed;

         manifold->contacts[j] = contact;
      }

      body_1->linear_velocity = lin_1;
      body_2->linear_velocity = lin_2;
      body_1->angular_velocity = ang_1;
      body_2->angular_velocity = ang_2;
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
   
      body->angular_velocity = Util_AddVec3(
         body->angular_velocity,
         Util_ScaleVec3(Util_MulMat3Vec3(body->inv_inertia_world, body->total_torque), delta)
      );

      body->total_force = VEC3(0, 0, 0);
      body->total_torque = VEC3(0, 0, 0);

      body->origin = Util_AddVec3(
         body->origin,
         Util_ScaleVec3(body->linear_velocity, delta)
      );

      quat body_q = Util_MakeQuatMat3(body->rotation);
      quat q = PHYS_IntegrateAngularVelocity(body->angular_velocity, body_q, delta);
      body->rotation = Util_QuatToMat3(q);
   }
}

void PHYS_Solver(PhysicsWorld* world, f32 delta)
{
   const f32 b_slop = 0.005f;
   const f32 b_fact = 0.3f;

   u32 manifold_count = Util_ArrayLength(world->manifolds);
   for (u32 i=0; i<manifold_count; i++)
   {
      phys_Manifold* manifold = world->manifolds + (uS)i;

      phys_PhysicsBody* body_1 = manifold->body[0];
      phys_PhysicsBody* body_2 = manifold->body[1];

      vec3 lin_1 = body_1->linear_velocity;
      vec3 lin_2 = body_2->linear_velocity;
      vec3 ang_1 = body_1->angular_velocity;
      vec3 ang_2 = body_2->angular_velocity;

      for (u32 j=0; j<manifold->contact_count; j++)
      {
         phys_Contact contact = manifold->contacts[j];
         vec3 normal = contact.basis.v[2];

         vec3 rel_1 = Util_SubVec3(body_1->origin, contact.world_point[0]);
         vec3 rel_2 = Util_SubVec3(body_2->origin, contact.world_point[1]);

         vec3 delta_vel = Util_SubVec3(
            Util_AddVec3(lin_2, Util_Cross(ang_2, rel_2)),
            Util_AddVec3(lin_1, Util_Cross(ang_1, rel_1))
         );

         for (u32 k=0; k<2; k++)
         {
            vec3 tangent = contact.basis.v[k];
            
            // NOTE: harcoded friction for now (also a bad idea)
            const f32 friction = 0.4f;

            f32 lambda = -Util_DotVec3(delta_vel, tangent) * contact.mass.t[k];
            f32 lambda_max = friction * contact.impulse.n;

            f32 tmp_impulse = contact.impulse.t[k];
            contact.impulse.t[k] = M_CLAMP(tmp_impulse + lambda,-lambda_max, lambda_max);
            lambda = contact.impulse.t[k] - tmp_impulse;

            vec3 impulse = Util_ScaleVec3(tangent, lambda);

            if (!body_1->is_static)
            {
               lin_1 = Util_SubVec3(
                  lin_1,
                  Util_ScaleVec3(impulse, body_1->inv_mass)
               );

               ang_1 = Util_SubVec3(
                  ang_1,
                  Util_MulMat3Vec3(
                     body_1->inv_inertia_world,
                     Util_Cross(rel_1, impulse)
                  )
               );
            }

            if (!body_2->is_static)
            {
               lin_2 = Util_AddVec3(
                  lin_2,
                  Util_ScaleVec3(impulse, body_2->inv_mass)
               );

               ang_2 = Util_AddVec3(
                  ang_2,
                  Util_MulMat3Vec3(
                     body_2->inv_inertia_world,
                     Util_Cross(rel_2, impulse)
                  )
               );
            }
         }

         delta_vel = Util_SubVec3(
            Util_AddVec3(lin_2, Util_Cross(ang_2, rel_2)),
            Util_AddVec3(lin_1, Util_Cross(ang_1, rel_1))
         );

         f32 delta_speed = Util_DotVec3(delta_vel, normal);
         f32 lambda = contact.mass.n * -delta_speed;

         f32 tmp_impulse = contact.impulse.n;
         contact.impulse.n = M_MAX(tmp_impulse + lambda, 0);
         lambda = contact.impulse.n - tmp_impulse;

         vec3 impulse = Util_ScaleVec3(normal, lambda);

         if (!body_1->is_static)
         {
            lin_1 = Util_SubVec3(
               lin_1,
               Util_ScaleVec3(impulse, body_1->inv_mass)
            );

            ang_1 = Util_SubVec3(
               ang_1,
               Util_MulMat3Vec3(
                  body_1->inv_inertia_world,
                  Util_Cross(rel_1, impulse)
               )
            );
         }

         if (!body_2->is_static)
         {
            lin_2 = Util_AddVec3(
               lin_2,
               Util_ScaleVec3(impulse, body_2->inv_mass)
            );

            ang_2 = Util_AddVec3(
               ang_2,
               Util_MulMat3Vec3(
                  body_2->inv_inertia_world,
                  Util_Cross(rel_2, impulse)
               )
            );
         }

         manifold->contacts[j] = contact;
      }

      body_1->linear_velocity = lin_1;
      body_2->linear_velocity = lin_2;
      body_1->angular_velocity = ang_1;
      body_2->angular_velocity = ang_2;
   }
}

void PHYS_BiasSolver(PhysicsWorld* world, f32 delta)
{
   const f32 b_slop = 0.005f;
   const f32 b_fact = 0.1f;

   u32 manifold_count = Util_ArrayLength(world->manifolds);
   for (u32 i=0; i<manifold_count; i++)
   {
      phys_Manifold* manifold = world->manifolds + (uS)i;

      phys_PhysicsBody* body_1 = manifold->body[0];
      phys_PhysicsBody* body_2 = manifold->body[1];

      for (u32 j=0; j<manifold->contact_count; j++)
      {
         phys_Contact contact = manifold->contacts[j];
         vec3 normal = contact.basis.v[2];

         vec3 rel_1 = Util_SubVec3(body_1->origin, contact.world_point[0]);
         vec3 rel_2 = Util_SubVec3(body_2->origin, contact.world_point[1]);

         vec3 impulse = Util_ScaleVec3(normal, contact.bias / contact.mass.n);

         if (!body_1->is_static)
         {
            body_1->origin = Util_SubVec3(
               body_1->origin,
               impulse
            );

            quat body_q = Util_MakeQuatMat3(body_1->rotation);

            quat q = PHYS_IntegrateAngularVelocity(Util_Cross(rel_1, impulse), body_q,-delta);
            body_1->rotation = Util_QuatToMat3(q);
         }

         if (!body_2->is_static)
         {
            body_2->origin = Util_AddVec3(
               body_2->origin,
               impulse
            );

            quat body_q = Util_MakeQuatMat3(body_2->rotation);

            quat q = PHYS_IntegrateAngularVelocity(Util_Cross(rel_2, impulse), body_q, delta);
            body_2->rotation = Util_QuatToMat3(body_q);
         }
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
         .next_warm = -1
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
         {
            bool is_new = true;

            u32 manifold_count = Util_ArrayLength(world->manifolds);
            for (u32 k=0; k<manifold_count; k++)
            {
               phys_Manifold m = world->manifolds[k];
               if (
                  ((m.body[0] == body_1) && (m.body[1] == body_2)) ||
                  ((m.body[0] == body_2) && (m.body[1] == body_1)))
               {
                  is_new = false;
                  world->manifolds[k].next_warm = world->warmed_root;
                  world->warmed_root = k;

                  for (u32 l=0; l<m.contact_count; l++)
                  {
                     world->manifolds[k].contacts[l].impulse.n = 0;
                  }

                  break;
               }
            }

            if (is_new)
               ADD_BACK_ARRAY(world->manifolds, manifold);
         }
      }
   }
}

void PHYS_Narrowphase(PhysicsWorld* world)
{
   u32 manifold_count = Util_ArrayLength(world->manifolds);
   for (u32 i=0; i<manifold_count; i++)
   {
      phys_Manifold* manifold = world->manifolds + (uS)i;

      if (!PHYS_TestGJK(world, manifold))
      {
         world->manifolds[i] = POP_BACK_ARRAY(world->manifolds);
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

void PHYS_AddContact(phys_Manifold* manifold, phys_Contact contact)
{
   if (manifold->next_warm < 0)
   {
      manifold->contact_count = 1;
      manifold->contacts[0] = contact;
      return;
   }

   u32 min_idx = 0;
   f32 min_dist = FLT_MAX;

   vec3 summed = VEC3(0, 0, 0);

   for (u32 i=0; i<manifold->contact_count; i++)
   {
      phys_Contact c = manifold->contacts[i];
      vec3 diff = Util_SubVec3(contact.world_point[0], c.world_point[0]);

      if (Util_DotVec3(diff, diff) < 0.02f)
         return;

      summed = Util_AddVec3(summed, c.world_point[0]);
      vec3 centroid = Util_ScaleVec3(summed, M_RCP((f32)(i + 1), M_FLOAT_FUZZ));

      vec3 d1 = Util_SubVec3(centroid, c.world_point[0]);
      f32 dist1 = Util_DotVec3(d1, d1);

      if (dist1 < min_dist)
      {
         min_idx = i;
         min_dist = dist1;
      }

      vec3 d2 = Util_SubVec3(centroid, contact.world_point[0]);
      f32 dist2 = Util_DotVec3(d2, d2);

      if (dist2 < dist1)
         return;
   }

   u32 idx = manifold->contact_count;
   if (idx == 4)
      idx = min_idx;

   manifold->contact_count = M_MIN(manifold->contact_count + 1, 4);
   manifold->contacts[idx] = contact;
}

quat PHYS_IntegrateAngularVelocity(vec3 angular_velocity, quat rotation, f32 delta)
{
   vec3 scaled_vel = Util_ScaleVec3(angular_velocity, delta);
   return Util_MulQuat(PHYS_QuatApproxAxisAngle(scaled_vel), rotation);
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
