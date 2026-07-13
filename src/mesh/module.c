#include "util/types.h"
#include "util/extra_types.h"
#include "util/array.h"
#include "util/files.h"
#include "util/math.h"

#include "mesh/internal.h"
#include "mesh.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void Model_Free(Model* model)
{
   if (model->nodes != NULL)
   {
      for (u32 node_i = 0; node_i < model->node_count; node_i++)
         free(model->nodes[node_i].name);

      free(model->nodes);
      model->nodes = NULL;

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
      for (u32 material_i = 0; material_i < model->material_count; material_i++)
      {
         Material material = model->materials[material_i];
         if (material.name != NULL)
            free((char*)material.name);

         if (material.surface_name != NULL)
            free((char*)material.surface_name);

         for (u32 tex_i = 0; tex_i < MATERIAL_MAX_TEXTURES; tex_i++)
         {
            if (material.texture_strings[tex_i] != NULL)
               free((char*)material.texture_strings[tex_i]);

         }

         for (u32 param_i = 0; param_i < MATERIAL_MAX_PARAMS; param_i++)
         {
            if (material.parameter[param_i].key != NULL)
               free((char*)material.parameter[param_i].key);

         }

      }

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

void Mesh_ParseEctorMaterials(memblob memory, Model* inout_model)
{
   if (memory.data == NULL || memory.size == 0 || inout_model == NULL)
      return;

   if (inout_model->materials == NULL)
      inout_model->materials = calloc(inout_model->material_count, sizeof(Material));

   uS char_offset = 0;
   u32 material_count = 0;
   while (true)
   {
      Material material = MSH_ParseNextMaterial(memory, &char_offset);
      if (material.name == NULL)
         break;

      u32 index = (material.id == INVALID_HANDLE_ID) ? material_count : material.id;
      material_count = index + 1;

      if (material_count > inout_model->material_count)
      {
         Material* tmp = realloc(inout_model->materials, material_count * sizeof(Material));
         if (tmp == NULL)
            break;

         inout_model->materials = tmp;

      }

      inout_model->materials[index] = material;

   }

   inout_model->material_count = M_MAX(material_count, inout_model->material_count);

}

Material MSH_ParseNextMaterial(memblob memory, uS* char_offset)
{
   if (memory.data == NULL || memory.size == 0 || char_offset == NULL)
      return (Material){ .name = NULL };

   char* read_head = (char*)memory.data + (*char_offset);
   uS read_size = memory.size - (*char_offset);

   uS buffer_size = 0;
   MSH_MatToken* tokens = MSH_TokenizeMaterial((memblob){ read_head, read_size }, &buffer_size);

   if (tokens == NULL || Util_ArrayLength(tokens) == 0)
   {
      FREE_ARRAY(tokens);

      return (Material){ .name = NULL };
   }

   (*char_offset) += buffer_size;

   bool in_material = false;
   bool in_id = false;
   bool in_surf = false;
   bool in_tex = false;
   bool in_param = false;
   bool has_start = false;

   u32 arg_count = 0;
   u32 param_count = 0;
   bool param_is_float = false;
   i32 tex_slot = 0;

   Material material = {
      .name = NULL,
      .id = INVALID_HANDLE_ID
   };

   for (u32 token_i = 0; token_i < Util_ArrayLength(tokens); token_i++)
   {
      MSH_MatToken token = tokens[token_i];

      if (token.token_type == MSH_MATTOK_INVALID)
      {
         Util_Log(NULL, "Mesh", (error){ .general = ERR_LEVEL_ERROR }, "Encountered invalid token! Material parsing failed.");

         break;
      }

      bool mat_scope = in_material && !in_id && !in_surf && !in_tex && !in_param;

      switch (token.token_type)
      {
         case MSH_MATTOK_VALID_STRING: {
            if (!in_material && material.name == NULL)
               material.name = MSH_CopyTokenString(token);
            else {
               if (in_id)
               {
                  if (arg_count >= 1)
                     Util_Log(NULL, "Mesh", (error){ .general = ERR_LEVEL_WARN }, "Material 'id' only takes 1 argument!");
                  else {
                     token.token_start[token.token_size] = 0;
                     material.id = strtol(token.token_start, NULL, 10);

                  }

               }

               if (in_surf)
               {
                  if (arg_count >= 1)
                     Util_Log(NULL, "Mesh", (error){ .general = ERR_LEVEL_WARN }, "Material 'surf' only takes 1 argument!");
                  else
                     material.surface_name = MSH_CopyTokenString(token);

               }

               if (in_tex)
               {
                  if (arg_count >= 2)
                     Util_Log(NULL, "Mesh", (error){ .general = ERR_LEVEL_WARN }, "Material 'tex' only takes 2 arguments!");
                  else {
                     if (arg_count == 0)
                     {
                        token.token_start[token.token_size] = 0;
                        tex_slot = strtol(token.token_start, NULL, 10);

                     } else
                        material.texture_strings[tex_slot] = MSH_CopyTokenString(token);

                  }

               }

               if (in_param)
               {
                  if (arg_count >= 6)
                     Util_Log(NULL, "Mesh", (error){ .general = ERR_LEVEL_WARN }, "Material 'param' only takes up to 6 arguments!");
                  else {
                     if (arg_count == 0)
                        material.parameter[param_count].key = MSH_CopyTokenString(token);
                     else if (arg_count == 1)
                        param_is_float = (token.token_start[0] == 'f');
                     else {
                        u32 param_idx = M_CLAMP(arg_count - 2, 0, 3);

                        token.token_start[token.token_size] = 0;
                        if (param_is_float)
                        {
                           material.parameter[param_count].type = MAT_PARAMTYPE_F32;
                           material.parameter[param_count].value.as_f32[param_idx] = strtof(token.token_start, NULL);

                        } else {
                           material.parameter[param_count].type = MAT_PARAMTYPE_I32;
                           material.parameter[param_count].value.as_i32[param_idx] = strtol(token.token_start, NULL, 10);

                        }

                        material.parameter[param_count].count++;

                     }

                  }

               }

               arg_count++;

               if (mat_scope)
               {
                  if (strncmp(token.token_start, "id", token.token_size) == 0)
                     in_id = true;
                  else if (strncmp(token.token_start, "surf", token.token_size) == 0)
                     in_surf = true;
                  else if (strncmp(token.token_start, "tex", token.token_size) == 0)
                     in_tex = true;
                  else if (strncmp(token.token_start, "param", token.token_size) == 0)
                     in_param = true;
                  else
                     Util_Log(NULL, "Mesh", (error){ .general = ERR_LEVEL_WARN }, "Unrecognized keyword \"%.*s\"", (i32)token.token_size, token.token_start);

                  arg_count = 0;

               }

            }

         } break;

         case MSH_MATTOK_END_LINE: {
            if (in_material)
            {
               if (in_param)
                  param_count++;

               in_id = false;
               in_surf = false;
               in_tex = false;
               in_param = false;

            }

         } break;

         case MSH_MATTOK_START: {
            has_start = true;

            if (in_material)
               Util_Log(NULL, "Mesh", (error){ .general = ERR_LEVEL_WARN }, "Misplaced open bracket!");
            else if (material.name == NULL)
               Util_Log(NULL, "Mesh", (error){ .general = ERR_LEVEL_WARN }, "Materials require names!");
            else
               in_material = true;

         } break;

         case MSH_MATTOK_STOP: {
            if (!in_material && !has_start)
               Util_Log(NULL, "Mesh", (error){ .general = ERR_LEVEL_WARN }, "Misplaced closing bracket!");
            else
               in_material = false;

         } break;

         default:
            break;
      }

   }

   if (in_material)
      Util_Log(NULL, "Mesh", (error){ .general = ERR_LEVEL_WARN }, "Missing closing bracket!");

   FREE_ARRAY(tokens);

   return material;
}

MSH_MatToken* MSH_TokenizeMaterial(memblob memory, uS* out_buffer_size)
{
   if (memory.data == NULL || memory.size == 0)
      return NULL;

   (*out_buffer_size) = 0;

   char* read_head = (char*)memory.data;
   MSH_MatToken* tokens = NEW_ARRAY(MSH_MatToken);
   uS space_count = 0;

   MSH_MatToken new_token = { .token_start = NULL };
   while ((*out_buffer_size) < memory.size)
   {
      char* prev_head = read_head;
      char head = (*prev_head);
      read_head++;
      (*out_buffer_size)++;

      MSH_MatTokenType token_type = MSH_MATTOK_INVALID;
      u32 token_size = new_token.token_size;

      if (head == '#')
      {
         new_token.token_type = MSH_MATTOK_COMMENT;
         new_token.token_size = 0;

      }

      if (new_token.token_type == MSH_MATTOK_COMMENT)
      {
         new_token.token_size++;

         if (head == '\n')
         {
            ADD_BACK_ARRAY(tokens, new_token);

            new_token.token_type = MSH_MATTOK_INVALID;
            new_token.token_size = 0;

         }

         continue;
      }

      if (isspace(head) || iscntrl(head))
      {
         space_count++;

         continue;
      }

      bool is_seperator = (head == '=' || head == ',');

      if (!is_seperator && (isalnum(head) || ispunct(head)) && head != '{' && head != '}')
      {
         token_type = MSH_MATTOK_VALID_STRING;
         token_size++;

         if (new_token.token_type != token_type)
            space_count = 0;

      }

      switch (head)
      {
         case ';': {
            token_type = MSH_MATTOK_END_LINE;
            token_size = 1;

         } break;

         case '{': {
            token_type = MSH_MATTOK_START;
            token_size = 1;

         } break;

         case '}': {
            token_type = MSH_MATTOK_STOP;
            token_size = 1;

         } break;

         default:
            break;
      }

      if (new_token.token_type == MSH_MATTOK_VALID_STRING && token_type != new_token.token_type)
         ADD_BACK_ARRAY(tokens, new_token);

      if (token_type != MSH_MATTOK_VALID_STRING)
      {
         new_token.token_type = token_type;
         new_token.token_size = token_size;
         new_token.token_start = prev_head;

         if (!is_seperator)
            ADD_BACK_ARRAY(tokens, new_token);

         new_token.token_size = 0;

         if (token_type == MSH_MATTOK_STOP)
            break;

      } else {
         if (new_token.token_type != token_type)
            new_token.token_start = prev_head;

         new_token.token_type = token_type;
         new_token.token_size = token_size + space_count;
         space_count = 0;

      }

   }

   return tokens;
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
