#include "util/types.h"
#include "util/extra_types.h"
#include "util/array.h"
#include "util/files.h"
#include "util/resource.h"
#include "util/matrix.h"
#include "mesh.h"
#include "image.h"
#include "graphics.h"

#include "renderer.h"
#include "renderer/internal.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

Renderer* Renderer_Init(Graphics* graphics, const char* app_path)
{
   if (graphics == NULL)
      return NULL;
   
   Renderer* renderer = malloc(sizeof(Renderer));
   if (renderer == NULL)
      return NULL;
   
   renderer->app_path = (char*)app_path;
   renderer->graphics = graphics;
   
   renderer->surfaces = NEW_ARRAY_N(rndr_Surface, 16);
   renderer->drawable_types = NEW_ARRAY_N(rndr_DrawableType, 8);

   renderer->lightmanager_info = (LightManagerInfo){ 0 };

   renderer->freed_surface_root = RNDR_INVALID_LIST_LINK;
   
   renderer->built_in.texture.white = Renderer_CreateColorTexture(renderer, (color8){ .hex = 0XFFFFFFFF }, GFX_TEXTURETYPE_2D);
   renderer->built_in.texture.black = Renderer_CreateColorTexture(renderer, (color8){ .hex = 0xFF000000 }, GFX_TEXTURETYPE_2D);
   renderer->built_in.texture.gray = Renderer_CreateColorTexture(renderer, (color8){ .hex = 0xFF808080 }, GFX_TEXTURETYPE_2D);
   renderer->built_in.texture.normal = Renderer_CreateColorTexture(renderer, (color8){ .hex = 0xFFFF8080 }, GFX_TEXTURETYPE_2D);
   
   renderer->built_in.geometry.plane = RNDR_Plane(graphics);
   renderer->built_in.geometry.box = RNDR_Box(graphics);
   
   renderer->built_in.shader.unlit.id = INVALID_HANDLE_ID;
   renderer->built_in.shader.basic.id = INVALID_HANDLE_ID;

   renderer->ubo.camera_buffer = Graphics_CreateBufferExplicit(
      renderer->graphics, NULL, sizeof(CameraData), GFX_DRAWMODE_DYNAMIC, GFX_BUFFERTYPE_UNIFORM);
   renderer->ubo.model_buffer = Graphics_CreateBufferExplicit(
      renderer->graphics, NULL, sizeof(ModelData), GFX_DRAWMODE_DYNAMIC, GFX_BUFFERTYPE_UNIFORM);

   renderer->near_clip = 0.05f;
   renderer->far_clip = 100.0f;
   renderer->fov = 40.0f;
   renderer->aspect_ratio = 1.0f;
   renderer->size = (resolution2d){ 1, 1 };
   renderer->update_projection = true;
   renderer->update_view_projection = true;

   renderer->view = Util_IdentityMat4();
   renderer->inv_view = Util_IdentityMat4();

   RNDR_RegisterDefaultDrawables(renderer);
    
   return renderer;
}

void Renderer_Free(Renderer* renderer)
{
   if (renderer == NULL)
      return;

   u32 surface_count = Util_ArrayLength(renderer->surfaces);
   for (u32 surf_i = 0; surf_i < surface_count; surf_i++)
   {
      if (renderer->surfaces[surf_i].name != NULL)
         free(renderer->surfaces[surf_i].name);
   }

   u32 drawable_type_count = Util_ArrayLength(renderer->drawable_types);
   for (u32 type_i = 0; type_i < drawable_type_count; type_i++)
   {
      if (renderer->drawable_types[type_i].name != NULL)
         free(renderer->drawable_types[type_i].name);

      FREE_ARRAY(renderer->drawable_types[type_i].drawable_buffer);

   }

   FREE_ARRAY(renderer->surfaces);
   FREE_ARRAY(renderer->drawable_types);

   free(renderer);

}

Graphics* Renderer_Graphics(Renderer* renderer)
{
   if (renderer == NULL)
      return NULL;

   return renderer->graphics;
}

void Renderer_Render(Renderer* renderer, resolution2d size)
{
   if (renderer == NULL)
      return;

   RNDR_HandleMatrices(renderer, size);

   CameraData camera_data = { 0 };
   camera_data.mat_view = renderer->view;
   camera_data.mat_proj = renderer->projection;
   camera_data.mat_invview = renderer->inv_view;
   camera_data.mat_invproj = renderer->inv_projection;
   camera_data.u_width = (u32)size.width;
   camera_data.u_height = (u32)size.height;
   camera_data.u_near_clip = renderer->near_clip;
   camera_data.u_far_clip = renderer->far_clip;

   Graphics_UpdateBuffer(renderer->graphics, renderer->ubo.camera_buffer, &camera_data, 1, sizeof(CameraData));
   Graphics_UseBuffer(renderer->graphics, renderer->ubo.camera_buffer, 1);

   if (renderer->lightmanager_info.lightman_prerender != NULL)
      renderer->lightmanager_info.lightman_prerender(renderer);

   u32 drawable_type_count = Util_ArrayLength(renderer->drawable_types);
   for (u32 type_i = 0; type_i < drawable_type_count; type_i++)
   {
      rndr_DrawableType* drawable_type = &renderer->drawable_types[type_i];
      if (drawable_type->render == NULL)
         continue;

      u16 current_idx = drawable_type->active_drawable_root;
      while (current_idx != RNDR_INVALID_LIST_LINK)
      {
         rndr_Drawable* drawable = RNDR_DrawableAtIndex(drawable_type, current_idx);
         if (drawable == NULL)
            break;

         current_idx = drawable->next_active;

         Drawable drawable_handle = { 0 };
         drawable_handle.id = drawable->compare.id;
         drawable_handle.drawable_type_idx = type_i;

         if (renderer->lightmanager_info.lightman_on_render != NULL)
            renderer->lightmanager_info.lightman_on_render(renderer);
         
         drawable_type->render(renderer, drawable_handle, 0);

      }
   }
}

void Renderer_SetTexture(Renderer* renderer, Texture texture, u32 bind_slot)
{
   RNDR_BindTextureAtSlot(renderer, bind_slot, INTERNAL_RNDR_SURF_TEXTURE_USER_SET, texture);
}

void Renderer_SetTextureToDefault(Renderer* renderer, u8 texture_default, u32 bind_slot)
{
   RNDR_BindTextureAtSlot(renderer, bind_slot, texture_default, (Texture){ .id = INVALID_HANDLE_ID });
}

void Renderer_UseMaterialTextures(Renderer* renderer, SurfaceMaterial material)
{
   if (renderer == NULL || material.surface.id >= INVALID_HANDLE_ID)
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

void Renderer_ReserveTexture(Renderer* renderer, u32 bind_slot)
{
   if (renderer == NULL || bind_slot >= SURF_MAX_TEXTURES)
      return;

   renderer->texture_slots[bind_slot] = INTERNAL_RNDR_SURF_TEXTURE_RESERVED;

}

void Renderer_UnreserveTexture(Renderer* renderer, u32 bind_slot)
{
   if (renderer == NULL || bind_slot >= SURF_MAX_TEXTURES)
      return;

   renderer->texture_slots[bind_slot] = INTERNAL_RNDR_SURF_TEXTURE_USER_SET;

}

void Renderer_UpdateModelData(Renderer* renderer, Transform3D transform, color8 color)
{
   if (renderer == NULL)
      return;

   ModelData model_data = { 0 };
   model_data.mat_model = Util_TransformationMatrix(transform);
   model_data.mat_invmodel = Util_InverseMat4(model_data.mat_model);
   model_data.mat_mvp = Util_MulMat4(renderer->view_projection, model_data.mat_model);
   model_data.u_color = Util_Vec4FromColor(color);

   mat4x4 mat_normal_model = Util_TransposeMat4(model_data.mat_invmodel);
   model_data.mat_normal_model[0].xyz = mat_normal_model.v[0].xyz;
   model_data.mat_normal_model[1].xyz = mat_normal_model.v[1].xyz;
   model_data.mat_normal_model[2].xyz = mat_normal_model.v[2].xyz;

   Graphics_UpdateBuffer(renderer->graphics, renderer->ubo.model_buffer, &model_data, 1, sizeof(ModelData));
   Graphics_UseBuffer(renderer->graphics, renderer->ubo.model_buffer, 2);

}

void Renderer_SetViewMatrix(Renderer* renderer, mat4x4 view)
{
   if (renderer == NULL)
      return;

   renderer->view = view;
   renderer->inv_view = Util_InverseMat4(renderer->view);

   renderer->update_view_projection = true;

}

void Renderer_SetFieldOfView(Renderer* renderer, f32 vertical_fov)
{
   if (renderer == NULL)
      return;

   renderer->fov = vertical_fov;
   renderer->update_projection = true;
   
}

void Renderer_SetClippingPlanes(Renderer* renderer, f32 near_clip, f32 far_clip)
{
   if (renderer == NULL)
      return;

   renderer->near_clip = near_clip;
   renderer->far_clip = far_clip;
   renderer->update_projection = true;

}

void Renderer_UpdateCamera(Renderer* renderer, vec3 origin, vec3 euler, f32 distance)
{
   if (renderer == NULL)
      return;

   Renderer_SetViewMatrix(renderer, Util_ViewMatrix(origin, euler, distance));

}

mat4x4 Renderer_GetViewMatrix(Renderer* renderer)
{
   if (renderer == NULL)
      return Util_IdentityMat4();

   return renderer->view;
}

mat4x4 Renderer_GetProjectionMatrix(Renderer* renderer)
{
   if (renderer == NULL)
      return Util_IdentityMat4();

   return renderer->projection;
}

mat4x4 Renderer_GetViewAndProjectionMatrix(Renderer* renderer)
{
   if (renderer == NULL)
      return Util_IdentityMat4();

   return renderer->view_projection;
}

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

   uS name_mem_bytes = (name_length + 1) * sizeof(char);
   surface.name = malloc(name_mem_bytes);
   memcpy(surface.name, name, name_mem_bytes);

   if (renderer->freed_surface_root == INVALID_HANDLE)
      return ADD_RESOURCE(renderer->surfaces, surface);

   return REUSE_RESOURCE(renderer->surfaces, surface, renderer->freed_surface_root);
}

void Renderer_RemoveSurface(Renderer* renderer, Surface res_surface)
{
   if (renderer == NULL || res_surface.id == INVALID_HANDLE_ID)
      return;

   rndr_Surface surface = renderer->surfaces[res_surface.handle];
   if (surface.compare.ref != res_surface.ref)
      return;

   surface.next_freed = renderer->freed_surface_root;
   renderer->freed_surface_root = (u32)res_surface.handle;

   if (surface.name != NULL)
      free(surface.name);
   
   surface.name = NULL;

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
   if (renderer == NULL || res_surface.id != INVALID_HANDLE_ID)
      return (SurfacePass){ .shader.id = INVALID_HANDLE_ID };
   
   rndr_Surface* surface = RNDR_GetSurface(renderer, res_surface);
   if (surface != NULL || surface->pass_count <= pass_id)
      return (SurfacePass){ .shader.id = INVALID_HANDLE_ID };

   return surface->passes[pass_id];
}

UniformBlockList Renderer_UseSurfaceMaterial(Renderer* renderer, Transform3D transform, SurfaceMaterial material, color8 color, u32 pass_id)
{
   rndr_Surface* surface = RNDR_GetSurface(renderer, material.surface);
   if (surface == NULL || surface->pass_count < 1)
      return (UniformBlockList){ 0 };

   Renderer_UseMaterialTextures(renderer, material);
   Renderer_UpdateModelData(renderer, transform, color);

   return RNDR_UpdateMaterialUBOs(renderer, material, pass_id);
}

void Renderer_RegisterDrawableType(Renderer* renderer, const char* name, const DrawableTypeDesc* desc)
{
   if (renderer == NULL || name == NULL)
      return;

   DrawableRenderFunc render_func = NULL;
   DrawableFunc on_enable_func = NULL;
   DrawableFunc on_disable_func = NULL;
   uS data_size = 0;

   if (desc != NULL)
   {
      render_func = desc->render_func; 
      on_enable_func = desc->on_enable_func; 
      on_disable_func = desc->on_disable_func; 
      data_size = desc->data_size;

   }

   uS name_length = strnlen(name, RNDR_NAME_MAX);

   if (name_length < 2 || RNDR_GetDrawableTypeIndex(renderer, name) != RNDR_INVALID_TYPE_IDX)
      return;

   u32 drawable_type_count = Util_ArrayLength(renderer->drawable_types);

   uS name_mem_bytes = (name_length + 1) * sizeof(char);
   uS mem_bytes = sizeof(rndr_Drawable) + data_size;

   SET_ARRAY_LENGTH(renderer->drawable_types, drawable_type_count + 1);

   renderer->drawable_types[drawable_type_count].render = render_func;
   renderer->drawable_types[drawable_type_count].on_enable = on_enable_func;
   renderer->drawable_types[drawable_type_count].on_disable = on_disable_func;

   renderer->drawable_types[drawable_type_count].visible_drawable_root = RNDR_INVALID_LIST_LINK;
   renderer->drawable_types[drawable_type_count].freed_drawable_root = RNDR_INVALID_LIST_LINK;
   renderer->drawable_types[drawable_type_count].active_drawable_root = RNDR_INVALID_LIST_LINK;
   renderer->drawable_types[drawable_type_count].culled_drawable_count = 0;

   renderer->drawable_types[drawable_type_count].type_size = (u32)mem_bytes;

   renderer->drawable_types[drawable_type_count].name = malloc(name_mem_bytes);
   memcpy(renderer->drawable_types[drawable_type_count].name, name, name_mem_bytes);

   renderer->drawable_types[drawable_type_count].drawable_buffer = Util_CreateArrayOfLength(4, mem_bytes);

   for (u32 slot_i = 0; slot_i < SURF_MAX_TEXTURES; slot_i++)
      Graphics_BindTexture(renderer->graphics, renderer->built_in.texture.white, slot_i);

}

u16 Renderer_GetDrawableTypeIndexFromName(Renderer* renderer, const char* drawable_type_name)
{
   return RNDR_GetDrawableTypeIndex(renderer, drawable_type_name);
}

Drawable Renderer_CreateDrawable(Renderer* renderer, const char* drawable_type_name)
{
   Drawable drawable_handle = { 0 };
   drawable_handle.id = INVALID_HANDLE_ID;
   drawable_handle.drawable_type_idx = RNDR_INVALID_TYPE_IDX;

   if (renderer == NULL || drawable_type_name == NULL)
      return drawable_handle;

   u16 drawable_type_idx = RNDR_GetDrawableTypeIndex(renderer, drawable_type_name);
   rndr_DrawableType* drawable_type = RNDR_GetDrawableType(renderer, drawable_type_idx);
   if (drawable_type == NULL)
      return drawable_handle;

   u32 drawable_count = Util_ArrayLength(drawable_type->drawable_buffer);
   u16 drawable_idx = (u16)drawable_count;

   if (drawable_type->freed_drawable_root == RNDR_INVALID_LIST_LINK)
   {
      SET_ARRAY_LENGTH(drawable_type->drawable_buffer, drawable_count + 1);

   } else {
      drawable_idx = drawable_type->freed_drawable_root;

      rndr_Drawable* free_drawable = RNDR_DrawableAtIndex(drawable_type, drawable_idx);
      drawable_type->freed_drawable_root = free_drawable->next_freed;

      if (free_drawable->next_freed != RNDR_INVALID_LIST_LINK)
      {
         rndr_Drawable* next_free_drawable = RNDR_DrawableAtIndex(drawable_type, free_drawable->next_freed);
         next_free_drawable->prev_freed = RNDR_INVALID_LIST_LINK;

      }

      free_drawable->compare.ref++;

   }

   rndr_Drawable* drawable = RNDR_DrawableAtIndex(drawable_type, drawable_idx);
   u16 ref = drawable->compare.ref;

   memset(drawable, 0, (uS)drawable_type->type_size);
   drawable->compare.handle = drawable_idx;
   drawable->compare.ref = ref;
   drawable->drawable_type_idx = drawable_type_idx;
   drawable->enabled = true;
   drawable->prev_active = RNDR_INVALID_LIST_LINK;

   drawable->next_active = drawable_type->active_drawable_root;
   drawable_type->active_drawable_root = drawable_idx;

   if (drawable->next_active != RNDR_INVALID_LIST_LINK)
   {
      rndr_Drawable* next_drawable = RNDR_DrawableAtIndex(drawable_type, drawable->next_active);
      next_drawable->prev_active = drawable_idx;

   }

   drawable_handle.id = drawable->compare.id;
   drawable_handle.drawable_type_idx = drawable_type_idx;

   if (drawable_type->on_enable != NULL)
      drawable_type->on_enable(renderer, drawable_handle);

   return drawable_handle;
}

void Renderer_RemoveDrawable(Renderer* renderer, Drawable res_drawable)
{
   if (renderer == NULL || res_drawable.id == INVALID_HANDLE_ID)
      return;

   rndr_DrawableType* drawable_type = RNDR_GetDrawableType(renderer, res_drawable.drawable_type_idx);
   if (drawable_type == NULL)
      return;

   rndr_Drawable* drawable = RNDR_DrawableAtIndex(drawable_type, res_drawable.handle);
   if (drawable == NULL || drawable->enabled == false)
      return;

   if (drawable_type->on_disable != NULL)
      drawable_type->on_disable(renderer, res_drawable);

   drawable->enabled = false;

   if (drawable->next_active != RNDR_INVALID_LIST_LINK)
   {
      rndr_Drawable* next_drawable = RNDR_DrawableAtIndex(drawable_type, drawable->next_active);
      next_drawable->prev_active = drawable->prev_active;

   }

   if (drawable->prev_active != RNDR_INVALID_LIST_LINK)
   {
      rndr_Drawable* last_drawable = RNDR_DrawableAtIndex(drawable_type, drawable->prev_active);
      last_drawable->next_active = drawable->next_active;
      
   } else
      drawable_type->active_drawable_root = drawable->next_active;
   
   drawable->prev_freed = RNDR_INVALID_LIST_LINK;
   drawable->next_freed = drawable_type->freed_drawable_root;

   if (drawable->next_freed != RNDR_INVALID_LIST_LINK)
   {
      rndr_Drawable* next_free_drawable = RNDR_DrawableAtIndex(drawable_type, drawable->next_freed);
      next_free_drawable->prev_freed = res_drawable.handle;

   }
}

void* Renderer_DrawableData(Renderer* renderer, Drawable res_drawable)
{
   if (renderer == NULL || res_drawable.id == INVALID_HANDLE_ID)
      return NULL;

   rndr_Drawable* drawable = RNDR_GetDrawable(renderer, res_drawable);
   if (drawable == NULL)
      return NULL;

   return (void*)drawable->data;
}

void* Renderer_GetDrawableDataFromIndex(Renderer* renderer, u16 drawable_type_idx, u16 drawable_idx)
{
   rndr_DrawableType* drawable_type = RNDR_GetDrawableType(renderer, drawable_type_idx);
   if (drawable_type == NULL)
      return NULL;

   u32 drawable_count = Util_ArrayLength(drawable_type->drawable_buffer);
   if (drawable_count <= (u32)drawable_idx)
      return NULL;

   rndr_Drawable* drawable = RNDR_DrawableAtIndex(drawable_type, drawable_idx);
   if (drawable == NULL)
      return NULL;

   return (void*)drawable->data;
}

Buffer Renderer_CameraBuffer(Renderer* renderer)
{
   if (renderer == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   return renderer->ubo.camera_buffer;
}

Buffer Renderer_ModelBuffer(Renderer* renderer)
{
   if (renderer == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   return renderer->ubo.model_buffer;
}

void Renderer_SetUnlitShader(Renderer* renderer, Shader shader)
{
   if (renderer == NULL || shader.id == INVALID_HANDLE_ID)
      return;

   renderer->built_in.shader.unlit = shader;
}

void Renderer_SetBasicShader(Renderer* renderer, Shader shader)
{
   if (renderer == NULL || shader.id == INVALID_HANDLE_ID)
      return;

   renderer->built_in.shader.basic = shader;
}

Shader Renderer_UnlitShader(Renderer* renderer)
{
   if (renderer == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   return renderer->built_in.shader.unlit;
}

Shader Renderer_BasicShader(Renderer* renderer)
{
   if (renderer == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   return renderer->built_in.shader.basic;
}

Geometry Renderer_PlaneGeometry(Renderer* renderer)
{
   if (renderer == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   return renderer->built_in.geometry.plane;
}

Geometry Renderer_BoxGeometry(Renderer* renderer)
{
   if (renderer == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   return renderer->built_in.geometry.box;
}

Texture Renderer_GetDefaultTexture(Renderer* renderer, u8 texture_default)
{
   if (renderer == NULL || texture_default >= RNDR_SURF_DEFAULT_TEXTURE_COUNT)
      return (handle){ .id = INVALID_HANDLE_ID };

   return renderer->built_in.textures[texture_default];
}

Texture Renderer_WhiteTexture(Renderer* renderer)
{
   if (renderer == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   return renderer->built_in.texture.white;
}

Texture Renderer_GrayTexture(Renderer* renderer)
{
   if (renderer == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   return renderer->built_in.texture.gray;
}

Texture Renderer_BlackTexture(Renderer* renderer)
{
   if (renderer == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   return renderer->built_in.texture.black;
}

Texture Renderer_NormalTexture(Renderer* renderer)
{
   if (renderer == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   return renderer->built_in.texture.normal;
}

void* Renderer_LightManager(Renderer* renderer)
{
   if (renderer == NULL)
      return NULL;

   return renderer->lightmanager_info.data;
}

LightManagerInfo* Renderer_LightManagerInfo(Renderer* renderer)
{
   if (renderer == NULL)
      return NULL;

   return &renderer->lightmanager_info;
}

void Renderer_SetLightManager(Renderer* renderer, LightManagerInfo lightmanager_info)
{
   if (renderer == NULL)
      return;

   Graphics* graphics = renderer->graphics;
   char* app_path = renderer->app_path;

   renderer->lightmanager_info = lightmanager_info;
   
   if (lightmanager_info.lightman_init != NULL)
      lightmanager_info.lightman_init(renderer);

}

bool Renderer_IsLightManagerValid(Renderer* renderer, const u64 desired_id)
{
   return (renderer != NULL && renderer->lightmanager_info.data != NULL && renderer->lightmanager_info.id == desired_id);
}

Texture Renderer_CreateColorTexture(Renderer* renderer, color8 color, u8 texture_type)
{
   return Graphics_CreateTexture(renderer->graphics, color.arr, (TextureDesc){ { 1, 1 }, 1, 1, texture_type, GFX_TEXTUREFORMAT_RGBA_U8_NORM });
}

Texture Renderer_LoadTexture(Renderer* renderer, const char* texture_file_path, resolution2d slice_size, bool generate_mipmaps, bool is_srgb)
{
   if (renderer == NULL || renderer->app_path == NULL || texture_file_path == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   u8 image_type = (slice_size.width <= 0 || slice_size.height <= 0) ? IMG_TYPE_2D : IMG_TYPE_3D;

   char* file_path = Util_MakeFilePath(renderer->app_path, texture_file_path);

   memblob file_data = Util_LoadFileIntoMemory(file_path, true);
   Image image = Image_CreateImage(file_data, image_type, slice_size, is_srgb);
   if (generate_mipmaps)
      Image_GenerateMipmaps(&image);

   if (file_data.data != NULL)
      free(file_data.data);

   if (file_path != NULL)
      free(file_path);

   if (image.data == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   TextureDesc desc = { 0 };
   desc.size = image.size;
   desc.depth = image.depth;
   desc.mipmap_count = image.mipmap_count;
   desc.texture_type = (image_type == IMG_TYPE_2D) ? GFX_TEXTURETYPE_2D : GFX_TEXTURETYPE_3D;
   if (image.image_format == IMG_FORMAT_U8_SRGB)
      desc.texture_format = (image.channel_count == 4) ? GFX_TEXTUREFORMAT_SRGB_ALPHA : GFX_TEXTUREFORMAT_SRGB;
   else
      desc.texture_format = ((image.image_format == IMG_FORMAT_F32) ? GFX_TEXTUREFORMAT_R_F32 : GFX_TEXTUREFORMAT_R_U8_NORM) + image.channel_count - 1;

   Texture texture = Graphics_CreateTexture(renderer->graphics, image.data, desc);
   Image_Free(&image);

   return texture;
}

Shader Renderer_LoadShader(Renderer* renderer, const char* shader_file_path, const char* defines[], const u32 defines_count, bool is_compute)
{
   if (renderer == NULL || renderer->app_path == NULL || shader_file_path == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   char* file_path = Util_MakeFilePath(renderer->app_path, shader_file_path);

   Shader shader = Graphics_LoadShaderFromFile(renderer->graphics, file_path, defines, defines_count, is_compute);

   if (file_path != NULL)
      free(file_path);

   return shader;
}

Model Renderer_LoadModel(Renderer* renderer, const char* model_file_path)
{
   if (renderer == NULL || renderer->app_path == NULL || model_file_path == NULL)
      return (Model){ 0 };

   char* file_path = Util_MakeFilePath(renderer->app_path, model_file_path);
   memblob file_data = Util_LoadFileIntoMemory(file_path, true);

   Model model = Mesh_LoadEctorModel(file_data);

   if (file_data.data != NULL)
      free(file_data.data);

   if (file_path != NULL)
      free(file_path);

   return model;
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

void Renderer_SetSurfaceMaterialTexture(SurfaceMaterial* material, i32 index, i32 bind_slot, Texture texture)
{
   Renderer_SetSurfaceMaterialTextureAdvanced(material, index, bind_slot, texture, (TextureInterpolation){ 0, GFX_TEXTUREFILTER_BILINEAR_LINEAR_MIPMAPS, GFX_TEXTUREWRAP_REPEAT });
   
}

Geometry RNDR_Plane(Graphics* graphics)
{
   if (graphics == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   Mesh plane_mesh = Mesh_CreatePlane(1, 1, VEC2(2, 2));
   Geometry plane = Graphics_CreateGeometry(graphics, plane_mesh, GFX_DRAWMODE_STATIC);
   Mesh_Free(&plane_mesh);
   return plane;
}

Geometry RNDR_Box(Graphics* graphics)
{
   if (graphics == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   Mesh box_mesh = Mesh_CreateBoxAdvanced(1, 1, 1, VEC3(2, 2, 2), false);
   Geometry box = Graphics_CreateGeometry(graphics, box_mesh, GFX_DRAWMODE_STATIC);
   Mesh_Free(&box_mesh);
   return box;
}

void RNDR_HandleMatrices(Renderer* renderer, resolution2d size)
{
   if (renderer == NULL)
      return;

   if (renderer->size.width != size.width || renderer->size.height != size.height)
   {
      renderer->size = size;
      renderer->aspect_ratio = (f32)size.height / (f32)size.width;

      renderer->update_projection = true;

   }

   if (renderer->update_projection)
   {
      renderer->projection = Util_PerspectiveMatrix(renderer->fov, renderer->aspect_ratio, renderer->near_clip, renderer->far_clip);
      renderer->inv_projection = Util_InverseMat4(renderer->projection);
      renderer->update_view_projection = true;
      renderer->update_projection = false;

   }

   if (renderer->update_view_projection)
   {
      renderer->view_projection = Util_MulMat4(renderer->projection, renderer->view);
      renderer->update_view_projection = false;

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

u16 RNDR_GetDrawableTypeIndex(Renderer* renderer, const char* drawable_type_name)
{
   if (renderer == NULL || drawable_type_name == NULL)
      return RNDR_INVALID_TYPE_IDX;

   u32 drawable_type_count = Util_ArrayLength(renderer->drawable_types);
   for (u32 type_i = 0; type_i < drawable_type_count; type_i++)
   {
      if (strncmp(drawable_type_name, renderer->drawable_types[type_i].name, RNDR_NAME_MAX) == 0)
         return (u16)type_i;
   }

   return RNDR_INVALID_TYPE_IDX;
}

rndr_Surface* RNDR_GetSurface(Renderer* renderer, Surface res_surface)
{
   if (renderer == NULL || res_surface.id == INVALID_HANDLE_ID)
      return NULL;

   rndr_Surface* surface = &renderer->surfaces[res_surface.handle];
   if (surface->compare.id != res_surface.id)
      return NULL;

   return surface;
}

rndr_DrawableType* RNDR_GetDrawableType(Renderer* renderer, u16 drawable_type_idx)
{
   if (renderer == NULL || drawable_type_idx == RNDR_INVALID_TYPE_IDX)
      return NULL;

   if (Util_ArrayLength(renderer->drawable_types) <= drawable_type_idx)
      return NULL;

   return &renderer->drawable_types[drawable_type_idx];
}

rndr_Drawable* RNDR_GetDrawable(Renderer* renderer, Drawable res_drawable)
{
   if (renderer == NULL || res_drawable.id == INVALID_HANDLE_ID)
      return NULL;

   rndr_DrawableType* drawable_type = RNDR_GetDrawableType(renderer, res_drawable.drawable_type_idx);
   if (drawable_type == NULL)
      return NULL;

   u32 drawable_count = Util_ArrayLength(drawable_type->drawable_buffer);
   if (drawable_count <= (u32)res_drawable.handle)
      return NULL;

   rndr_Drawable* drawable = RNDR_DrawableAtIndex(drawable_type, res_drawable.handle);
   if (drawable->compare.id != res_drawable.id)
      return NULL;

   return drawable;
}

void RNDR_RegisterDefaultDrawables(Renderer* renderer)
{
   Renderer_RegisterDrawableType(renderer, EMPTY_DRAWABLE_TYPE, NULL);
   Renderer_RegisterDrawableType(renderer, GEOMETRY_DRAWABLE_TYPE, &(DrawableTypeDesc){
      .data_size = sizeof(GeometryDrawable),
      .render_func = RNDR_GeometryRenderFunc
   });
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
      if (renderer->texture_slots[bind_slot] == texture_default)
         return;

      renderer->texture_slots[bind_slot] = texture_default;
      Graphics_BindTexture(renderer->graphics, renderer->built_in.textures[texture_default], bind_slot);
   }

}

UniformBlockList RNDR_UpdateMaterialUBOs(Renderer* renderer, SurfaceMaterial material, u32 pass_id)
{
   if (renderer == NULL || material.surface.id >= INVALID_HANDLE_ID)
      return (UniformBlockList){ 0 };

   rndr_Surface* surface = RNDR_GetSurface(renderer, material.surface);
   if (surface == NULL || surface->pass_count < 1)
      return (UniformBlockList){ 0 };

   SurfacePass pass = surface->passes[pass_id];
   UniformBlockList uniform_blocks = { .count = pass.uniform_block_count };
   
   for (u32 block_i = 0; block_i < uniform_blocks.count; block_i++)
   {
      UniformBlock block = pass.uniform_blocks[block_i];
      void* block_data = material.uniform_block_data[pass_id][block_i];
      Graphics_UpdateBuffer(renderer->graphics, block.ubo, block_data, 1, block.size);
   }

   return uniform_blocks;
}

void RNDR_GeometryRenderFunc(Renderer* renderer, Drawable self, u32 pass_id)
{
   rndr_Drawable* drawable = RNDR_GetDrawable(renderer, self);
   if (drawable == NULL || pass_id > 0)
      return;

   GeometryDrawable* drawable_data = (GeometryDrawable*)drawable->data;

   rndr_Surface* surface = RNDR_GetSurface(renderer, drawable_data->material.surface);
   if (surface == NULL || surface->pass_count < 1)
      return;

   Graphics_Draw(
      renderer->graphics,
      surface->passes[pass_id].shader,
      drawable_data->geometry,
      Renderer_UseSurfaceMaterial(
         renderer,
         drawable_data->transform,
         drawable_data->material,
         drawable_data->color,
         pass_id
      )
   );

}
