#ifndef ECT_MESH_H
#define ECT_MESH_H

#include "util/extra_types.h"
#include "util/types.h"

#define MESH_MAX_ATTRIBUTES 8

enum {
   MESH_ATTRIBUTE_1_CHANNEL = 0,
   MESH_ATTRIBUTE_2_CHANNEL,
   MESH_ATTRIBUTE_3_CHANNEL,
   MESH_ATTRIBUTE_4_CHANNEL,
   MESH_ATTRIBUTE_COLOR

};

enum {
   MESH_PRIMITIVE_TRIANGLE = 0,
   MESH_PRIMITIVE_INVALID // no clue if i should even be tracking this and why i do at the moment

};

enum {
   MESH_INDEXTYPE_16BIT = 0,
   MESH_INDEXTYPE_32BIT

};

typedef struct Node_t
{
   char* name;
   Transform3D transform;
   u32 child_count;
   i16 parent_id;
   i16 prev_sibling_id;
   i16 next_sibling_id;
   i16 root_child_id;

} Node;

typedef struct Mesh_t
{
   u8* vertex_buffer;
   void* index_buffer;
   
   u32 vertex_count;
   u32 index_count;
   i32 node_id;
   u16 material_id;
   u8 primitive: 7;
   u8 index_type: 1;
   u8 attribute_count;
   u8 attributes[MESH_MAX_ATTRIBUTES];

} Mesh;

typedef struct MeshInterface_t
{
   Mesh* mesh;
   uS total_bytes;

   struct {
      uS position_size;
      uS normal_ofs;
      uS normal_size;
      uS texcoord_ofs[2];
      uS texcoord_size[2];
      uS tangent_ofs;
      uS tangent_size;

   } atr;

} MeshInterface;

#define MATERIAL_MAX_TEXTURES 8
#define MATERIAL_MAX_PARAMS 32

typedef struct Material_t
{
   char* name;
   char* texture_strings[MATERIAL_MAX_TEXTURES];
   struct {
      char* key;
      vec4 value;
   } parameter[MATERIAL_MAX_PARAMS];
} Material;

typedef struct Model_t
{
   Node* nodes;
   Mesh* meshes;
   Material* materials;
   
   u16 version;
   i16 root_bone_id;
   u32 node_count;
   u32 mesh_count;
   u32 material_count;

} Model;

static inline Mesh Mesh_EmptyMeshWithIndexType(const u8 primitive, const u8 index_type)
{
   return (Mesh){
      .vertex_buffer = NULL,
      .index_buffer = NULL,
      .vertex_count = 0,
      .index_count = 0,
      .node_id = -1,
      .material_id = 0,
      .primitive = primitive,
      .index_type = index_type,
      .attribute_count = 0,
      .attributes = { 0 }
   };
}

static inline Mesh Mesh_EmptyMesh(const u8 primitive)
{
   return Mesh_EmptyMeshWithIndexType(primitive, MESH_INDEXTYPE_16BIT);
}

static inline MeshInterface Mesh_NewInterface(Mesh* mesh)
{
   return (MeshInterface){
      .mesh = mesh,
      .total_bytes = 0,
      .atr = { 0 }
   };
}

static inline bool Mesh_InterfaceIsValid(MeshInterface mesh_interface, bool need_position, bool need_normal, bool need_texcoord0, bool need_texcoord1, bool need_tangent)
{
   if (mesh_interface.mesh == NULL)
      return false;

   bool has_position = !need_position || (need_position && mesh_interface.mesh->vertex_buffer != NULL && mesh_interface.atr.position_size != 0);
   bool has_normal = !need_normal || (need_normal && mesh_interface.atr.normal_size != 0);
   bool has_texcoord0 = !need_texcoord0 || (need_texcoord0 && mesh_interface.atr.texcoord_size[0] != 0);
   bool has_texcoord1 = !need_texcoord1 || (need_texcoord1 && mesh_interface.atr.texcoord_size[1] != 0);
   bool has_tangent = !need_tangent || (need_tangent && mesh_interface.atr.tangent_size != 0);

   return has_position && has_normal && has_texcoord0 && has_texcoord1 && has_tangent;
}

void Model_Free(Model* model);
void Mesh_Free(Mesh* mesh);

void Mesh_SetIndexInBuffer(Mesh* mesh, u32 at_index, u32 index_value);
u32 Mesh_GetIndexFromBuffer(Mesh mesh, u32 at_index);

MeshInterface Mesh_ReallocVertices(u32 vertex_count, bool use_normal, bool use_texcoord0, bool use_texcoord1, bool use_tangent, MeshInterface mesh_interface);

MeshInterface Mesh_AddQuad(u32 faces_x, u32 faces_y, mat4x4 transform, MeshInterface mesh_interface);
MeshInterface Mesh_AddQuadAdvanced(u32 faces_x, u32 faces_y, mat4x4 transform, vec4 texture_coords, MeshInterface mesh_interface);
MeshInterface Mesh_GenNormals(MeshInterface mesh_interface);
MeshInterface Mesh_AverageNormalsOverSeams(MeshInterface mesh_interface);
MeshInterface Mesh_GenTexcoords(MeshInterface mesh_interface, vec3 triplanar_scale);
MeshInterface Mesh_GenTangents(MeshInterface mesh_interface);

Mesh Mesh_CreatePlane(u32 faces_x, u32 faces_y, vec2 size);
Mesh Mesh_CreateBox(u32 faces_x, u32 faces_y, u32 faces_z, vec3 size);
Mesh Mesh_CreateBoxAdvanced(u32 faces_x, u32 faces_y, u32 faces_z, vec3 size, bool smooth_seams);
Mesh Mesh_CreateSphere(u32 faces, f32 size);

Mesh Mesh_LoadEctorMesh(memblob memory);
Model Mesh_LoadEctorModel(memblob memory);


#endif
