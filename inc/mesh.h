#ifndef ECT_MESH_H
#define ECT_MESH_H

#include "util/types.h"

typedef struct Mesh_t
{
   u8* attributes;
   u8* vertex_buffer;
   u16* index_buffer;
   
   u16 vertex_count;
   u16 index_count;
   u8 primitive;
   u8 attribute_count;
} Mesh;

typedef struct MeshInterface_t
{
   u16* vertex_count;
   struct {
      vec3** position;
      vec2** texcoord[2];
      vec3** normal;
      vec4** tangent;
   } atr;

   u16* index_count;
   u16** indices;
} MeshInterface;

void Mesh_Free(Mesh* mesh);

void Mesh_AddQuad(u32 faces_x, u32 faces_y, mat4x4 transform, MeshInterface mesh_interface);
// void GenNormals

Mesh Mesh_CreatePlaneMesh(u32 faces_x, u32 faces_y, vec2 size);
Mesh Mesh_CreateBoxMesh(u32 faces_x, u32 faces_y, u32 faces_z, vec3 size);

#endif
