#include "util/types.h"
#include "util/extra_types.h"
#include "graphics.h"

#include "mesh.h"

#include <stdlib.h>

typedef struct MSH_MeshHeader_t
{
   u16 version_number;
   u16 index_count;
   u16 vertex_count;
   u8 primitive;
   u8 attribute_count;
   u16 material_id;
} MSH_MeshHeader;

void Mesh_Free(Mesh* mesh)
{
   if (mesh->vertex_buffer != NULL)
   {
      free(mesh->vertex_buffer);
      mesh->vertex_buffer = NULL;
   }

   if (mesh->index_buffer != NULL)
   {
      free(mesh->index_buffer);
      mesh->index_buffer = NULL;
   }

   mesh->vertex_count = 0;
   mesh->index_count = 0;
}

Mesh Mesh_LoadEctorMesh(memblob memory)
{
   if ((memory.data == NULL) || (memory.size < sizeof(MSH_MeshHeader)))
      return (Mesh){ 0 };

   void* read_head = memory.data;

   MSH_MeshHeader mesh_header = READ_HEAD(read_head, MSH_MeshHeader);

   if ((mesh_header.vertex_count == 0) || (mesh_header.attribute_count == 0))
      return (Mesh){ 0 };

   uS index_size = mesh_header.index_count * sizeof(u16);
   uS vertex_size = 0;

   Mesh mesh = { .attribute_count = mesh_header.attribute_count };
   for (u8 attribute_i = 0; attribute_i < mesh.attribute_count; attribute_i++)
   {
      u8 attribute = READ_HEAD(read_head, u8);
      switch (attribute)
      {
         case GFX_ATTRIBUTE_F32_1X:
         case GFX_ATTRIBUTE_U8_4X_NORM:
            vertex_size += (uS)mesh_header.vertex_count * 4;
            break;
         
         case GFX_ATTRIBUTE_F32_2X:
            vertex_size += (uS)mesh_header.vertex_count * 8;
            break;
         
         case GFX_ATTRIBUTE_F32_3X:
            vertex_size += (uS)mesh_header.vertex_count * 12;
            break;
         
         case GFX_ATTRIBUTE_F32_4X:
            vertex_size += (uS)mesh_header.vertex_count * 16;
            break;

         default:
            break;
      }
      mesh.attributes[attribute_i] = attribute;
   }

   if ((index_size + vertex_size) > memory.size)
      return (Mesh){ 0 };

   mesh.index_count = mesh_header.index_count;
   mesh.vertex_count = mesh_header.vertex_count;
   mesh.primitive = mesh_header.primitive;

   mesh.index_buffer = malloc(index_size);
   mesh.vertex_buffer = malloc(vertex_size);

   memcpy(mesh.index_buffer, Util_ReadThenMove(&read_head, index_size), index_size);
   memcpy(mesh.vertex_buffer, Util_ReadThenMove(&read_head, vertex_size), vertex_size);

   return mesh;
}
