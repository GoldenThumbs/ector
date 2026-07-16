#include "util/types.h"
#include "util/array.h"
#include "util/resource.h"
#include "graphics.h"

#include "renderer.h"
#include "renderer/internal.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

Surface Renderer_AddSurface(Renderer* renderer, const char* name, const SurfaceDesc* desc)
{
   if (renderer == NULL || name == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   uS name_length = strnlen(name, RNDR_NAME_MAX);

   if (name_length < 2 || desc == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   u16 existing_idx = RNDR_GetSurfaceIndex(renderer, name);
   if (existing_idx != INVALID_HANDLE)
      return renderer->surfaces[existing_idx].compare;

   rndr_Surface surface = { 0 };
   surface.pass_count = desc->pass_count;

   for (u32 pass_i = 0; pass_i < desc->pass_count; pass_i++)
      surface.passes[pass_i] = desc->passes[pass_i];

   for (u32 tex_i = 0; tex_i < SURF_MAX_TEXTURES; tex_i++)
      surface.textures[tex_i] = desc->texture_defaults[tex_i];

   surface.name = name;

   if (renderer->freed_surface_root == INVALID_HANDLE)
      return ADD_RESOURCE(renderer->surfaces, surface);

   return REUSE_RESOURCE(renderer->surfaces, surface, renderer->freed_surface_root);
}

void Renderer_RemoveSurface(Renderer* renderer, Surface res_surface)
{
   if (renderer == NULL || !Util_IsHandleValid(renderer->surfaces, res_surface))
      return;

   rndr_Surface surface = renderer->surfaces[res_surface.handle];
   if (surface.compare.ref != res_surface.ref)
      return;

   surface.next_freed = renderer->freed_surface_root;
   renderer->freed_surface_root = (u32)res_surface.handle;

}

Surface Renderer_GetSurface(Renderer* renderer, const char* name)
{
   if (renderer == NULL || name == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   u16 index = RNDR_GetSurfaceIndex(renderer, name);
   if (index == INVALID_HANDLE)
      return (handle){ .id = INVALID_HANDLE_ID };

   return renderer->surfaces[index].compare;
}

SurfacePass Renderer_GetSurfacePass(Renderer* renderer, Surface res_surface, u32 pass_id)
{
   if (renderer == NULL || !Util_IsHandleValid(renderer->surfaces, res_surface))
      return (SurfacePass){ .shader.id = INVALID_HANDLE_ID };

   rndr_Surface* surface = RNDR_GetSurface(renderer, res_surface);
   if (surface == NULL || surface->pass_count <= pass_id)
      return (SurfacePass){ .shader.id = INVALID_HANDLE_ID };

   return surface->passes[pass_id];
}

UniformBlockList Renderer_UseSurfaceMaterial(Renderer* renderer, Transform3D transform, SurfaceMaterial material, color8 color, u32 pass_id)
{
   rndr_Surface* surface = RNDR_GetSurface(renderer, material.surface);
   if (surface == NULL || surface->pass_count  < pass_id + 1)
      return (UniformBlockList){ 0 };

   Renderer_UseMaterialTextures(renderer, material);
   Renderer_UpdateModelData(renderer, transform, color);

   return RNDR_UpdateMaterialUBOs(renderer, material, pass_id);
}

void Renderer_SetSurfaceMaterialTexture(SurfaceMaterial* material, i32 index, i32 bind_slot, Texture texture)
{
   Renderer_SetSurfaceMaterialTextureAdvanced(material, index, bind_slot, texture, (TextureInterpolation){ 0, GFX_TEXTUREFILTER_BILINEAR_LINEAR_MIPMAPS, GFX_TEXTUREWRAP_REPEAT });

}

void Renderer_SetSurfaceMaterialTextureAdvanced(SurfaceMaterial* material, i32 index, i32 bind_slot, Texture texture, TextureInterpolation interpolation_settings)
{
   if (material == NULL)
      return;

   u32 tex_idx = (index >= 0) ? (u32)index : material->texture_count;
   material->texture_count = M_MAX(material->texture_count, tex_idx + 1);
   material->textures[tex_idx].bind_slot = (bind_slot >= 0) ? (u32)bind_slot : tex_idx;
   material->textures[tex_idx].texture = texture;
   material->textures[tex_idx].interpolation_settings = interpolation_settings;

}

void Renderer_UseMaterialTextures(Renderer* renderer, SurfaceMaterial material)
{
   if (renderer == NULL)
      return;

   rndr_Surface* surface = RNDR_GetSurface(renderer, material.surface);
   if (surface == NULL || surface->pass_count < 1)
      return;

   struct {
      bool is_set;
      Texture texture;
      TextureInterpolation interpolation_settings;
   } user_texture_slots[SURF_MAX_TEXTURES] = { 0 };

   for (u32 tex_i = 0; tex_i < material.texture_count; tex_i++)
   {
      SurfaceTexture surf_tex = material.textures[tex_i];

      bool is_slot_reserved = (renderer->texture_slots[surf_tex.bind_slot] == INTERNAL_RNDR_SURF_TEXTURE_RESERVED);
      if (surf_tex.texture.id == INVALID_HANDLE_ID || is_slot_reserved)
         continue;

      user_texture_slots[surf_tex.bind_slot].is_set = true;
      user_texture_slots[surf_tex.bind_slot].texture = surf_tex.texture;
      user_texture_slots[surf_tex.bind_slot].interpolation_settings = surf_tex.interpolation_settings;

   }

   for (u32 slot_i = 0; slot_i < SURF_MAX_TEXTURES; slot_i++)
   {
      if (user_texture_slots[slot_i].is_set)
      {
         RNDR_BindTextureAtSlot(renderer, slot_i, INTERNAL_RNDR_SURF_TEXTURE_USER_SET, user_texture_slots[slot_i].texture);
         Graphics_SetTextureInterpolation(renderer->graphics, user_texture_slots[slot_i].texture, user_texture_slots[slot_i].interpolation_settings);

      } else
         RNDR_BindTextureAtSlot(renderer, slot_i, surface->textures[slot_i], (Texture){ .id = INVALID_HANDLE_ID });

   }
}

u16 RNDR_GetSurfaceIndex(Renderer* renderer, const char* surface_name)
{
   if (renderer == NULL || surface_name == NULL)
      return INVALID_HANDLE;

   u32 surface_count = Util_ArrayLength(renderer->surfaces);
   for (u32 surf_i = 0; surf_i < surface_count; surf_i++)
   {
      if (strncmp(surface_name, renderer->surfaces[surf_i].name, RNDR_NAME_MAX) == 0)
         return (u16)surf_i;
   }

   return INVALID_HANDLE;
}

rndr_Surface* RNDR_GetSurface(Renderer* renderer, Surface res_surface)
{
   if (renderer == NULL || !Util_IsHandleValid(renderer->surfaces, res_surface))
      return NULL;

   rndr_Surface* surface = &renderer->surfaces[res_surface.handle];
   if (surface->compare.id != res_surface.id)
      return NULL;

   return surface;
}

void RNDR_BindTextureAtSlot(Renderer* renderer, u32 bind_slot, u8 texture_default, Texture texture)
{
   if (renderer == NULL || bind_slot >= SURF_MAX_TEXTURES)
      return;

   if (renderer->texture_slots[bind_slot] == INTERNAL_RNDR_SURF_TEXTURE_RESERVED)
      return;

   if (texture_default == INTERNAL_RNDR_SURF_TEXTURE_USER_SET)
   {
      if (texture.id == INVALID_HANDLE_ID)
         return;

      renderer->texture_slots[bind_slot] = INTERNAL_RNDR_SURF_TEXTURE_USER_SET;
      Graphics_BindTexture(renderer->graphics, texture, bind_slot);

   }

   if (texture_default < RNDR_SURF_DEFAULT_TEXTURE_COUNT)
   {
      // if (renderer->texture_slots[bind_slot] == texture_default)
      //    return;

      renderer->texture_slots[bind_slot] = texture_default;
      Graphics_BindTexture(renderer->graphics, renderer->built_in.textures[texture_default], bind_slot);
   }

}

UniformBlockList RNDR_UpdateMaterialUBOs(Renderer* renderer, SurfaceMaterial material, u32 pass_id)
{
   if (renderer == NULL)
      return (UniformBlockList){ 0 };

   rndr_Surface* surface = RNDR_GetSurface(renderer, material.surface);
   if (surface == NULL || surface->pass_count < pass_id + 1)
      return (UniformBlockList){ 0 };

   SurfacePass pass = surface->passes[pass_id];
   UniformBlockList uniform_blocks = { .count = pass.uniform_block_count };

   for (u32 block_i = 0; block_i < uniform_blocks.count; block_i++)
   {
      UniformBlock block = pass.uniform_blocks[block_i];
      void* block_data = material.uniform_block_data[pass_id][block_i];
      Graphics_UpdateBuffer(renderer->graphics, block.ubo, block_data, 1, block.size);

      uniform_blocks.blocks[block_i] = block;

   }

   return uniform_blocks;
}
