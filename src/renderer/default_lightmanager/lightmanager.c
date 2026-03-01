#include "util/types.h"
#include "util/array.h"
#include "graphics.h"
#include "renderer.h"

#include "renderer/internal.h"
#include "default_lightmanager.h"
#include "renderer/default_lightmanager/internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LightManagerInfo DefaultLightManager_Info(Renderer* renderer)
{
   LightManagerInfo lightmanager_info = {
      .id = DEFAULTLIGHTMANAGER_ID,
      .lightman_init = LIGHTMAN_InitFunc,
      .lightman_free = LIGHTMAN_FreeFunc,
      .lightman_prerender = LIGHTMAN_PreRenderFunc,
      .lightman_on_render = NULL,
      .lightman_defs = LIGHTMAN_Defines
   };

   return lightmanager_info;
}

DefaultLightManager* DefaultLightManager_Init(Renderer* renderer)
{
   if (renderer == NULL)
      return NULL;

   u32 cluster_x = 24;
   u32 cluster_y = 16;
   u32 cluster_z = 32;
   u32 lights_per_cluster = 200;

   i32 define_size = snprintf(NULL, 0, "LIGHTS_PER_CLUSTER %u", lights_per_cluster) + 1;

   DefaultLightManager* lightmanager = malloc(sizeof(DefaultLightManager) + define_size);
   if (lightmanager == NULL)
      return NULL;

   memset(lightmanager, 0, sizeof(DefaultLightManager) + define_size);

   Graphics* graphics = Renderer_Graphics(renderer);

   snprintf(lightmanager->light_count_define, define_size, "LIGHTS_PER_CLUSTER %u", lights_per_cluster);

   lightmanager->packed_lights = NEW_ARRAY_N(lightman_PackedLight, 16);
   lightmanager->packed_sun_lights = NEW_ARRAY_N(lightman_PackedSunLight, 1);

   lightmanager->cluster_dimensions[0] = cluster_x;
   lightmanager->cluster_dimensions[1] = cluster_y;
   lightmanager->cluster_dimensions[2] = cluster_z;
   lightmanager->total_clusters = cluster_x * cluster_y * cluster_z;
   
   lightmanager->light_list = -1;
   
   lightmanager->lights_per_cluster = lights_per_cluster;

   lightmanager->freed_light_root_id = INVALID_HANDLE_ID;
   lightmanager->active_light_root_id = INVALID_HANDLE_ID;

   const char* defines[] = {
      lightmanager->light_count_define
   };

   lightmanager->build_clusters_cs = RNDR_LoadShader(graphics, renderer->app_path, "assets/core/shaders/cs_build_clusters.glsl", NULL, 0, true);
   lightmanager->fill_clusters_cs = RNDR_LoadShader(graphics, renderer->app_path, "assets/core/shaders/cs_cull_lights.glsl", defines, 1, true);

   lightmanager->cluster_ssbo = Graphics_CreateBufferExplicit(graphics, NULL, LIGHTMAN_ClustersSize(lightmanager), GFX_DRAWMODE_STATIC, GFX_BUFFERTYPE_STORAGE);
   lightmanager->light_ssbo = Graphics_CreateBufferExplicit(graphics, NULL, LIGHTMAN_LightBufferSize(lightmanager), GFX_DRAWMODE_STATIC, GFX_BUFFERTYPE_STORAGE);

   Graphics_UpdateBuffer(graphics, lightmanager->cluster_ssbo, &lightmanager->cluster_info, 4, sizeof(u32));
   Graphics_UpdateBuffer(graphics, lightmanager->light_ssbo, &lightmanager->light_list, 1, sizeof(i32));

   Renderer_RegisterDrawableType(renderer, LIGHT_DRAWABLE_TYPE, &(DrawableTypeDesc){
      .render_func = LIGHTMAN_LightRenderFunc,
      .on_enable_func = LIGHTMAN_LightEnableFunc,
      .on_disable_func = LIGHTMAN_LightDisableFunc,
      .data_size = sizeof(lightman_LightDrawable)
   });

   return lightmanager;
}

void DefaultLightManager_Free(DefaultLightManager* lightmanager)
{
   if (lightmanager == NULL)
      return;

   FREE_ARRAY(lightmanager->packed_lights);
   FREE_ARRAY(lightmanager->packed_sun_lights);

   free(lightmanager);
}

void DefaultLightManager_PreRender(DefaultLightManager* lightmanager, Renderer* renderer)
{
   if (lightmanager == NULL || renderer == NULL)
      return;

   Graphics* graphics = Renderer_Graphics(renderer);
   
   Graphics_UpdateBuffer(graphics, lightmanager->light_ssbo, &lightmanager->light_list, 1, sizeof(i32));

   Graphics_UseBuffer(graphics, lightmanager->cluster_ssbo, 1);
   Graphics_UseBuffer(graphics, lightmanager->light_ssbo, 2);

   Graphics_Dispatch(
      graphics,
      lightmanager->build_clusters_cs,
      lightmanager->cluster_dimensions[0],
      lightmanager->cluster_dimensions[1],
      lightmanager->cluster_dimensions[2],
      (UniformBlockList){ .count = 0 }
   );
   Graphics_DispatchBarrier();

   Graphics_Dispatch(
      graphics,
      lightmanager->fill_clusters_cs,
      lightmanager->total_clusters / 128,
      1,
      1,
      (UniformBlockList){ .count = 0 }
   );
   Graphics_DispatchBarrier();

}

Drawable DefaultLightManager_CreateLight(Renderer* renderer)
{
   if (!Renderer_IsLightManagerValid(renderer, DEFAULTLIGHTMANAGER_ID))
      return (Drawable){ .id = INVALID_HANDLE_ID };

   return Renderer_CreateDrawable(renderer, LIGHT_DRAWABLE_TYPE);
}

void DefaultLightManager_SetLightOrigin(Renderer* renderer, Drawable light_drawable, vec3 origin)
{
   lightman_LightDrawable* light_data = Renderer_DrawableData(renderer, light_drawable);
   if (light_data == NULL || !Renderer_IsLightManagerValid(renderer, DEFAULTLIGHTMANAGER_ID))
      return;

   light_data->origin = origin;
   light_data->needs_update = true;

}

void DefaultLightManager_SetLightRadius(Renderer* renderer, Drawable light_drawable, f32 radius)
{
   lightman_LightDrawable* light_data = Renderer_DrawableData(renderer, light_drawable);
   if (light_data == NULL || !Renderer_IsLightManagerValid(renderer, DEFAULTLIGHTMANAGER_ID))
      return;

   light_data->radius = radius;
   light_data->needs_update = true;

}

void DefaultLightManager_SetLightColor(Renderer* renderer, Drawable light_drawable, color8 color)
{
   lightman_LightDrawable* light_data = Renderer_DrawableData(renderer, light_drawable);
   if (light_data == NULL || !Renderer_IsLightManagerValid(renderer, DEFAULTLIGHTMANAGER_ID))
      return;

   light_data->color = color;
   light_data->needs_update = true;

}

void DefaultLightManager_SetLightBrightness(Renderer* renderer, Drawable light_drawable, f32 brightness)
{
   lightman_LightDrawable* light_data = Renderer_DrawableData(renderer, light_drawable);
   if (light_data == NULL || !Renderer_IsLightManagerValid(renderer, DEFAULTLIGHTMANAGER_ID))
      return;

   light_data->brightness = brightness;
   light_data->needs_update = true;

}

void DefaultLightManager_SetLightSpotlightAngle(Renderer* renderer, Drawable light_drawable, f32 spotlight_angle)
{
   lightman_LightDrawable* light_data = Renderer_DrawableData(renderer, light_drawable);
   if (light_data == NULL || !Renderer_IsLightManagerValid(renderer, DEFAULTLIGHTMANAGER_ID))
      return;

   light_data->spot_angle = spotlight_angle;
   light_data->needs_update = true;

}

void DefaultLightManager_SetLightSpotlightSoftness(Renderer* renderer, Drawable light_drawable, f32 spotlight_softness)
{
   lightman_LightDrawable* light_data = Renderer_DrawableData(renderer, light_drawable);
   if (light_data == NULL || !Renderer_IsLightManagerValid(renderer, DEFAULTLIGHTMANAGER_ID))
      return;

   light_data->spot_softness = spotlight_softness;
   light_data->needs_update = true;

}

void DefaultLightManager_SetLightAngles(Renderer* renderer, Drawable light_drawable, f32 azimuth_angle, f32 zenith_angle)
{
   lightman_LightDrawable* light_data = Renderer_DrawableData(renderer, light_drawable);
   if (light_data == NULL || !Renderer_IsLightManagerValid(renderer, DEFAULTLIGHTMANAGER_ID))
      return;

   light_data->theta = azimuth_angle;
   light_data->phi = zenith_angle;
   light_data->needs_update = true;

}

error LIGHTMAN_InitFunc(Renderer* renderer)
{
   if (renderer == NULL || Renderer_LightManagerInfo(renderer)->id != DEFAULTLIGHTMANAGER_ID)
      return (error){ .general = ERR_ERROR };

   LightManagerInfo* lightmanager_info = Renderer_LightManagerInfo(renderer);
   lightmanager_info->data = DefaultLightManager_Init(renderer);

   return (error){ 0 };
}

error LIGHTMAN_FreeFunc(Renderer* renderer)
{
   if (!Renderer_IsLightManagerValid(renderer, DEFAULTLIGHTMANAGER_ID))
      return (error){ .general = ERR_ERROR };

   DefaultLightManager_Free(Renderer_LightManager(renderer));

   return (error){ 0 };
}

error LIGHTMAN_PreRenderFunc(Renderer* renderer)
{
   if (!Renderer_IsLightManagerValid(renderer, DEFAULTLIGHTMANAGER_ID))
      return (error){ .general = ERR_ERROR };

   DefaultLightManager_PreRender(Renderer_LightManager(renderer), renderer);

   return (error){ 0 };
}

ShaderDefines LIGHTMAN_Defines(Renderer* renderer)
{
   if (!Renderer_IsLightManagerValid(renderer, DEFAULTLIGHTMANAGER_ID))
      return (ShaderDefines){ 0 };

   DefaultLightManager* lightmanager = Renderer_LightManager(renderer);

   return (ShaderDefines){
      .define_count = 2,
      .defines = {
         [0] = "USE_LIGHTING",
         [1] = lightmanager->light_count_define
      }
   };
}

void LIGHTMAN_UpdateLight(Renderer* renderer, u16 index)
{
   if (!Renderer_IsLightManagerValid(renderer, DEFAULTLIGHTMANAGER_ID))
      return;

   DefaultLightManager* lightmanager = Renderer_LightManager(renderer);

   Graphics_UpdateBufferExplicit(
      Renderer_Graphics(renderer),
      lightmanager->light_ssbo,
      &lightmanager->packed_lights[index],
      sizeof(u32) * 4 + index * sizeof(lightman_PackedLight),
      sizeof(lightman_PackedLight)
   );

}

void LIGHTMAN_LightRenderFunc(Renderer* renderer, Drawable self, u32 pass_id)
{
   lightman_LightDrawable* light_data = Renderer_DrawableData(renderer, self);
   if (light_data == NULL || !Renderer_IsLightManagerValid(renderer, DEFAULTLIGHTMANAGER_ID) || pass_id != 0)
      return;

   if (light_data->needs_update)
   {
      DefaultLightManager* lightmanager = Renderer_LightManager(renderer);

      lightman_PackedLight packed_light = LIGHTMAN_CreatePackedLight(*light_data);
      lightmanager->packed_lights[light_data->light_idx] = packed_light;

      LIGHTMAN_UpdateLight(renderer, light_data->light_idx);

   }

}

void LIGHTMAN_LightEnableFunc(Renderer* renderer, Drawable self)
{
   lightman_LightDrawable* light_data = Renderer_DrawableData(renderer, self);
   if (light_data == NULL || !Renderer_IsLightManagerValid(renderer, DEFAULTLIGHTMANAGER_ID))
      return;

   Graphics* graphics = Renderer_Graphics(renderer);
   DefaultLightManager* lightmanager = Renderer_LightManager(renderer);

   light_data->origin = VEC3(0);
   light_data->radius = 5.0f;
   light_data->color.hex = 0xFFFFFFFF;
   light_data->brightness = 1.0f;
   light_data->spot_angle = 200.0f;
   light_data->spot_softness = 0.0f;
   light_data->theta = 0.0f;
   light_data->phi = 0.0f;

   light_data->prev_id = INVALID_HANDLE_ID;
   light_data->prev_light_idx = 0;

   light_data->next_id = lightmanager->active_light_root_id;
   light_data->next_light_idx = 0;
   
   lightmanager->active_light_root_id = self.id;

   u32 light_count = Util_ArrayLength(lightmanager->packed_lights);
   light_data->light_idx = (u16)light_count;

   if (lightmanager->freed_light_root_id == INVALID_HANDLE_ID)
   {
      uS old_light_memory = Util_ArrayMemory(lightmanager->packed_lights);
      SET_ARRAY_LENGTH(lightmanager->packed_lights, light_count + 1);

      if (old_light_memory != Util_ArrayMemory(lightmanager->packed_lights))
         Graphics_ReuseBufferExplicit(graphics, lightmanager->packed_lights, LIGHTMAN_LightBufferSize(lightmanager), lightmanager->light_ssbo);

   } else {
      Drawable free_drawable = self;
      free_drawable.id = lightmanager->freed_light_root_id;
      lightman_LightDrawable* free_light_data = Renderer_DrawableData(renderer, free_drawable);
      if (free_light_data != NULL)
      {
         light_data->light_idx = free_light_data->light_idx;

         Drawable next_drawable = self;
         next_drawable.id = free_light_data->next_id;;
         lightman_LightDrawable* next_light_data = Renderer_DrawableData(renderer, next_drawable);
         if (next_light_data != NULL)
         {
            next_light_data->prev_id = INVALID_HANDLE_ID;
            next_light_data->prev_light_idx = 0;

         }

      }

   }

   lightmanager->light_list = (i32)light_data->light_idx;

   Drawable next_drawable = self;
   next_drawable.id = light_data->next_id;
   lightman_LightDrawable* next_light_data = Renderer_DrawableData(renderer, next_drawable);
   if (next_light_data != NULL)
   {
      next_light_data->prev_id = self.id;
      next_light_data->prev_light_idx = (i16)((i32)light_data->light_idx - (i32)next_light_data->light_idx);

      light_data->next_light_idx = (i16)((i32)next_light_data->light_idx - (i32)light_data->light_idx);

   }

   lightman_PackedLight packed_light = LIGHTMAN_CreatePackedLight(*light_data);
   lightmanager->packed_lights[light_data->light_idx] = packed_light;

   LIGHTMAN_UpdateLight(renderer, light_data->light_idx);

}

void LIGHTMAN_LightDisableFunc(Renderer* renderer, Drawable self)
{
   lightman_LightDrawable* light_data = Renderer_DrawableData(renderer, self);
   if (light_data == NULL || !Renderer_IsLightManagerValid(renderer, DEFAULTLIGHTMANAGER_ID))
      return;

   Graphics* graphics = Renderer_Graphics(renderer);
   DefaultLightManager* lightmanager = Renderer_LightManager(renderer);
   
   Drawable next_drawable = self;
   next_drawable.id = light_data->next_id;
   lightman_LightDrawable* next_light_data = Renderer_DrawableData(renderer, next_drawable);

   Drawable prev_drawable = self;
   prev_drawable.id = light_data->prev_id;
   lightman_LightDrawable* prev_light_data = Renderer_DrawableData(renderer, prev_drawable);

   if (next_light_data != NULL)
   {
      next_light_data->prev_id = prev_drawable.id;
      next_light_data->prev_light_idx = (prev_light_data == NULL) ? 0 : (i16)((i32)prev_light_data->light_idx - (i32)next_light_data->light_idx);

   }

   if (prev_light_data != NULL)
   {
      prev_light_data->next_id = next_drawable.id;
      prev_light_data->next_light_idx = (next_light_data == NULL) ? 0 : (i16)((i32)next_light_data->light_idx - (i32)prev_light_data->light_idx);

      lightmanager->packed_lights[prev_light_data->light_idx].next_light = prev_light_data->next_light_idx;

      LIGHTMAN_UpdateLight(renderer, prev_light_data->light_idx);

   } else {
      lightmanager->active_light_root_id = next_drawable.id;
      lightmanager->light_list = (next_light_data == NULL) ? -1 : (i32)next_light_data->light_idx;

   }

   light_data->prev_id = INVALID_HANDLE_ID;
   light_data->prev_light_idx = 0;

   light_data->next_id = lightmanager->freed_light_root_id;
   lightmanager->freed_light_root_id = self.id;

   Drawable next_free_drawable = self;
   next_free_drawable.id = light_data->next_id;
   lightman_LightDrawable* next_free_light_data = Renderer_DrawableData(renderer, next_free_drawable);

   if (next_free_light_data != NULL)
   {
      next_free_light_data->prev_id = self.id;
      next_free_light_data->prev_light_idx = (i16)((i32)light_data->light_idx - (i32)next_free_light_data->light_idx);

   }

   LIGHTMAN_UpdateLight(renderer, light_data->light_idx);

}
