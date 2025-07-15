#ifndef MSH_INTERNAL
#define MSH_INTERNAL

#include "util/types.h"

#include "mesh.h"

// "EBMF" in hex
#define MODEL_MAGIC_ID 0x45424D46

// Header for the Ector Binary Model Format
typedef struct MSH_ModelHeader_t
{
   union {
      u8 string[4]; 
      u32 magic;
   } identifier; // must equal "EBMF"

   u32 mesh_count;
   u32 material_count;

} MSH_ModelHeader;

typedef struct MSH_MeshHeader_t
{
   u16 material_id;
   u16 index_count;
   u16 vertex_count;
   u8 primitive;
   u8 attribute_count;
} MSH_MeshHeader;

Mesh MSH_ParseEctorMesh(memblob* memory);

#endif
