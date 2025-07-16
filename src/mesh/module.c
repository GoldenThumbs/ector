#include "util/types.h"
#include "util/extra_types.h"

#include "mesh/internal.h"
#include "mesh.h"

#include <stdlib.h>

void Model_Free(Model* model)
{
   if (model->meshes != NULL)
   {
      for (u32 mesh_i = 0; mesh_i < model->mesh_count; mesh_i++)
         Mesh_Free(&model->meshes[mesh_i]);
      free(model->meshes);
      model->meshes = NULL;
   }

   if (model->materials != NULL)
   {
      free(model->materials);
      model->materials = NULL;
   }

   model->mesh_count = 0;
   model->material_count = 0;
}

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
   return MSH_ParseEctorMesh(memory, NULL);
}

Model Mesh_LoadEctorModel(memblob memory)
{
   if ((memory.data == NULL) || (memory.size < sizeof(MSH_ModelHeader)))
      return (Model){ 0 };

   void* read_head = memory.data;
   uS size_left = memory.size - sizeof(MSH_ModelHeader);

   MSH_ModelHeader model_header = READ_HEAD(read_head, MSH_ModelHeader);

   if ((model_header.identifier.magic != MODEL_MAGIC_ID) || (model_header.mesh_count == 0))
      return (Model){ 0 };

   Model model = { .mesh_count = model_header.mesh_count, .material_count = model_header.material_count };
   model.meshes = calloc((uS)model.mesh_count, sizeof(Mesh));
   model.materials = (model.material_count > 0) ? calloc((uS)model.material_count, sizeof(Material)) : NULL;
   for (u32 mesh_i = 0; mesh_i < model.mesh_count; mesh_i++)
   {
      uS mesh_size = 0;
      model.meshes[mesh_i] = MSH_ParseEctorMesh((memblob){ read_head, size_left }, &mesh_size);

      size_left -= size_left;
      read_head = read_head + mesh_size;
   }

   return model;
}

Mesh MSH_ParseEctorMesh(memblob memory, uS* mesh_size)
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
         case MESH_ATTRIBUTE_1_CHANNEL:
         case MESH_ATTRIBUTE_COLOR:
            vertex_size += (uS)mesh_header.vertex_count * 4;
            break;
         
         case MESH_ATTRIBUTE_2_CHANNEL:
            vertex_size += (uS)mesh_header.vertex_count * 8;
            break;
         
         case MESH_ATTRIBUTE_3_CHANNEL:
            vertex_size += (uS)mesh_header.vertex_count * 12;
            break;
         
         case MESH_ATTRIBUTE_4_CHANNEL:
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

   *mesh_size = index_size + vertex_size + sizeof(MSH_MeshHeader);

   return mesh;
}
