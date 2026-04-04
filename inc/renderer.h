#ifndef ECT_RENDERER_H
#define ECT_RENDERER_H

#include "util/types.h"
#include "util/extra_types.h"
#include "mesh.h"
#include "graphics.h"

#define RENDERER_MODULE "Renderer"

#define EMPTY_DRAWABLE_TYPE "EmptyDrawable"
#define GEOMETRY_DRAWABLE_TYPE "GeometryDrawable"

#define SURF_MAX_PASSES 4
#define SURF_MAX_BLOCKS_PER_PASS 4
#define SURF_MAX_TEXTURES 8

#define LIGHTMANAGER_MAX_DEFINES 32

enum {
   RNDR_SURF_TEXTURE_WHITE = 0,
   RNDR_SURF_TEXTURE_GRAY,
   RNDR_SURF_TEXTURE_BLACK,
   RNDR_SURF_TEXTURE_NORMAL,

   RNDR_SURF_DEFAULT_TEXTURE_COUNT

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
   f32 u_near_clip;
   f32 u_far_clip;
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

   union {
      TextureInterpolation interpolation_settings;

      struct {
         u16 anisotropy;
         u8 filter;
         u8 wrap;
      };

   };

} SurfaceTexture;

typedef struct SurfaceMaterial_t
{
   Surface surface;
   SurfaceTexture textures[SURF_MAX_TEXTURES];
   u32 texture_count;

   void* uniform_block_data[SURF_MAX_PASSES][SURF_MAX_BLOCKS_PER_PASS];

} SurfaceMaterial;

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

typedef struct ShaderDefines_t
{
   u32 define_count;
   char* defines[LIGHTMANAGER_MAX_DEFINES];
   
} ShaderDefines;

struct LightManagerInfo_t;
typedef error (*LightManagerFunc)(struct Renderer_t* renderer);
typedef ShaderDefines (*LightManagerDefines)(struct Renderer_t* renderer);

typedef struct LightManagerInfo_t
{
   union {
      u64 id;
      char id_string[8]; // 8 character identifier string

   };

   LightManagerFunc lightman_init;
   LightManagerFunc lightman_free;
   LightManagerFunc lightman_prerender;
   LightManagerFunc lightman_on_render;
   LightManagerDefines lightman_defs;
   void* data;

} LightManagerInfo;

typedef struct GeometryDrawable_t
{
   SurfaceMaterial material;
   Geometry geometry;
   color8 color;
   Transform3D transform;
   
} GeometryDrawable;


typedef struct Renderer_t Renderer;

Renderer* Renderer_Init(Graphics* graphics, const char* app_path);
void Renderer_Free(Renderer* renderer);

Graphics* Renderer_Graphics(Renderer* renderer);
void Renderer_RenderPass(Renderer* renderer, resolution2d size, f64 engine_frame_delta, u32 pass_id);

void Renderer_SetTexture(Renderer* renderer, Texture texture, u32 bind_slot);
void Renderer_SetTextureToDefault(Renderer* renderer, u8 texture_default, u32 bind_slot);
void Renderer_UseMaterialTextures(Renderer* renderer, SurfaceMaterial material);
void Renderer_ReserveTexture(Renderer* renderer, u32 bind_slot);
void Renderer_UnreserveTexture(Renderer* renderer, u32 bind_slot);

void Renderer_UpdateModelData(Renderer* renderer, Transform3D transform, color8 color);

void Renderer_SetViewMatrix(Renderer* renderer, mat4x4 view);
void Renderer_SetProjectionMatrix(Renderer* renderer, mat4x4 proj);
void Renderer_SetFieldOfView(Renderer* renderer, f32 vertical_fov);
void Renderer_SetClippingPlanes(Renderer* renderer, f32 near_clip, f32 far_clip);
void Renderer_UpdateCamera(Renderer* renderer, vec3 origin, vec3 euler, f32 distance);

mat4x4 Renderer_GetViewMatrix(Renderer* renderer);
mat4x4 Renderer_GetProjectionMatrix(Renderer* renderer);
mat4x4 Renderer_GetViewAndProjectionMatrix(Renderer* renderer);

Surface Renderer_AddSurface(Renderer* renderer, const char* name, const SurfaceDesc* desc);
void Renderer_RemoveSurface(Renderer* renderer, Surface res_surface);

Surface Renderer_GetSurface(Renderer* renderer, const char* name);
SurfacePass Renderer_GetSurfacePass(Renderer* renderer, Surface res_surface, u32 pass_id);
UniformBlockList Renderer_UseSurfaceMaterial(Renderer* renderer, Transform3D transform, SurfaceMaterial material, color8 color, u32 pass_id);

void Renderer_RegisterDrawableType(Renderer* renderer, const char* name, const DrawableTypeDesc* desc);
u16 Renderer_GetDrawableTypeIndexFromName(Renderer* renderer, const char* drawable_type_name);

Drawable Renderer_CreateDrawable(Renderer* renderer, const char* drawable_type_name);
void Renderer_RemoveDrawable(Renderer* renderer, Drawable res_drawable);

void* Renderer_DrawableData(Renderer* renderer, Drawable res_drawable);
void* Renderer_GetDrawableDataFromIndex(Renderer* renderer, u16 drawable_type_idx, u16 drawable_idx);

Buffer Renderer_CameraBuffer(Renderer* renderer);
Buffer Renderer_ModelBuffer(Renderer* renderer);

void Renderer_SetUnlitShader(Renderer* renderer, Shader shader);
void Renderer_SetBasicShader(Renderer* renderer, Shader shader);

Shader Renderer_UnlitShader(Renderer* renderer);
Shader Renderer_BasicShader(Renderer* renderer);
Geometry Renderer_PlaneGeometry(Renderer* renderer);
Geometry Renderer_BoxGeometry(Renderer* renderer);
Texture Renderer_GetDefaultTexture(Renderer* renderer, u8 texture_default);
Texture Renderer_WhiteTexture(Renderer* renderer);
Texture Renderer_GrayTexture(Renderer* renderer);
Texture Renderer_BlackTexture(Renderer* renderer);
Texture Renderer_NormalTexture(Renderer* renderer);

void* Renderer_LightManager(Renderer* renderer);
LightManagerInfo* Renderer_LightManagerInfo(Renderer* renderer);
void Renderer_SetLightManager(Renderer* renderer, LightManagerInfo lightmanager_info);
bool Renderer_IsLightManagerValid(Renderer* renderer, const u64 desired_id);

Texture Renderer_CreateColorTexture(Renderer* renderer, color8 color, u8 texture_type);
Texture Renderer_LoadTexture(Renderer* renderer, const char* texture_file_path, resolution2d slice_size, bool generate_mipmaps, bool is_srgb);
Shader Renderer_LoadShader(Renderer* renderer, const char* shader_file_path, const char* defines[], const u32 defines_count, bool is_compute);
Model Renderer_LoadModel(Renderer* renderer, const char* model_file_path);

void Renderer_SetSurfaceMaterialTextureAdvanced(SurfaceMaterial* material, i32 index, i32 bind_slot, Texture texture, TextureInterpolation interpolation_settings);
void Renderer_SetSurfaceMaterialTexture(SurfaceMaterial* material, i32 index, i32 bind_slot, Texture texture);

#endif
