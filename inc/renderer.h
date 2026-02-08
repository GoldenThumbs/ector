#ifndef ECT_RENDERER_H
#define ECT_RENDERER_H

#include "util/extra_types.h"
#include "util/types.h"
// #include "util/extra_types.h"
#include "mesh.h"
#include "graphics.h"

#define RENDERER_MODULE "renderer"

#define EMPTY_DRAWABLE_TYPE "EmptyDrawable"
#define GEOMETRY_DRAWABLE_TYPE "GeometryDrawable"
// #define LIGHT_DRAWABLE_TYPE "LightDrawable"

#define SURF_MAX_PASSES 4
#define SURF_MAX_BLOCKS_PER_PASS 4
#define SURF_MAX_TEXTURES 8

enum {
   RNDR_SURF_TEXTURE_WHITE = 0,
   RNDR_SURF_TEXTURE_GRAY,
   RNDR_SURF_TEXTURE_BLACK,
   RNDR_SURF_TEXTURE_NORMAL
};

typedef handle Surface;

typedef union Drawable_t
{
   u64 total_bits;
   u32 id;

   struct {
      u16 handle;
      u16 ref;
      u16 drawable_type_idx;
      u8 mem_unused_[2];

   };

} Drawable;

typedef struct CameraData_t
{
   mat4x4 mat_view;
   mat4x4 mat_proj;
   mat4x4 mat_invview;
   mat4x4 mat_invproj;
   vec2 u_near_far;
   u32 u_width, u_height;
   vec4 u_proj_info;

} CameraData;

typedef struct ModelData_t
{
   mat4x4 mat_model;
   mat4x4 mat_invmodel;
   mat4x4 mat_mvp;
   vec4 mat_normal_model[3];
   vec4 u_color;

} ModelData;

typedef struct SurfacePass_t
{
   UniformBlock uniform_blocks[SURF_MAX_BLOCKS_PER_PASS];
   Shader shader;
   u8 uniform_block_count;
   u8 cull_mode;
   u8 depth_mode;
   u8 blend_mode;
} SurfacePass;

typedef struct SurfaceDesc_t
{
   SurfacePass passes[SURF_MAX_PASSES];
   u32 pass_count;
   u8 texture_defaults[SURF_MAX_TEXTURES];
} SurfaceDesc;

typedef struct SurfaceTexture_t
{
   u32 bind_slot;
   Texture texture;
} SurfaceTexture;

typedef struct SurfaceMaterial_t
{
   Surface surface;
   SurfaceTexture textures[SURF_MAX_TEXTURES];
   u32 texture_count;

   void* uniform_block_data[SURF_MAX_PASSES][SURF_MAX_BLOCKS_PER_PASS];

} SurfaceMaterial;

typedef struct MaterialTexture_t
{
   Texture texture;
   u8 slot;
   u8 default_texture: 4;
   u8 wrap_mode: 4;
   u8 filter_mode: 7;
   u8 is_set: 1;

} MaterialTexture;

typedef struct MaterialData_t
{
   Shader shader;
   Buffer material_ubo;
   MaterialTexture textures[MATERIAL_MAX_TEXTURES];
   u32 texture_count;

} MaterialData;

typedef struct LightDesc_t
{
   vec3 origin;
   f32 radius;
   f32 spotlight_angle;
   color8 color;
   f32 brightness;
   f32 theta;
   f32 phi;
} LightDesc;

struct Renderer_t;
typedef void (*DrawableFunc)(struct Renderer_t* renderer, Drawable self);
typedef void (*DrawableRenderFunc)(struct Renderer_t* renderer, Drawable self, u32 pass_id);

typedef struct DrawableTypeDesc_t
{
   DrawableRenderFunc render_func;
   DrawableFunc on_enable_func;
   DrawableFunc on_disable_func;
   uS data_size;
} DrawableTypeDesc;

typedef struct Renderer_t Renderer;

typedef struct GeometryDrawable_t
{
   SurfaceMaterial material;
   Geometry geometry;
   color8 color;
   Transform3D transform;
   
} GeometryDrawable;

typedef struct LightDrawable_t
{
   u32 light_idx;
   vec3 origin;
   f32 radius;
   f32 spotlight_angle;
   color8 color;
   f32 brightness;
   f32 theta;
   f32 phi;
   
   struct {
      u32 is_dirty: 1;
   };

} LightDrawable;

Renderer* Renderer_Init(Graphics* graphics);
void Renderer_Free(Renderer* renderer);

Graphics* Renderer_Graphics(Renderer* renderer);
void Renderer_Render(Renderer* renderer, resolution2d size);

u32 Renderer_AddLight(Renderer* renderer, LightDesc desc);
void Renderer_RemoveLight(Renderer* renderer, u32 index);
void Renderer_UpdateLight(Renderer* renderer, u32 index, LightDesc desc);

void Renderer_SetTexture(Renderer* renderer, Texture texture, u32 bind_slot);
void Renderer_SetTextureDefault(Renderer* renderer, u8 texture_default, u32 bind_slot);
void Renderer_UseMaterialTextures(Renderer* renderer, SurfaceMaterial material);

void Renderer_UpdateModelData(Renderer* renderer, Transform3D transform, color8 color);

void Renderer_SetViewMatrix(Renderer* renderer, mat4x4 view);
void Renderer_SetProjectionMatrix(Renderer* renderer, mat4x4 projection);
void Renderer_SetViewAndProjectionMatrix(Renderer* renderer, mat4x4 view, mat4x4 projection);

mat4x4 Renderer_GetViewMatrix(Renderer* renderer);
mat4x4 Renderer_GetProjectionMatrix(Renderer* renderer);
mat4x4 Renderer_GetViewAndProjectionMatrix(Renderer* renderer);

Surface Renderer_AddSurface(Renderer* renderer, const char* name, const SurfaceDesc* desc);
void Renderer_RemoveSurface(Renderer* renderer, Surface res_surface);

Surface Renderer_GetSurface(Renderer* renderer, const char* name);
SurfacePass Renderer_GetSurfacePass(Renderer* renderer, Surface res_surface, u32 pass_id);
UniformBlockList Renderer_UseSurfaceMaterial(Renderer* renderer, Transform3D transform, SurfaceMaterial material, color8 color, u32 pass_id);

void Renderer_RegisterDrawableType(Renderer* renderer, const char* name, const DrawableTypeDesc* desc);

Drawable Renderer_CreateDrawable(Renderer* renderer, const char* drawable_type_name);
void Renderer_RemoveDrawable(Renderer* renderer, Drawable res_drawable);

void* Renderer_DrawableData(Renderer* renderer, Drawable res_drawable);

Buffer Renderer_CameraBuffer(Renderer* renderer);
Buffer Renderer_ModelBuffer(Renderer* renderer);

Shader Renderer_BasicShader(Renderer* renderer);
Geometry Renderer_PlaneGeometry(Renderer* renderer);
Geometry Renderer_BoxGeometry(Renderer* renderer);
Texture Renderer_WhiteTexture(Renderer* renderer);
Texture Renderer_GrayTexture(Renderer* renderer);
Texture Renderer_BlackTexture(Renderer* renderer);
Texture Renderer_NormalTexture(Renderer* renderer);

#endif
