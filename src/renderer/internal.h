#ifndef RNDR_INTERNAL
#define RNDR_INTERNAL

#include "util/types.h"
#include "util/extra_types.h"
#include "graphics.h"

#include "renderer.h"

#define RNDR_INVALID_LIST_LINK UINT16_MAX
#define RNDR_INVALID_TYPE_IDX UINT16_MAX

#define RNDR_CLUSTER_X 24
#define RNDR_CLUSTER_Y 16
#define RNDR_CLUSTER_Z 32

#define RNDR_NAME_MAX 128

enum {
   RNDR_SURF_TEXTURE_USER_SET = 16
   
};

typedef struct rndr_DrawableType_t
{
   char* name;
   
   u8* drawable_buffer;
   
   DrawableRenderFunc render;
   DrawableFunc on_enable;
   DrawableFunc on_disable;
   
   u32 type_size;
   
   u16 visible_drawable_root;
   u16 freed_drawable_root;
   u16 active_drawable_root;
   
   u16 culled_drawable_count;
   
} rndr_DrawableType;

typedef struct rndr_Drawable_t
{
   BBox bounds;
   
   struct {
      u16 enabled: 1;
      u16 culled: 1;
   };
   
   u16 drawable_type_idx;
   u16 next_visible;
   u16 last_visible;
   
   union {
      u16 next_freed;
      u16 next_active;
   };
   
   union {
      u16 last_freed;
      u16 last_active;
   };
   
   handle compare;
   
   u8 data[];
   
} rndr_Drawable;

typedef struct rndr_PointLightSource_t
{
   // -< 16 bytes

   vec3 origin;
   f32 radius;

   // >- 16 bytes

   // -< 16 bytes

   color8 rgbe_color; // RGBE encoded HDR color value

   u16 cos_half_angle; // cosine of spotlight cone half angle. bfloat16 encoding
   u16 spot_softness; // spotlight softness factor. bfloat16 encoding

   u16 theta; // angle theta in radians. bfloat16 encoding
   u16 phi; // angle phi in radians. bfloat16 encoding

   i16 shadow_id; // id of shadow map. 0 is no shadows, negative sign is for cubemap shadows. when shadow_id != 0 the index is abs(shadow_id) - 1
   i16 next_light; // index of next light. if sign is negative then there is no next light

   // >- 16 bytes
   
} rndr_PointLightSource;

typedef struct rndr_Cluster_t
{
   u32 frustum_idx[3];
   u32 light_count;
   vec4 min_bound;
   vec4 max_bound;
   // color8 min_bound; // lower boundary vertex relative to cluster position in frustum, RGBE encoded XYZ values
   // color8 max_bound; // upper boundary vertex relative to cluster position in frustum, RGBE encoded XYZ values
   u32 indices[200];
   
} rndr_Cluster;

typedef struct rndr_Surface_t
{
   char* name;

   SurfacePass passes[SURF_MAX_PASSES];
   u8 textures[SURF_MAX_TEXTURES];
   
   handle compare;
   u16 next_freed;

   u16 pass_count;

} rndr_Surface;

struct Renderer_t {
   Graphics* graphics;
   
   rndr_Surface* surfaces;
   rndr_Cluster* clusters;
   rndr_PointLightSource* point_lights;
   rndr_DrawableType* drawable_types;
   
   struct {
      u32 x, y, z;
      u32 total_clusters;
   } cluster_dimensions;

   struct {
      Buffer camera_buffer;
      Buffer model_buffer;
   } ubo;

   struct {
      Buffer light_buffer;
      Buffer cluster_buffer;
   } ssbo;
   
   struct {
      union {
         Texture textures[4];

         struct {
            Texture white;
            Texture gray;
            Texture black;
            Texture normal;

         } texture;
         
      };
      
      struct {
         Geometry plane;
         Geometry box;
         
      } geometry;
      
      struct {
         Shader basic;
         
      } shader;
      
   } built_in;

   mat4x4 view;
   mat4x4 projection;
   mat4x4 view_projection;

   u16 freed_surface_root;

   struct {
      u16 update_lights: 1;
   };

   u8 texture_slots[SURF_MAX_TEXTURES];
   
};

static inline u16 RNDR_NormF16(f32 value)
{
   const f32 u32_maxf = (f32)UINT16_MAX;
   f32 f = M_CLAMP(value, 0.0f, 1.0f);

   return (u16)(f * u32_maxf);
}

static inline rndr_Drawable* RNDR_DrawableAtIndex(rndr_DrawableType* drawable_type, u16 drawable_idx)
{
   return (rndr_Drawable*)(drawable_type->drawable_buffer + (uS)drawable_idx * (uS)drawable_type->type_size);
}

Texture RNDR_LoadColorTexture(Graphics* graphics, color8 color, u8 texture_type);

Geometry RNDR_Plane(Graphics* graphics);
Geometry RNDR_Box(Graphics* graphics);

u16 RNDR_GetSurfaceIndex(Renderer* renderer, const char* surface_name);
u16 RNDR_GetDrawableTypeIndex(Renderer* renderer, const char* drawable_type_name);
rndr_Surface* RNDR_GetSurface(Renderer* renderer, Surface res_surface);
rndr_DrawableType* RNDR_GetDrawableType(Renderer* renderer, u16 drawable_type_idx);
rndr_Drawable* RNDR_GetDrawable(Renderer* renderer, Drawable res_drawable);
void RNDR_RegisterDefaultDrawables(Renderer* renderer);
void RNDR_BindTextureAtSlot(Renderer* renderer, u32 bind_slot, u8 texture_default, Texture texture);

UniformBlockList RNDR_UpdateMaterialUBOs(Renderer* renderer, SurfaceMaterial material, u32 pass_id);

void RNDR_GeometryRenderFunc(Renderer* renderer, Drawable self, u32 pass_id);

void RNDR_LightRenderFunc(Renderer* renderer, Drawable self, u32 pass_id);
void RNDR_LightEnableFunc(Renderer* renderer, Drawable self);
void RNDR_LightDisableFunc(Renderer* renderer, Drawable self);

#endif
