#include "util/types.h"
#include "util/extra_types.h"

#include "mesh/internal.h"
#include "mesh.h"

#include <stdlib.h>
#include <string.h>

void Model_Free(Model* model)
{
   if (model->nodes != NULL)
   {
      for (u32 node_i = 0; node_i < model->node_count; node_i++)
         free(model->nodes[node_i].name);
      free(model->meshes);
      model->meshes = NULL;

   }

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

void Mesh_SetIndexInBuffer(Mesh* mesh, u32 at_index, u32 index_value)
{
   if (mesh->index_buffer == NULL)
      return;

   if (mesh->index_type == MESH_INDEXTYPE_16BIT)
   {
      u16* index_buffer = mesh->index_buffer;
      index_buffer[at_index] = (u16)index_value;

   } else {
      u32* index_buffer = mesh->index_buffer;
      index_buffer[at_index] = index_value;

   }
   
}

u32 Mesh_GetIndexFromBuffer(Mesh mesh, u32 at_index)
{
   if (mesh.index_buffer == NULL)
      return UINT32_MAX;

   if (mesh.index_type == MESH_INDEXTYPE_16BIT)
      return (u32)(((u16*)mesh.index_buffer)[at_index]);

   return ((u32*)mesh.index_buffer)[at_index];
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

   MSH_ModelHeader model_header = { 0 };
   READ_HEAD(read_head, model_header, MSH_ModelHeader);

   if (model_header.identifier.magic != MODEL_MAGIC_ID || model_header.mesh_count == 0 || model_header.version != EBMF_VERSION)
      return (Model){ 0 };

   Model model = {
      .version = model_header.version,
      .root_bone_id = model_header.root_bone_id,
      .node_count = model_header.node_count,
      .mesh_count = model_header.mesh_count,
      .material_count = model_header.material_count
   };

   model.meshes = calloc((uS)model.mesh_count, sizeof(Mesh));
   model.nodes = (model.node_count > 0) ? calloc((uS)model.node_count, sizeof(Node)) : NULL;
   model.materials = (model.material_count > 0) ? calloc((uS)model.material_count, sizeof(Material)) : NULL;

   for (u32 node_i = 0; node_i < model.node_count; node_i++)
   {
      uS name_length = strnlen(read_head, EBMF_NODE_NAME_MAX);

      model.nodes[node_i].name = malloc(name_length + 1);
      memcpy(model.nodes[node_i].name, read_head, name_length + 1);

      size_left -= name_length + 1;
      read_head = ((u8*)read_head) + name_length + 1;

      READ_HEAD(read_head, model.nodes[node_i].transform, Transform3D);
      READ_HEAD(read_head, model.nodes[node_i].child_count, u32);
      READ_HEAD(read_head, model.nodes[node_i].parent_id, i16);
      READ_HEAD(read_head, model.nodes[node_i].prev_sibling_id, i16);
      READ_HEAD(read_head, model.nodes[node_i].next_sibling_id, i16);
      READ_HEAD(read_head, model.nodes[node_i].root_child_id, i16);

      size_left -= sizeof(Transform3D) + 12;

   }

   for (u32 mesh_i = 0; mesh_i < model.mesh_count; mesh_i++)
   {
      uS mesh_size = 0;
      model.meshes[mesh_i] = MSH_ParseEctorMesh((memblob){ read_head, size_left }, &mesh_size);

      size_left -= mesh_size;
      read_head = ((u8*)read_head) + mesh_size;

   }

   return model;
}

Mesh MSH_ParseEctorMesh(memblob memory, uS* mesh_size)
{
   if ((memory.data == NULL) || (memory.size < sizeof(MSH_MeshHeader)))
      return (Mesh){ 0 };

   void* read_head = memory.data;

   MSH_MeshHeader mesh_header = { 0 };
   READ_HEAD(read_head, mesh_header, MSH_MeshHeader);

   if ((mesh_header.vertex_count == 0) || (mesh_header.attribute_count == 0))
      return (Mesh){ 0 };

   uS index_size = mesh_header.index_count * sizeof(u16);
   uS vertex_size = 0;

   Mesh mesh = {
      .node_id = mesh_header.node_id,
      .material_id = mesh_header.material_id,
      .attribute_count = mesh_header.attribute_count
   };

   for (u8 attribute_i = 0; attribute_i < mesh.attribute_count; attribute_i++)
   {
      u8 attribute = 0;
      READ_HEAD(read_head, attribute, u8);
      
      switch (attribute)
      {
         case MESH_ATTRIBUTE_1_CHANNEL:
         case MESH_ATTRIBUTE_COLOR:
            vertex_size += 4;
            break;
         
         case MESH_ATTRIBUTE_2_CHANNEL:
            vertex_size += 8;
            break;
         
         case MESH_ATTRIBUTE_3_CHANNEL:
            vertex_size += 12;
            break;
         
         case MESH_ATTRIBUTE_4_CHANNEL:
            vertex_size += 16;
            break;

         default:
            break;

      }

      mesh.attributes[attribute_i] = attribute;

   }

   vertex_size *= (uS)mesh_header.vertex_count;

   if ((index_size + vertex_size) > memory.size)
      return (Mesh){ 0 };

   mesh.index_count = mesh_header.index_count;
   mesh.vertex_count = mesh_header.vertex_count;
   mesh.primitive = mesh_header.primitive;

   mesh.index_buffer = malloc(index_size);
   mesh.vertex_buffer = malloc(vertex_size);

   Util_ReadThenMove(&read_head, mesh.index_buffer, index_size);
   Util_ReadThenMove(&read_head, mesh.vertex_buffer, vertex_size);

   *mesh_size = sizeof(MSH_MeshHeader) + (uS)mesh_header.attribute_count + index_size + vertex_size;

   return mesh;
}
