#ifndef MSH_INTERNAL
#define MSH_INTERNAL

#include "util/types.h"

#include "mesh.h"

// "EBMF" in hex
#define MODEL_MAGIC_ID 0x464D4245

// EBMF version
#define EBMF_VERSION 0x0001

#define EBMF_NODE_NAME_MAX 128

// Header for the Ector Binary Model Format
typedef struct MSH_ModelHeader_t
{
   union {
      u8 string[4]; 
      u32 magic;
   } identifier; // must equal "EBMF"

   u16 version;
   i16 root_bone_id;
   u32 node_count;
   u32 mesh_count;
   u32 material_count;

} MSH_ModelHeader;

typedef struct MSH_MeshHeader_t
{
   u32 index_count;
   u32 vertex_count;
   i32 node_id;
   u16 material_id;
   u8 primitive;
   u8 attribute_count;

} MSH_MeshHeader;

Mesh MSH_ParseEctorMesh(memblob memory, uS* mesh_size);

#endif
