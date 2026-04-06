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
   INTERNAL_RNDR_SURF_TEXTURE_RESERVED = 15,
   INTERNAL_RNDR_SURF_TEXTURE_USER_SET = 16
   
};

typedef struct rndr_DrawableType_t
{
   char* name;
   
   u8* drawable_buffer;
   
   DrawableRenderFunc render;
   DrawableFunc on_create;
   DrawableFunc on_remove;
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
   
   u16 drawable_type_idx;
   u16 next_visible;
   u16 prev_visible;
   
   union {
      u16 next_freed;
      u16 next_active;

   };
   
   union {
      u16 prev_freed;
      u16 prev_active;

   };

   struct {
      u16 enabled: 1;
      u16 culled: 1;

   };
   
   handle compare;
   
   u8 data[];

} rndr_Drawable;

typedef struct rndr_Surface_t
{
   char* name;

   SurfacePass passes[SURF_MAX_PASSES];
   u8 textures[SURF_MAX_TEXTURES];
   
   handle compare;
   u16 next_freed;

   u16 pass_count;

} rndr_Surface;

struct Renderer_t
{
   char* app_path;
   
   Graphics* graphics;
   
   rndr_Surface* surfaces;
   rndr_DrawableType* drawable_types;

   LightManagerInfo lightmanager_info;

   struct {
      Buffer camera_buffer;
      Buffer model_buffer;

   } ubo;
   
   struct {
      union {
         Texture textures[RNDR_SURF_DEFAULT_TEXTURE_COUNT];

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
         Shader unlit;
         Shader basic;
         
      } shader;
      
   } built_in;

   f32 near_clip;
   f32 far_clip;
   f32 fov;

   f32 aspect_ratio;
   res2D size;

   mat4x4 view;
   mat4x4 inv_view;
   mat4x4 projection;
   mat4x4 inv_projection;
   mat4x4 view_projection;

   f32 frame_delta;

   u16 freed_surface_root;

   struct {
      u16 use_ortho_camera: 1;
      u16 update_projection: 1;
      u16 update_view_projection: 1;
      u16 user_supplied_projection: 1;

   };

   u8 texture_slots[SURF_MAX_TEXTURES];
   
};

static inline u16 RNDR_U16Norm(f32 value)
{
   const f32 u16_maxf = (f32)UINT16_MAX;
   f32 f = M_CLAMP(value, 0.0f, 1.0f);

   return (u16)(f * u16_maxf);
}

static inline rndr_Drawable* RNDR_DrawableAtIndex(rndr_DrawableType* drawable_type, u16 drawable_idx)
{
   if (drawable_idx == INVALID_HANDLE)
      return NULL;
   
   return (rndr_Drawable*)(drawable_type->drawable_buffer + (uS)drawable_idx * (uS)drawable_type->type_size);
}

Geometry RNDR_CreateDefaultPlane(Graphics* graphics);
Geometry RNDR_CreateDefaultBox(Graphics* graphics);

void RNDR_HandleMatrices(Renderer* renderer, res2D size);

u16 RNDR_GetSurfaceIndex(Renderer* renderer, const char* surface_name);
u16 RNDR_GetDrawableTypeIndex(Renderer* renderer, const char* drawable_type_name);
rndr_Surface* RNDR_GetSurface(Renderer* renderer, Surface res_surface);
rndr_DrawableType* RNDR_GetDrawableType(Renderer* renderer, u16 drawable_type_idx);
rndr_Drawable* RNDR_GetDrawable(Renderer* renderer, Drawable res_drawable);
void RNDR_RegisterDefaultDrawables(Renderer* renderer);
void RNDR_BindTextureAtSlot(Renderer* renderer, u32 bind_slot, u8 texture_default, Texture texture);

UniformBlockList RNDR_UpdateMaterialUBOs(Renderer* renderer, SurfaceMaterial material, u32 pass_id);

void RNDR_GeometryOnCreateFunc(Renderer* renderer, Drawable self);
void RNDR_GeometryRenderFunc(Renderer* renderer, Drawable self, u32 pass_id);

#endif
