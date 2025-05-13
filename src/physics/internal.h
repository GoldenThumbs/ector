#include "physics.h"
#include "util/types.h"
#include "util/extra_types.h"
#include "util/vec3.h"
#include "util/vec4.h"
// #include "util/quaternion.h"
// #include "physics.h"

#define PHYS_MAX 64
#define PHYS_MARGIN 0.5f

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

   u32 is_static; // TODO: flags
} phys_PhysicsBody;

typedef struct phys_Contact_t
{
   vec3 object_point[2];
   vec3 world_point[2];
   
   mat3x3 basis; // tangent[0], tangent[1], normal
   
   f32 depth;
   
   struct {
      f32 t[2];
      f32 n;
   } impulse;

   struct {
      f32 t[2];
      f32 n;
   } mass;

   f32 bias;

   u32 warmed;
} phys_Contact;

typedef struct phys_Manifold_t
{
   phys_PhysicsBody* body[2];
   BBox aabb;
   u32 contact_count;
   phys_Contact contacts[4];
   i32 next_warm;
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

typedef union phys_EpaEdge_t
{
   u64 edge_id;
   u32 idx[2];
} phys_EpaEdge;

struct PhysicsWorld_t
{
   phys_PhysicsBody* bodies;
   phys_Manifold* manifolds;
   u16 ref;
   i32 warmed_root;
   
   phys_Support* supports;
   phys_EpaFace* epa_faces;
   phys_EpaEdge* epa_edges;

   struct {
      phys_DebugVertex* points;
      phys_DebugVertex* lines;
      phys_DebugVertex* faces;
   } debug;
};

static inline bool PHYS_IsInDirection(vec3 vector, vec3 direction)
{
   return (Util_DotVec3(vector, direction) > 0);
}

static inline quat PHYS_QuatApproxAxisAngle(vec3 vector)
{
   vec3 v = Util_ScaleVec3(vector, 0.5f);
   return Util_NormalizeVec4(Util_VecF32Vec4(v, 1.0f));
}

void PHYS_PreStep(PhysicsWorld* world, f32 delta);
void PHYS_PostStep(PhysicsWorld* world, f32 delta);
void PHYS_Solver(PhysicsWorld* world, f32 delta);
void PHYS_BiasSolver(PhysicsWorld* world, f32 delta);

bool PHYS_TestCollisonCoarse(phys_PhysicsBody* body_1, phys_PhysicsBody* body_2, phys_Manifold* result);
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

bool PHYS_TestGJK(PhysicsWorld* world, phys_Manifold* manifold);
bool PHYS_EvolveGJK(PhysicsWorld* world, phys_Manifold* manifold, vec3* direction);
bool PHYS_EvolveGJK_2(PhysicsWorld* world, phys_Manifold* manifold, vec3* direction);
bool PHYS_EvolveGJK_3(PhysicsWorld* world, phys_Manifold* manifold, vec3* direction);
bool PHYS_EvolveGJK_4(PhysicsWorld* world, phys_Manifold* manifold, vec3* direction);

void PHYS_RunEPA(PhysicsWorld* world, phys_Manifold* manifold);
bool PHYS_InitPolytope(PhysicsWorld* world, u32* closest_face_idx);
u32 PHYS_GrowPolytope(PhysicsWorld* world, phys_Support support);

vec3 PHYS_BarycentricCoords(PhysicsWorld* world, phys_EpaFace face, vec3 point);
vec3 PHYS_ProjectLocalPoint(PhysicsWorld* world, phys_EpaFace face, vec3 barycentric, u32 body_idx);
mat3x3 PHYS_MakeBasis(PhysicsWorld* world, vec3 normal);

phys_EpaFace PHYS_MakeFace(PhysicsWorld* world, u32 idx_a, u32 idx_b, u32 idx_c);
void PHYS_RemoveFaces(PhysicsWorld* world, u32 point_idx);
u32 PHYS_RepairFaces(PhysicsWorld* world, u32 point_idx);

void PHYS_AddEdge(PhysicsWorld* world, phys_EpaEdge edge);
void PHYS_RemoveEdge(PhysicsWorld* world, u32 index);
void PHYS_ClearEdges(PhysicsWorld* world);

void PHYS_AddContact(phys_Manifold* manifold, phys_Contact contact);

quat PHYS_IntegrateAngularVelocity(vec3 angular_velocity, quat rotation, f32 delta);

phys_Support PHYS_GetSupport(phys_Manifold* manifold, vec3 direction);
vec3 PHYS_BBoxSupportPoint(phys_PhysicsBody* body, vec3 direction);
