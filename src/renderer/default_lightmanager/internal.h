#ifndef DEFAULT_LIGHTMANAGER_INTERNAL
#define DEFAULT_LIGHTMANAGER_INTERNAL

#include "util/extra_types.h"
#include "util/matrix.h"
#include "util/types.h"
#include "util/array.h"
#include "graphics.h"
#include "renderer.h"

#include "default_lightmanager.h"
#include "util/vec3.h"

#define LIGHTMAN_INVALID_LIST_LINK UINT16_MAX

struct lightman_Cluster_t
{
   vec4 center;
   vec4 extents;
   u32 frustum_idx[3];
   u32 light_count;
   u32 indices[];
   
};

typedef struct lightman_PackedLight_t
{
   // -< 16 bytes
   vec3 origin;
   f32 radius;
   // >- 16 bytes

   // -< 16 bytes
   color8 rgbe_color; // RGBE encoded HDR color value

   u16 cos_half_angle; // cosine of spotlight cone half angle. normalized as 0 to 1 value
   u16 spot_softness; // spotlight softness factor. normalized as 0 to 1 value

   u16 theta; // angle theta in turns. normalized as 0 to 1 value
   u16 phi; // angle phi in turns. normalized as 0 to 1 value

   i16 shadow_id; // id of shadow map. 0 is no shadows, negative sign is for cubemap shadows. when shadow_id != 0 the index is abs(shadow_id) - 1
   i16 next_light; // index of next light. if value == UINT16_MAX then this is the last light in the list
   // >- 16 bytes
   
} lightman_PackedLight;

typedef struct lightman_PackedSunLight_t
{
   // -< 16 bytes

   color8 rgbe_color; // RGBE encoded HDR color value

   u16 pcf_quality; // quality of pcf filtering
   u16 shadow_blur; // how blurry the shadow is. normalized as 0 to 1 value

   u16 theta; // angle theta in turns. normalized as 0 to 1 value
   u16 phi; // angle phi in turns. normalized as 0 to 1 value

   i16 shadow_id; // id of shadow map. negative sign is no shadows. when shadow_id >= 0 the index is shadow_id * num_cascades + current_cascade
   i16 next_light; // index of next light. if value == UINT16_MAX then this is the last light in the list

   // >- 16 bytes

} lightman_PackedSunLight;

typedef struct lightman_LightDrawable_t
{
   vec3 origin;
   f32 radius;
   color8 color;
   f32 brightness;
   f32 spot_angle;
   f32 spot_softness;
   f32 theta;
   f32 phi;

   u16 next_idx;
   u16 prev_idx;

   u16 light_idx;
   i16 next_light_idx;
   i16 prev_light_idx;
   i16 shadow_idx;

   struct {
      u16 enabled: 1;
      u16 culled: 1;
      u16 needs_update: 1;
      u16 casts_shadows: 1;

   };

} lightman_LightDrawable;

struct DefaultLightManager_t
{
   lightman_PackedSunLight* packed_sun_lights;
   lightman_PackedLight* packed_lights;

   union {
      struct {
         u32 cluster_dimensions[3];
         u32 total_clusters;
      };

      u32 cluster_info[4];

   };

   u32 lights_per_cluster;

   i32 light_list;

   u16 freed_light_root_idx;
   u16 active_light_root_idx;

   u16 light_drawable_type_idx;

   Framebuffer cascade_fbo;
   Framebuffer shadow_fbo;

   Buffer cluster_ssbo;
   Buffer sun_light_ssbo;
   Buffer light_ssbo;
   Shader build_clusters_cs;
   Shader fill_clusters_cs;

   struct {
      Texture pointlight;
      Texture spotlight;
      Texture sunlight;

      i32 num_cascades;
      i32 num_shadows;
      res2D cascade_size;
      res2D shadow_size;

   } shadow;

   char light_count_define[];

};

static inline uS LIGHTMAN_ClustersSize(DefaultLightManager* lightmanager)
{
   return sizeof(u32) * 4 + (sizeof(struct lightman_Cluster_t) + lightmanager->lights_per_cluster * sizeof(u32)) * lightmanager->total_clusters;
}

static inline uS LIGHTMAN_LightBufferSize(DefaultLightManager* lightmanager)
{
   uS light_memory = Util_ArrayMemory(lightmanager->packed_lights);

   return sizeof(i32) * 4 + sizeof(lightman_PackedLight) * light_memory;
}

static inline u16 LIGHTMAN_U16Norm(f32 value)
{
   const f32 u16_maxf = (f32)UINT16_MAX;
   f32 f = M_CLAMP(value, 0.0f, 1.0f);

   return (u16)(f * u16_maxf);
}

static inline mat4x4 LIGHTMAN_CubemapViewMatrix(vec3 origin, u8 cubemap_face)
{
   const vec3 s[2] = { { 1, 0, 0 }, {-1, 0, 0 } };
   const vec3 u[2] = { { 0, 1, 0 }, { 0,-1, 0 } };
   const vec3 f[2] = { { 0, 0, 1 }, { 0, 0,-1 } };

   switch (cubemap_face)
   {
      case GFX_CUBEMAPFACE_POSITIVE_X:
         return Util_LookAtMatrix(origin, Util_AddVec3(origin, s[1]), u[1]);
      
      case GFX_CUBEMAPFACE_NEGATIVE_X:
         return Util_LookAtMatrix(origin, Util_AddVec3(origin, s[0]), u[1]);

      case GFX_CUBEMAPFACE_POSITIVE_Y:
         Util_ViewMatrix(origin, VEC3(50, 0, 0), 0);
      
      case GFX_CUBEMAPFACE_NEGATIVE_Y:
         return Util_ViewMatrix(origin, VEC3(150, 0, 0), 0);

      case GFX_CUBEMAPFACE_POSITIVE_Z:
         return Util_LookAtMatrix(origin, Util_AddVec3(origin, f[1]), u[1]);
      
      case GFX_CUBEMAPFACE_NEGATIVE_Z:
         return Util_LookAtMatrix(origin, Util_AddVec3(origin, f[0]), u[1]);

      default:
         break;

   }

   return Util_IdentityMat4();
}

static inline lightman_PackedLight LIGHTMAN_CreatePackedLight(lightman_LightDrawable light_drawable)
{
   vec4 base_color = Util_Vec4FromColor(light_drawable.color);
   vec3 light_color = Util_ScaleVec3(base_color.xyz, base_color.w * light_drawable.brightness);

   f32 cos_half_angle = M_COS(light_drawable.spot_angle * 0.5f) * 0.5f + 0.5f;
   f32 spot_softness = light_drawable.spot_softness;
   f32 theta = light_drawable.theta * 0.005f;
   f32 phi = light_drawable.phi * 0.005f;

   lightman_PackedLight packed_light = { 0 };
   packed_light.origin = light_drawable.origin;
   packed_light.radius = light_drawable.radius;
   packed_light.rgbe_color = Util_MakeRGBE(light_color);
   packed_light.cos_half_angle = LIGHTMAN_U16Norm(cos_half_angle);
   packed_light.spot_softness = LIGHTMAN_U16Norm(spot_softness);
   packed_light.theta = LIGHTMAN_U16Norm(theta);
   packed_light.phi = LIGHTMAN_U16Norm(phi);
   packed_light.shadow_id = light_drawable.shadow_idx;
   packed_light.next_light = light_drawable.next_light_idx;

   return packed_light;
}

void LIGHTMAN_AddLight(Renderer* renderer, Drawable light_obj);
void LIGHTMAN_RemoveLight(Renderer* renderer, Drawable light_obj);

error LIGHTMAN_InitFunc(Renderer* renderer);
error LIGHTMAN_FreeFunc(Renderer* renderer);
error LIGHTMAN_PreRenderFunc(Renderer* renderer, u32 pass_id);
error LIGHTMAN_OnRenderFunc(Renderer* renderer, u32 pass_id);
ShaderDefines LIGHTMAN_Defines(Renderer* renderer);

void LIGHTMAN_UpdateLight(Renderer* renderer, u16 index);

void LIGHTMAN_LightRenderFunc(Renderer* renderer, Drawable self, u32 pass_id);
void LIGHTMAN_LightEnableFunc(Renderer* renderer, Drawable self);
void LIGHTMAN_LightDisableFunc(Renderer* renderer, Drawable self);

#endif
