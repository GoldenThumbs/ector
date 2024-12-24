#ifndef ECT_RENDERER_H
#define ECT_RENDERER_H

#include "util/types.h"

#include <limits.h>

enum {
   ERR_SHADER_COMPILE_FAILED = 1
};

#define ERR_SHADER_FAILED (1u<<0u)
#define ERR_SHADER_COMPUTE_FAILED (1u<<1u)
#define ERR_SHADER_VERTEX_FAILED (1u<<2u)
#define ERR_SHADER_GEOMETRY_FAILED (1u<<3u)
#define ERR_SHADER_FRAGMENT_FAILED (1u<<4u)

typedef enum PixelFormat_t
{
   PIX_R_8BPC = 0,
   PIX_RG_8BPC,
   PIX_RGB_8BPC,
   PIX_RGBA_8BPC,

   PIX_R_F16BPC,
   PIX_RG_F16BPC,
   PIX_RGB_F16BPC,
   PIX_RGBA_F16BPC,

   PIX_R_F32BPC,
   PIX_RG_F32BPC,
   PIX_RGB_F32BPC,
   PIX_RGBA_F32BPC,

   PIX_DEPTH_24BPC,
   PIX_DEPTH_F32BPC,
   PIX_DEPTH_STENCIL
} PixelFormat;

typedef enum TextureType_t
{
   TEX_1D = 0,
   TEX_2D,
   TEX_2D_MSAA,
   // TEX_2D_ARRAY,
   // TEX_2D_MSAA_ARRAY,
   TEX_3D,
   TEX_CUBEMAP
   // TEX_CUBEMAP_ARRAY
} TextureType;

typedef enum TargetType_t
{
   TRG_TEX_COLOR0 = 0,
   TRG_TEX_COLOR1,
   TRG_TEX_COLOR2,
   TRG_TEX_COLOR3,
   TRG_TEX_DEPTH
} TargetType;

typedef enum FilterType_t
{
   FTR_NEAREST_NO_MIPMAP = 0,
   FTR_NEAREST_MIP_NEAREST,
   FTR_NEAREST_MIP_LINEAR,
   FTR_LINEAR_NO_MIPMAP,
   FTR_LINEAR_MIP_NEAREST,
   FTR_LINEAR_MIP_LINEAR,
} FilterType;

typedef enum WrapType_t
{
   WRP_REPEAT,
   WRP_CLAMP
} WrapType;

typedef enum CubemapFace_t
{
   CBE_LEFT = 0, // -X Cube Face
   CBE_RIGHT,    // +X Cube Face
   CBE_BOTTOM,   // -Y Cube Face
   CBE_TOP,      // +Y Cube Face
   CBE_FRONT,    // -Z Cube Face
   CBE_BACK      // +Z Cube Face
} CubemapFace;

typedef enum IndexType_t
{
   IDX_NONE = 0,
   IDX_U16,
   IDX_U32
} IndexType;

typedef enum ShaderStage_t
{
   SHD_COMPUTE = 0,
   SHD_VERTEX,
   SHD_GEOMTRY,
   SHD_FRAGMENT
} ShaderStage;

#define SHD_MAX_STAGES 4

typedef enum UniformType_t
{
   UNI_NULL_TYPE = UINT32_MAX,
   UNI_F32 = 0,
   UNI_F32_V2,
   UNI_F32_V3,
   UNI_F32_V4,

   UNI_I32,
   UNI_I32_V2,
   UNI_I32_V3,
   UNI_I32_V4,

   UNI_U32,
   UNI_U32_V2,
   UNI_U32_V3,
   UNI_U32_V4,

   UNI_MATRIX,
   UNI_SAMPLER
} UniformType;

typedef enum AttributeFormat_t
{
   ATR_F32 = 0,
   ATR_F32_V2,
   ATR_F32_V3,
   ATR_F32_V4,

   ATR_I16,
   ATR_I16_V2,
   ATR_I16_V3,
   ATR_I16_V4,

   ATR_IN16,
   ATR_IN16_V2,
   ATR_IN16_V3,
   ATR_IN16_V4,

   ATR_U16,
   ATR_U16_V2,
   ATR_U16_V3,
   ATR_U16_V4,

   ATR_UN16,
   ATR_UN16_V2,
   ATR_UN16_V3,
   ATR_UN16_V4,

   ATR_I32,
   ATR_I32_V2,
   ATR_I32_V3,
   ATR_I32_V4,

   ATR_IN32,
   ATR_IN32_V2,
   ATR_IN32_V3,
   ATR_IN32_V4,

   ATR_U32,
   ATR_U32_V2,
   ATR_U32_V3,
   ATR_U32_V4,

   ATR_UN32,
   ATR_UN32_V2,
   ATR_UN32_V3,
   ATR_UN32_V4
} AttributeFormat;

typedef struct ClearTargets_t
{
   bool color;
   bool depth;
   bool stencil;
} ClearTargets;

typedef struct View_t
{
   mat4x4 view_proj;
   size2i frame_size;
} View;

typedef struct Buffer_t
{
   const void* data;
   u32 size;
   u32 count;
} Buffer;

typedef struct ImageDesc_t
{
   size2i size;
   u32 depth;
   u32 mipmaps;
   TextureType type;
   PixelFormat format;
} ImageDesc;

typedef struct Image_t
{
   void* data;
   size2i size;
   u32 depth;
   u32 mipmaps;
   TextureType type;
   PixelFormat format;
} Image;

typedef struct TextureSampler_t
{
   struct {
      FilterType minify;
      FilterType magnify;
   } filtering;
   struct {
      WrapType s;
      WrapType t;
   } wrapping;
} TextureSampler;

typedef struct Texture_t
{
   size2i size;
   u32 depth;
   u32 mipmaps;
   TextureType type;
   PixelFormat format;
   u32 id;
   error err;
} Texture;

typedef struct TargetTextureDesc_t
{
   size2i size;
   u32 depth;
   u32 mipmaps;
   TextureType type;
   PixelFormat format;
   TargetType target_type;
} TargetTextureDesc;

#define TRG_MAX_TEXTURES 4

typedef struct TargetDesc_t
{
   u32 samples;
   u32 texture_count;
   TargetTextureDesc textures[TRG_MAX_TEXTURES];
} TargetDesc;

typedef struct Target_t
{
   u32 samples;
   u32 texture_count;
   Texture textures[TRG_MAX_TEXTURES];
   u32 id;
   error err;
} Target;

typedef struct UniformDesc_t
{
   memblob datablob;
   const char* name;
   u32 location;
   UniformType type;
} UniformDesc;

typedef struct UniformData_t
{
   memblob datablob;
   u32 location;
   UniformType type;
} UniformData;

#define SHD_MAX_UNIFORMS 64
#define SHD_MAX_TEX_SLOTS 16

typedef struct ShaderDesc_t
{
   const char* stage_src[SHD_MAX_STAGES];
   u32 uniform_count;
} ShaderDesc;

typedef struct Shader_t
{
   u32 stage_id[SHD_MAX_STAGES];
   u32 id;
   error err;
} Shader;

typedef struct VertexDesc_t
{
   u32 count;
   IndexType index_type;
   struct {
      u32 offset;
      AttributeFormat format;
   } attribute[8];
} VertexDesc;

typedef struct Geometry_t
{
   u32 id;
   struct {
      u32 id;
      u32 count;
   } vertices;

   struct {
      u32 id;
      u32 count;
   } indices;

   IndexType index_type;
   u32 vertex_size;
} Geometry;

typedef struct DrawCall_t
{
   UniformDesc uniforms[SHD_MAX_UNIFORMS];
   u32 uniform_count;
   Texture textures[SHD_MAX_TEX_SLOTS];
   u32 texture_count;
   Shader shader;
   Geometry geometry;
} DrawCall;

typedef struct RendererDesc_t
{
   PixelFormat color_format;
   PixelFormat depth_format;
   u32 samples;
   vec2 render_scale;
} RendererDesc;

typedef struct Renderer_t Renderer;

Renderer* Renderer_Init(RendererDesc* renderer_desc);
void Renderer_Free(Renderer* renderer);

size2i Renderer_ScaleFrameSize(Renderer* renderer, size2i frame_size);
void Renderer_SetFrameSize(Renderer* renderer, size2i size);
size2i Renderer_GetFrameSize(Renderer* renderer);
void Renderer_SetViewProjMatrix(Renderer* renderer, mat4x4 view_proj);
mat4x4 Renderer_GetViewProjMatrix(Renderer* renderer);

void Renderer_SubmitDrawCall(Renderer* renderer, DrawCall drawcall);

Texture Renderer_CreateTexture(Image image);
void Renderer_FreeTexture(Texture* texture);
void Renderer_TextureSampler(Texture texture, TextureSampler sampler);

Target Renderer_CreateTarget(TargetDesc target_desc);
void Renderer_FreeTarget(Target* target);
void Renderer_BeginTarget(Target target);
void Renderer_FinishTarget(void);

Shader Renderer_CreateShader(const ShaderDesc shader_desc);
void Renderer_FreeShader(Shader* shader);

Geometry Renderer_CreateGeometry(const Buffer vertices, const Buffer indices, const VertexDesc vertex_desc);
void Renderer_FreeGeometry(Geometry* geometry);

void Renderer_Clear(vec4 rgba, ClearTargets clear_targets);
void Renderer_SetViewport(size2i viewport_size, i32 offset_x, i32 offset_y);

#endif
