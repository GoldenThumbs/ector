#include "util/extra_types.h"
#include "util/matrix.h"
#include "util/types.h"
#include "util/array.h"
// #include "util/extra_types.h"
#include "graphics.h"

#include "renderer/default_shaders.h"

#include "renderer.h"
#include "renderer/internal.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

Renderer* Renderer_Init(Graphics* graphics)
{
   if (graphics == NULL)
      return NULL;
   
   Renderer* renderer = malloc(sizeof(Renderer));
   if (renderer == NULL)
      return NULL;
   
   renderer->graphics = graphics;
   
   renderer->point_lights = NEW_ARRAY_N(rndr_PointLightSource, 16);
   renderer->drawable_types = NEW_ARRAY_N(rndr_DrawableType, 8);
   
   renderer->built_in.texture.white = RNDR_LoadColorTexture(renderer->graphics, (color8){ .hex = 0XFFFFFFFF }, GFX_TEXTURETYPE_2D);
   renderer->built_in.texture.black = RNDR_LoadColorTexture(renderer->graphics, (color8){ .hex = 0xFF000000 }, GFX_TEXTURETYPE_2D);
   renderer->built_in.texture.gray = RNDR_LoadColorTexture(renderer->graphics, (color8){ .hex = 0xFF888888 }, GFX_TEXTURETYPE_2D);
   renderer->built_in.texture.normal = RNDR_LoadColorTexture(renderer->graphics, (color8){ .hex = 0xFFFF8888 }, GFX_TEXTURETYPE_2D);
   
   renderer->built_in.geometry.plane = RNDR_Plane(graphics);
   renderer->built_in.geometry.box = RNDR_Box(graphics);
   
   renderer->built_in.shader.basic = Graphics_CreateShader(graphics, builtin_vertex_code, builtin_fragment_code);
   
   renderer->cluster_dimensions.x = RNDR_CLUSTER_X;
   renderer->cluster_dimensions.y = RNDR_CLUSTER_Y;
   renderer->cluster_dimensions.z = RNDR_CLUSTER_Z;
   
   u32 xy_cluster_count = renderer->cluster_dimensions.x * renderer->cluster_dimensions.y;
   renderer->cluster_dimensions.total_clusters = xy_cluster_count * renderer->cluster_dimensions.z;
   renderer->clusters = calloc((uS)renderer->cluster_dimensions.total_clusters, sizeof(rndr_Cluster));
   
   for (u32 z_i = 0; z_i < renderer->cluster_dimensions.z; z_i++)
      for (u32 y_i = 0; y_i < renderer->cluster_dimensions.y; y_i++)
         for (u32 x_i = 0; x_i < renderer->cluster_dimensions.x; x_i++)
   {
      u32 cluster_idx = z_i * xy_cluster_count + y_i * renderer->cluster_dimensions.x + x_i;
      renderer->clusters[cluster_idx].frustum_idx[0] = x_i;
      renderer->clusters[cluster_idx].frustum_idx[1] = y_i;
      renderer->clusters[cluster_idx].frustum_idx[2] = z_i;
   }

   renderer->ubo.camera_data = Graphics_CreateBufferExplicit(
      renderer->graphics, NULL, sizeof(CameraData), GFX_DRAWMODE_DYNAMIC, GFX_BUFFERTYPE_UNIFORM);
   renderer->ubo.model_data = Graphics_CreateBufferExplicit(
      renderer->graphics, NULL, sizeof(ModelData), GFX_DRAWMODE_DYNAMIC, GFX_BUFFERTYPE_UNIFORM);

   renderer->ssbo.light_buffer = Graphics_CreateBuffer(
      renderer->graphics, NULL, 0, sizeof(rndr_PointLightSource), GFX_DRAWMODE_DYNAMIC, GFX_BUFFERTYPE_STORAGE);
   renderer->ssbo.cluster_buffer = Graphics_CreateBufferExplicit(
      renderer->graphics, NULL, sizeof(renderer->cluster_dimensions) + sizeof(rndr_Cluster) * renderer->cluster_dimensions.total_clusters, GFX_DRAWMODE_STATIC, GFX_BUFFERTYPE_STORAGE);

   renderer->view = Util_IdentityMat4();
   renderer->projection = Util_IdentityMat4();
   renderer->view_projection = Util_IdentityMat4();

   RNDR_RegisterDefaultDrawables(renderer);
    
   return renderer;
}

void Renderer_Free(Renderer* renderer)
{
   if (renderer == NULL)
      return;

   u32 drawable_type_count = Util_ArrayLength(renderer->drawable_types);
   for (u32 type_i = 0; type_i < drawable_type_count; type_i++)
   {
      if (renderer->drawable_types[type_i].name != NULL)
         free(renderer->drawable_types[type_i].name);

      FREE_ARRAY(renderer->drawable_types[type_i].drawable_buffer);

   }

   FREE_ARRAY(renderer->point_lights);
   FREE_ARRAY(renderer->drawable_types);
   free(renderer->clusters);

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

   CameraData camera_data = { 0 };
   camera_data.mat_view = renderer->view;
   camera_data.mat_proj = renderer->projection;
   camera_data.mat_invview = Util_InverseViewMatrix(renderer->view);
   camera_data.mat_invproj = Util_InversePerspectiveMatrix(renderer->projection);
   camera_data.u_width = (u32)size.width;
   camera_data.u_height = (u32)size.height;

   Graphics_UpdateBuffer(renderer->graphics, renderer->ubo.camera_data, &camera_data, 1, sizeof(CameraData));

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
         
         drawable_type->render(renderer, drawable_handle);

      }
   }
}

void Renderer_SetViewMatrix(Renderer* renderer, mat4x4 view)
{
   if (renderer == NULL)
      return;

   Renderer_SetViewAndProjectionMatrix(renderer, view, renderer->projection);
}

void Renderer_SetProjectionMatrix(Renderer* renderer, mat4x4 projection)
{
   if (renderer == NULL)
      return;

   Renderer_SetViewAndProjectionMatrix(renderer, renderer->view, projection);
}

void Renderer_SetViewAndProjectionMatrix(Renderer* renderer, mat4x4 view, mat4x4 projection)
{
   if (renderer == NULL)
      return;

   renderer->view = view;
   renderer->projection = projection;
   renderer->view_projection = Util_MulMat4(projection, view);
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



void Renderer_RegisterDrawableType(Renderer* renderer, const char* name, const DrawableTypeDesc* desc)
{
   if (renderer == NULL || name == NULL)
      return;

   DrawableFunc render_func = NULL;
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
   {
      return;
   }

   u32 drawable_type_count = Util_ArrayLength(renderer->drawable_types);

   uS name_mem_bytes = (name_length + 1) * sizeof(char);
   uS mem_bytes = sizeof(rndr_Drawable) + data_size;

   SET_ARRAY_LENGTH(renderer->drawable_types, drawable_type_count + 1);

   renderer->drawable_types[drawable_type_count].render = render_func;
   renderer->drawable_types[drawable_type_count].on_enable = on_enable_func;
   renderer->drawable_types[drawable_type_count].on_disable = on_disable_func;

   renderer->drawable_types[drawable_type_count].visible_drawable_root = RNDR_INVALID_LIST_LINK;
   renderer->drawable_types[drawable_type_count].free_drawable_root = RNDR_INVALID_LIST_LINK;
   renderer->drawable_types[drawable_type_count].active_drawable_root = RNDR_INVALID_LIST_LINK;
   renderer->drawable_types[drawable_type_count].culled_drawable_count = 0;

   renderer->drawable_types[drawable_type_count].type_size = (u32)mem_bytes;

   renderer->drawable_types[drawable_type_count].name = malloc(name_mem_bytes);
   memcpy(renderer->drawable_types[drawable_type_count].name, name, name_mem_bytes);

   renderer->drawable_types[drawable_type_count].drawable_buffer = Util_CreateArrayOfLength(4, mem_bytes);

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

   if (drawable_type->free_drawable_root == RNDR_INVALID_LIST_LINK)
   {
      SET_ARRAY_LENGTH(drawable_type->drawable_buffer, drawable_count + 1);

   } else {
      drawable_idx = drawable_type->free_drawable_root;

      rndr_Drawable* free_drawable = RNDR_DrawableAtIndex(drawable_type, drawable_idx);
      drawable_type->free_drawable_root = free_drawable->next_freed;

      if (free_drawable->next_freed != RNDR_INVALID_LIST_LINK)
      {
         rndr_Drawable* next_free_drawable = RNDR_DrawableAtIndex(drawable_type, free_drawable->next_freed);
         next_free_drawable->last_freed = RNDR_INVALID_LIST_LINK;

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
   drawable->last_active = RNDR_INVALID_LIST_LINK;

   drawable->next_active = drawable_type->active_drawable_root;
   drawable_type->active_drawable_root = drawable_idx;

   if (drawable->next_active != RNDR_INVALID_LIST_LINK)
   {
      rndr_Drawable* next_drawable = RNDR_DrawableAtIndex(drawable_type, drawable->next_active);
      next_drawable->last_active = drawable_idx;

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
      next_drawable->last_active = drawable->last_active;

   }

   if (drawable->last_active != RNDR_INVALID_LIST_LINK)
   {
      rndr_Drawable* last_drawable = RNDR_DrawableAtIndex(drawable_type, drawable->last_active);
      last_drawable->next_active = drawable->next_active;
      
   } else
      drawable_type->active_drawable_root = drawable->next_active;
   
   drawable->last_freed = RNDR_INVALID_LIST_LINK;
   drawable->next_freed = drawable_type->free_drawable_root;

   if (drawable->next_freed != RNDR_INVALID_LIST_LINK)
   {
      rndr_Drawable* next_free_drawable = RNDR_DrawableAtIndex(drawable_type, drawable->next_freed);
      next_free_drawable->last_freed = res_drawable.handle;

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

Buffer Renderer_CameraBuffer(Renderer* renderer)
{
   if (renderer == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   return renderer->ubo.camera_data;
}

Buffer Renderer_ModelBuffer(Renderer* renderer)
{
   if (renderer == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   return renderer->ubo.model_data;
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


Texture RNDR_LoadColorTexture(Graphics* graphics, color8 color, u8 texture_type)
{
   return Graphics_CreateTexture(graphics, color.arr, (TextureDesc){ { 1, 1 }, 1, 1, texture_type, GFX_TEXTUREFORMAT_RGBA_U8_NORM });
}

Geometry RNDR_Plane(Graphics* graphics)
{
   Mesh plane_mesh = Mesh_CreatePlane(1, 1, VEC2(2, 2));
   Geometry plane = Graphics_CreateGeometry(graphics, plane_mesh, GFX_DRAWMODE_STATIC);
   Mesh_Free(&plane_mesh);
   return plane;
}

Geometry RNDR_Box(Graphics* graphics)
{
   Mesh box_mesh = Mesh_CreateBoxAdvanced(1, 1, 1, VEC3(2, 2, 2), false);
   Geometry box = Graphics_CreateGeometry(graphics, box_mesh, GFX_DRAWMODE_STATIC);
   Mesh_Free(&box_mesh);
   return box;
}

u16 RNDR_GetDrawableTypeIndex(Renderer* renderer, const char* drawable_type_name)
{
   if (renderer == NULL || drawable_type_name == NULL)
      return RNDR_INVALID_TYPE_IDX;

   u32 drawable_type_count = Util_ArrayLength(renderer->drawable_types);
   for (u32 type_i = 0; type_i < drawable_type_count; type_i++)
   {
      if (strncmp(drawable_type_name, renderer->drawable_types[type_i].name, RNDR_NAME_MAX) == 0)
         return type_i;
   }

   return RNDR_INVALID_TYPE_IDX;
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

void RNDR_GeometryRenderFunc(Renderer* renderer, Drawable self)
{
   rndr_Drawable* drawable = RNDR_GetDrawable(renderer, self);
   if (drawable == NULL)
      return;

   GeometryDrawable* drawable_data = (GeometryDrawable*)drawable->data;

   ModelData model_data = { 0 };
   model_data.mat_model = Util_TransformationMatrix(drawable_data->transform);
   model_data.mat_invmodel = Util_InverseMat4(model_data.mat_model);
   model_data.mat_mvp = Util_MulMat4(renderer->view_projection, model_data.mat_model);
   model_data.u_color = Util_Vec4FromColor(drawable_data->color);

   mat4x4 mat_normal_model = Util_TransposeMat4(model_data.mat_invmodel);
   model_data.mat_normal_model[0].xyz = mat_normal_model.v[0].xyz;
   model_data.mat_normal_model[1].xyz = mat_normal_model.v[1].xyz;
   model_data.mat_normal_model[2].xyz = mat_normal_model.v[2].xyz;

   Graphics_UpdateBuffer(renderer->graphics, renderer->ubo.model_data, &model_data, 1, sizeof(ModelData));
   Graphics_UseBuffer(renderer->graphics, renderer->ubo.model_data, 2);

   Graphics_BindTexture(renderer->graphics, renderer->built_in.texture.white, 0);

   Graphics_Draw(renderer->graphics, drawable_data->material.shader, drawable_data->geometry, (UniformBlockList){ .count = 0 });

   Graphics_UnbindTextures(renderer->graphics, GFX_TEXTURETYPE_2D);
}
