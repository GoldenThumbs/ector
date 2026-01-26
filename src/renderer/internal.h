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

typedef struct rndr_DrawableType_t
{
   char* name;
   
   u8* drawable_buffer;
   
   DrawableFunc render;
   DrawableFunc on_enable;
   DrawableFunc on_disable;
   
   u32 type_size;
   
   u16 visible_drawable_root;
   u16 free_drawable_root;
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
   vec3 origin;
   f32 radius;
   color8 rgbe_color; // RGBE encoded HDR color value
   f32 cos_half_angle; // cosine of spotlight cone half angle
   f32 theta_radians; // angle theta in radians
   f32 phi_radians; // angle phi in radians
   
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

struct Renderer_t {
   Graphics* graphics;
   
   rndr_Cluster* clusters;
   rndr_PointLightSource* point_lights;
   rndr_DrawableType* drawable_types;
   
   struct {
      u32 x, y, z;
      u32 total_clusters;
   } cluster_dimensions;

   struct {
      Buffer camera_data;
      Buffer model_data;
   } ubo;

   struct {
      Buffer light_buffer;
      Buffer cluster_buffer;
   } ssbo;
   
   struct {
      struct {
         Texture white;
         Texture gray;
         Texture black;
         Texture normal;
         
      } texture;
      
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
   
};

static inline rndr_Drawable* RNDR_DrawableAtIndex(rndr_DrawableType* drawable_type, u16 drawable_idx)
{
   return (rndr_Drawable*)(drawable_type->drawable_buffer + (uS)drawable_idx * (uS)drawable_type->type_size);
}

Texture RNDR_LoadColorTexture(Graphics* graphics, color8 color, u8 texture_type);

Geometry RNDR_Plane(Graphics* graphics);
Geometry RNDR_Box(Graphics* graphics);

u16 RNDR_GetDrawableTypeIndex(Renderer* renderer, const char* drawable_type_name);
rndr_DrawableType* RNDR_GetDrawableType(Renderer* renderer, u16 drawable_type_idx);
rndr_Drawable* RNDR_GetDrawable(Renderer* renderer, Drawable res_drawable);
void RNDR_RegisterDefaultDrawables(Renderer* renderer);

void RNDR_GeometryRenderFunc(Renderer* renderer, Drawable self);


#endif
