#ifndef ECT_PHYSICS_H
#define ECT_PHYSICS_H

#include "util/types.h"
#include "util/extra_types.h"

typedef handle PhysicsBody;

typedef struct PhysicsBodyDesc_t
{
   Transform3D transform;
   BBox bounds;
   f32 mass;
} PhysicsBodyDesc;

typedef struct PhysicsDebugInfo_t
{
   u32 point_count;
   u32 line_count;
   u32 face_count;

   void* points;
   void* lines;
   void* faces;

} PhysicsDebugInfo;

typedef struct PhysicsWorld_t PhysicsWorld;

PhysicsWorld* Physics_Init(void);
void Physics_Free(PhysicsWorld* world);

void Physics_Update(PhysicsWorld* world, f32 delta);

PhysicsBody Physics_AddBody(PhysicsWorld* world, PhysicsBodyDesc* desc);
vec3 Physics_GetBodyOrigin(PhysicsWorld* world, PhysicsBody res_body);
void Physics_MoveBody(PhysicsWorld* world, PhysicsBody res_body, vec3 translate);

PhysicsDebugInfo Physics_GetDebugInfo(PhysicsWorld* world);

#endif
