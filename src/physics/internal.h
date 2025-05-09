#include "physics.h"
#include "util/types.h"
#include "util/extra_types.h"
#include "util/vec3.h"
//#include "physics.h"

#define PHYS_MAX 64
#define PHYS_BIAS 0.0f

typedef struct phys_PhysicsBody_t
{
   f32 inv_mass;

   vec3 origin;
   mat3x3 rotation;
   mat3x3 inv_rotation;

   BBox bounds; // object-oriented bounding box
   BBox aabb; // axis-aligned bounding box

   vec3 angular_velocity;
   vec3 linear_velocity;

   vec3 total_force;
   vec3 total_torque;

   mat3x3 inv_inertia_object;
   mat3x3 inv_inertia_world;

   handle compare;
} phys_PhysicsBody;

typedef struct phys_Contact_t
{
   vec3 object_point[2];
   vec3 world_point[2];
   
   mat3x3 basis; // tangent[0], tangent[1], normal
   
   f32 depth;
   
   struct {
      f32 tangent[2];
      f32 normal;
   } impulse_sums;
} phys_Contact;

typedef struct phys_Manifold_t
{
   phys_PhysicsBody* body[2];
   BBox aabb;
   u32 contact_count;
   phys_Contact contacts[4];
   i32 next;
} phys_Manifold;


// debug vertex render types:
// - [0] solid, white
// - [1] solid, red
// - [2] solid, orange
// - [3] solid, blue
// - [4] checker, white
// - [5] checker, red
// - [6] checker, orange
// - [7] checker, blue
// - [8] solid, normal
// - [9] checker, normal
typedef struct phys_DebugVertex_t
{
   vec3 position;
   u32 render_type;
} phys_DebugVertex;

typedef struct phys_Support_t
{
   vec3 world_point;
   vec3 object_point[2];
} phys_Support;

typedef struct phys_EpaFace_t
{
   u32 idx[3];
   vec4 normal;
} phys_EpaFace;

typedef union phys_EpaEdge_y
{
   u64 edge_id;
   u32 idx[2];
} phys_EpaEdge;

struct PhysicsWorld_t
{
   phys_PhysicsBody* bodies;
   phys_Manifold* manifolds;
   u16 ref;
   i32 manifold_root;
   
   phys_Support* supports;
   phys_EpaFace* epa_faces;
   phys_EpaEdge* epa_edges;

   struct {
      phys_DebugVertex* points;
      phys_DebugVertex* lines;
      phys_DebugVertex* faces;
   } debug;
};

static inline bool PHYS_IsInDirection(vec3 v, vec3 direction)
{
   return (Util_DotVec3(v, direction) > 0);
}

bool PHYS_TestCollisonCoarse(phys_PhysicsBody* body_1, phys_PhysicsBody* body_2, phys_Manifold* result);
bool PHYS_TestGJK(PhysicsWorld* world, phys_Manifold* manifold);
void PHYS_Broadphase(PhysicsWorld* world);
void PHYS_Narrowphase(PhysicsWorld* world);

void PHYS_BodyApplyForce(phys_PhysicsBody* body, vec3 force, vec3 point);
void PHYS_BodyUpdateRotation(phys_PhysicsBody* body);

vec3 PHYS_ObjectPointInWorld(phys_PhysicsBody* body, vec3 point);
vec3 PHYS_WorldPointInObject(phys_PhysicsBody* body, vec3 point);

vec3 PHYS_ObjectVectorInWorld(phys_PhysicsBody* body, vec3 vector);
vec3 PHYS_WorldVectorInObject(phys_PhysicsBody* body, vec3 vector);

mat3x3 PHYS_BoxInertia(vec3 size, f32 mass);

void PHYS_AddSupport(PhysicsWorld* world, phys_Support support, u32 index);
void PHYS_ShrinkSupports(PhysicsWorld* world, u32 count);
void PHYS_ClearSupports(PhysicsWorld* world);

bool PHYS_EvolveGJK(PhysicsWorld* world, phys_Manifold* manifold, vec3* direction);
bool PHYS_EvolveGJK_2(PhysicsWorld* world, phys_Manifold* manifold, vec3* direction);
bool PHYS_EvolveGJK_3(PhysicsWorld* world, phys_Manifold* manifold, vec3* direction);
bool PHYS_EvolveGJK_4(PhysicsWorld* world, phys_Manifold* manifold, vec3* direction);

// void PHYS_RunEPA(PhysicsWorld* world, phys_Manifold* manifold);

phys_Support PHYS_GetSupport(phys_Manifold* manifold, vec3 direction);
vec3 PHYS_BBoxSupportPoint(phys_PhysicsBody* body, vec3 direction);
