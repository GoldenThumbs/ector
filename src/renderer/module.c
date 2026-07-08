#include "util/types.h"
#include "util/extra_types.h"
#include "util/array.h"
#include "util/files.h"
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
#include <assert.h>

Renderer* Renderer_Init(Graphics* graphics, const char* app_path)
{
   if (graphics == NULL)
      return NULL;

   Renderer* renderer = malloc(sizeof(Renderer));
   if (renderer == NULL)
      return NULL;

   renderer->app_path = app_path;
   renderer->graphics = graphics;

   renderer->surfaces = NEW_ARRAY_N(rndr_Surface, 16);
   renderer->drawable_types = NEW_ARRAY_N(rndr_DrawableType, 8);
   renderer->textures = NEW_MAP_N(Texture, 4);

   renderer->lightmanager_info = (LightManagerInfo){ 0 };

   renderer->freed_surface_root = RNDR_INVALID_LIST_LINK;

   renderer->built_in.texture.white = Renderer_CreateColorTexture(renderer, Util_IntToColor(0XFFFFFFFF), GFX_TEXTURETYPE_2D);
   renderer->built_in.texture.black = Renderer_CreateColorTexture(renderer, Util_IntToColor(0x000000FF), GFX_TEXTURETYPE_2D);
   renderer->built_in.texture.gray = Renderer_CreateColorTexture(renderer, (color8){ 128, 128, 128, 255 }, GFX_TEXTURETYPE_2D);
   renderer->built_in.texture.normal = Renderer_CreateColorTexture(renderer, (color8){ 127, 127, 255, 255 }, GFX_TEXTURETYPE_2D);

   renderer->built_in.geometry.plane = RNDR_CreateDefaultPlane(graphics);
   renderer->built_in.geometry.box = RNDR_CreateDefaultBox(graphics);

   renderer->built_in.shader.unlit.id = INVALID_HANDLE_ID;
   renderer->built_in.shader.basic.id = INVALID_HANDLE_ID;

   renderer->ubo.camera_buffer = Graphics_CreateBufferExplicit(
      renderer->graphics, NULL, sizeof(CameraData), GFX_DRAWMODE_DYNAMIC, GFX_BUFFERTYPE_UNIFORM);
   renderer->ubo.model_buffer = Graphics_CreateBufferExplicit(
      renderer->graphics, NULL, sizeof(ModelData), GFX_DRAWMODE_DYNAMIC, GFX_BUFFERTYPE_UNIFORM);

   Graphics_CheckErrors(graphics);

   renderer->near_clip = 0.05f;
   renderer->far_clip = 100.0f;
   renderer->fov = 40.0f;
   renderer->aspect_ratio = 1.0f;
   renderer->size = (res2D){ 1, 1 };
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

   if (renderer->lightmanager_info.lightman_free != NULL)
      renderer->lightmanager_info.lightman_free(renderer);

   for (u32 type_i = 0; type_i < Util_ArrayLength(renderer->drawable_types); type_i++)
      FREE_ARRAY(renderer->drawable_types[type_i].drawable_buffer);

   FREE_ARRAY(renderer->surfaces);
   FREE_ARRAY(renderer->drawable_types);
   FREE_MAP(renderer->textures);

   free(renderer);

}

Graphics* Renderer_GetGraphics(Renderer* renderer)
{
   if (renderer == NULL)
      return NULL;

   return renderer->graphics;
}

void Renderer_PreRender(Renderer* renderer)
{
   if (renderer == NULL)
      return;

   if (renderer->lightmanager_info.lightman_prerender != NULL)
      renderer->lightmanager_info.lightman_prerender(renderer, 0);

}

void Renderer_RenderPass(Renderer* renderer, res2D size, f64 engine_frame_delta, u32 pass_id)
{
   if (renderer == NULL)
      return;

   renderer->frame_delta = (f32)engine_frame_delta;

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
   Graphics_BindBuffer(renderer->graphics, renderer->ubo.camera_buffer, 1);

   if (renderer->lightmanager_info.lightman_on_render != NULL)
      renderer->lightmanager_info.lightman_on_render(renderer, pass_id);

   u32 drawable_type_count = Util_ArrayLength(renderer->drawable_types);
   for (u32 type_i = 0; type_i < drawable_type_count; type_i++)
   {
      rndr_DrawableType* drawable_type = &renderer->drawable_types[type_i];
      if (drawable_type->render == NULL)
         continue;

      u32 drawable_count = Util_ArrayLength(drawable_type->drawable_buffer);

      for (u32 drawable_i = 0; drawable_i < drawable_count; drawable_i++)
      {
         rndr_Drawable* drawable = RNDR_DrawableAtIndex(drawable_type, drawable_i);
         if (drawable == NULL || !drawable->enabled || drawable->next_freed != INVALID_HANDLE)
            continue;

         Drawable drawable_handle = { 0 };
         drawable_handle.id = drawable->compare.id;
         drawable_handle.drawable_type_idx = type_i;

         drawable_type->render(renderer, drawable_handle, pass_id);

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
   Graphics_BindBuffer(renderer->graphics, renderer->ubo.model_buffer, 2);

}

void Renderer_SetViewMatrix(Renderer* renderer, mat4x4 view)
{
   if (renderer == NULL)
      return;

   renderer->view = view;
   renderer->inv_view = Util_InverseMat4(renderer->view);

   renderer->update_view_projection = true;

}

void Renderer_SetProjectionMatrix(Renderer* renderer, mat4x4 proj)
{
   if (renderer == NULL)
      return;

   renderer->projection = proj;
   renderer->inv_projection = Util_InverseMat4(renderer->projection);

   renderer->user_supplied_projection = true;
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

f32 Renderer_GetFieldOfView(Renderer* renderer)
{
   if (renderer == NULL)
      return 0.0f;

   return renderer->fov;
}

f32 Renderer_GetNearClippingPlane(Renderer* renderer)
{
   if (renderer == NULL)
      return 0.0f;

   return renderer->near_clip;
}

f32 Renderer_GetFarClippingPlane(Renderer* renderer)
{
   if (renderer == NULL)
      return 0.0f;

   return renderer->far_clip;
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

f32 Renderer_GetFrameDelta(Renderer* renderer)
{
   if (renderer == NULL)
      return 1.0f;

   return renderer->frame_delta;
}

Buffer Renderer_GetCameraBuffer(Renderer* renderer)
{
   if (renderer == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   return renderer->ubo.camera_buffer;
}

Buffer Renderer_GetModelBuffer(Renderer* renderer)
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

LightManagerInfo* Renderer_LightManagerInfo(Renderer* renderer)
{
   if (renderer == NULL)
      return NULL;

   return &renderer->lightmanager_info;
}

void* Renderer_GetLightManagerData(Renderer* renderer)
{
   if (renderer == NULL)
      return NULL;

   return renderer->lightmanager_info.data;
}

void Renderer_SetLightManager(Renderer* renderer, LightManagerInfo lightmanager_info)
{
   if (renderer == NULL)
      return;

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

Texture Renderer_LoadTexture(Renderer* renderer, const char* texture_file_path, res2D slice_size, bool generate_mipmaps, bool is_srgb)
{
   if (renderer == NULL || texture_file_path == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   Texture* texture_ptr = GET_MAP_ITEM(renderer->textures, texture_file_path);
   if (texture_ptr == NULL)
   {
      Texture texture = RNDR_LoadTexture(renderer, texture_file_path, slice_size, generate_mipmaps, is_srgb);
      texture_ptr = ADD_MAP_ITEM(renderer->textures, texture_file_path, texture);

   }

   if (texture_ptr == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   return (*texture_ptr);
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
   char* mat_path = Util_ReplaceFileExtension(file_path, ".mat");
   memblob file_data = Util_LoadFileIntoMemory(file_path, true);
   memblob mat_data = Util_LoadFileIntoMemory(mat_path, false);

   Model model = Mesh_LoadEctorModel(file_data);
   Mesh_ParseEctorMaterials(mat_data, &model);

   if (file_data.data != NULL)
      free(file_data.data);

   if (file_path != NULL)
      free(file_path);

   if (mat_path != NULL)
      free(mat_path);

   if (mat_data.data != NULL)
      free(mat_data.data);

   return model;
}

Texture RNDR_LoadTexture(Renderer* renderer, const char* texture_file_path, res2D slice_size, bool generate_mipmaps, bool is_srgb)
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
   desc.size = image.size.width_height;
   desc.depth = image.size.depth;
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

Geometry RNDR_CreateDefaultPlane(Graphics* graphics)
{
   if (graphics == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   Mesh plane_mesh = Mesh_CreatePlane(1, 1, VEC2(2, 2));
   Geometry plane = Graphics_CreateGeometry(graphics, plane_mesh, GFX_DRAWMODE_STATIC);
   Mesh_Free(&plane_mesh);

   return plane;
}

Geometry RNDR_CreateDefaultBox(Graphics* graphics)
{
   if (graphics == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   Mesh box_mesh = Mesh_CreateBoxAdvanced(1, 1, 1, VEC3(2, 2, 2), false);
   Geometry box = Graphics_CreateGeometry(graphics, box_mesh, GFX_DRAWMODE_STATIC);
   Mesh_Free(&box_mesh);

   return box;
}

void RNDR_HandleMatrices(Renderer* renderer, res2D size)
{
   if (renderer == NULL)
      return;

   if (renderer->size.width != size.width || renderer->size.height != size.height)
   {
      renderer->size = size;
      renderer->aspect_ratio = (f32)size.height / (f32)size.width;

      renderer->update_projection = true;

   }

   if (renderer->update_projection && !renderer->user_supplied_projection)
   {
      renderer->projection = Util_PerspectiveMatrix(renderer->fov, renderer->aspect_ratio, renderer->near_clip, renderer->far_clip);
      renderer->inv_projection = Util_InverseMat4(renderer->projection);
      renderer->update_view_projection = true;
      renderer->update_projection = false;

   } else
      renderer->user_supplied_projection = false; // user must supply their custom projection matrix every frame

   if (renderer->update_view_projection)
   {
      renderer->view_projection = Util_MulMat4(renderer->projection, renderer->view);
      renderer->update_view_projection = false;

   }

}

void RNDR_GeometryOnCreateFunc(Renderer* renderer, Drawable self)
{
   rndr_Drawable* drawable = RNDR_GetDrawable(renderer, self);
   GeometryDrawable* drawable_data = (GeometryDrawable*)drawable->data;

   if (drawable_data == NULL)
      return;

   drawable_data->color.hex = 0xFFFFFFFF;
   drawable_data->transform = Util_IdentityTransform();

}

void RNDR_GeometryRenderFunc(Renderer* renderer, Drawable self, u32 pass_id)
{
   rndr_Drawable* drawable = RNDR_GetDrawable(renderer, self);
   GeometryDrawable* drawable_data = (GeometryDrawable*)drawable->data;

   if (drawable_data == NULL)
      return;

   rndr_Surface* surface = RNDR_GetSurface(renderer, drawable_data->material.surface);
   if (surface == NULL || surface->pass_count < pass_id + 1)
      return;

   SurfacePass pass = surface->passes[pass_id];
   Graphics_SetBlending(renderer->graphics, pass.blend_mode);
   Graphics_SetGeometryFaceCullMode(renderer->graphics, drawable_data->geometry, pass.cull_mode);
   Graphics_SetDepthTest(renderer->graphics, pass.depth_mode);

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
