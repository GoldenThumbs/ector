#ifndef ECT_MESH_H
#define ECT_MESH_H

#include "util/types.h"

#define MESH_MAX_ATTRIBUTES 10

typedef struct Mesh_t
{
   u8* vertex_buffer;
   u16* index_buffer;
   
   u16 vertex_count;
   u16 index_count;
   u8 primitive;
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

static inline Mesh Mesh_EmptyMesh(const u8 primitive)
{
   return (Mesh){
      .vertex_buffer = NULL,
      .index_buffer = NULL,
      .vertex_count = 0,
      .index_count = 0,
      .primitive = primitive,
      .attribute_count = 0,
      .attributes = { 0 }
   };
}

static inline MeshInterface Mesh_NewInterface(Mesh* mesh)
{
   return (MeshInterface){
      .mesh = mesh,
      .total_bytes = 0,
      .atr = { 0 }
   };
}

void Mesh_Free(Mesh* mesh);

MeshInterface Mesh_AddQuad(u32 faces_x, u32 faces_y, mat4x4 transform, MeshInterface mesh_interface);
MeshInterface Mesh_GenNormals(MeshInterface mesh_interface);
MeshInterface Mesh_GenTexcoords(MeshInterface mesh_interface);

Mesh Mesh_CreatePlane(u32 faces_x, u32 faces_y, vec2 size);
Mesh Mesh_CreateBox(u32 faces_x, u32 faces_y, u32 faces_z, vec3 size);

#endif
