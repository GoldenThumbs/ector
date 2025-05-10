#include "util/types.h"
#include "util/array.h"
#include "util/vec3.h"

#include "physics.h"
#include "physics/internal.h"

bool PHYS_TestGJK(PhysicsWorld* world, phys_Manifold* manifold)
{
   PHYS_ClearSupports(world);

   vec3 direction = VEC3(1, 0, 0);

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
