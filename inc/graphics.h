#ifndef ECT_GRAPHICS_H
#define ECT_GRAPHICS_H

#include "util/types.h"
#include "mesh.h"
#include "image.h"

#define GFX_INVALID_INDEX UINT32_MAX

#define GRAPHICS_MODULE "Graphics"

typedef handle Shader;
typedef handle Buffer;
typedef handle Geometry;
typedef handle Texture;
typedef handle Framebuffer;

enum {
   ERR_GFX_CONTEXT_FAILED = 1,
   ERR_GFX_SHADER_COMPILATION_FAILED,
   ERR_GFX_FRAMEBUFFER_ATTACHMENT_FAILED

};

#define ERR_INFO_GFX_INVALID_FRAMEBUFFER_ATTACHMENT (1u << 0u)

enum {
   GFX_INDEXTYPE_16BIT = 0,
   GFX_INDEXTYPE_32BIT

};

enum {
   GFX_DRAWMODE_STATIC = 0,
   GFX_DRAWMODE_DYNAMIC,
   GFX_DRAWMODE_STREAM,
   GFX_DRAWMODE_STATIC_READ,
   GFX_DRAWMODE_STATIC_COPY,
   GFX_DRAWMODE_DYNAMIC_READ,
   GFX_DRAWMODE_DYNAMIC_COPY,
   GFX_DRAWMODE_STREAM_READ,
   GFX_DRAWMODE_STREAM_COPY

};

enum {
   GFX_BUFFERTYPE_UNIFORM = 0,
   GFX_BUFFERTYPE_STORAGE

};

enum {
   GFX_FACECULL_BACK = 0,
   GFX_FACECULL_FRONT,
   GFX_FACECULL_NONE

};

enum {
   GFX_PRIMITIVE_POINT = 0,
   GFX_PRIMITIVE_LINE,
   GFX_PRIMITIVE_TRIANGLE

};

enum {
   GFX_ATTRIBUTE_NULL = 0,

   GFX_ATTRIBUTE_F32_1X,
   GFX_ATTRIBUTE_F32_2X,
   GFX_ATTRIBUTE_F32_3X,
   GFX_ATTRIBUTE_F32_4X,

   GFX_ATTRIBUTE_U8_4X_NORM

};

enum {
   GFX_UNIFORMTYPE_F32_1X = 0,
   GFX_UNIFORMTYPE_F32_2X,
   GFX_UNIFORMTYPE_F32_3X,
   GFX_UNIFORMTYPE_F32_4X,
   GFX_UNIFORMTYPE_MAT3,
   GFX_UNIFORMTYPE_MAT4,
   GFX_UNIFORMTYPE_U32_1X,
   GFX_UNIFORMTYPE_U32_2X,
   GFX_UNIFORMTYPE_U32_3X,
   GFX_UNIFORMTYPE_U32_4X,
   GFX_UNIFORMTYPE_TEX_SLOT

};

enum {
   GFX_TEXTURETYPE_2D = 0,
   GFX_TEXTURETYPE_3D,
   GFX_TEXTURETYPE_CUBEMAP,

   GFX_TEXTURETYPE_2D_ARRAY,
   GFX_TEXTURETYPE_CUBEMAP_ARRAY

};

enum {
   GFX_CUBEMAPFACE_POSITIVE_X = 0,
   GFX_CUBEMAPFACE_NEGATIVE_X,
   GFX_CUBEMAPFACE_POSITIVE_Y,
   GFX_CUBEMAPFACE_NEGATIVE_Y,
   GFX_CUBEMAPFACE_POSITIVE_Z,
   GFX_CUBEMAPFACE_NEGATIVE_Z

};

#define GFX_CUBEMAPFACE_NONE GFX_CUBEMAPFACE_POSITIVE_X

enum {
   // Standard Color Formats
   GFX_TEXTUREFORMAT_R_U8_NORM = 0,
   GFX_TEXTUREFORMAT_RG_U8_NORM,
   GFX_TEXTUREFORMAT_RGB_U8_NORM,
   GFX_TEXTUREFORMAT_RGBA_U8_NORM,

   GFX_TEXTUREFORMAT_R_U16_NORM = 4,
   GFX_TEXTUREFORMAT_RG_U16_NORM,
   GFX_TEXTUREFORMAT_RGB_U16_NORM,
   GFX_TEXTUREFORMAT_RGBA_U16_NORM,

   GFX_TEXTUREFORMAT_R_F16 = 8,
   GFX_TEXTUREFORMAT_RG_F16,
   GFX_TEXTUREFORMAT_RGB_F16,
   GFX_TEXTUREFORMAT_RGBA_F16,

   GFX_TEXTUREFORMAT_R_F32 = 12,
   GFX_TEXTUREFORMAT_RG_F32,
   GFX_TEXTUREFORMAT_RGB_F32,
   GFX_TEXTUREFORMAT_RGBA_F32,

   // Special Formats
   GFX_TEXTUREFORMAT_R11F_G11F_B10F,

   // Depth and Stencil Formats
   GFX_TEXTUREFORMAT_DEPTH_16,
   GFX_TEXTUREFORMAT_DEPTH_24,
   GFX_TEXTUREFORMAT_DEPTH_F32,
   GFX_TEXTUREFORMAT_DEPTH_24_STENCIL_8,
   GFX_TEXTUREFORMAT_DEPTH_F32_STENCIL_8,

   // 8 bits per channel sRGB Formats
   GFX_TEXTUREFORMAT_SRGB,
   GFX_TEXTUREFORMAT_SRGB_ALPHA,

   // Block Compression Formats
   GFX_TEXTUREFORMAT_RGB_BC1,
   GFX_TEXTUREFORMAT_RGBA_BC1,
   GFX_TEXTUREFORMAT_RGBA_BC2,
   GFX_TEXTUREFORMAT_RGBA_BC3,
   GFX_TEXTUREFORMAT_RGBA_BC7,
   GFX_TEXTUREFORMAT_SRGB_BC1,
   GFX_TEXTUREFORMAT_SRGB_ALPHA_BC1,
   GFX_TEXTUREFORMAT_SRGB_ALPHA_BC2,
   GFX_TEXTUREFORMAT_SRGB_ALPHA_BC3,
   GFX_TEXTUREFORMAT_SRGB_ALPHA_BC7,

   // High Dynamic Range Block Compression Formats
   GFX_TEXTUREFORMAT_RGB_SIGNED_BC6,
   GFX_TEXTUREFORMAT_RGB_UNSIGNED_BC6

};

enum {
   GFX_TEXTUREFILTER_POINT_NO_MIPMAPS = 0,
   GFX_TEXTUREFILTER_POINT_NEAREST_MIPMAPS,
   GFX_TEXTUREFILTER_POINT_LINEAR_MIPMAPS,
   GFX_TEXTUREFILTER_BILINEAR_NO_MIPMAPS,
   GFX_TEXTUREFILTER_BILINEAR_NEAREST_MIPMAPS,
   GFX_TEXTUREFILTER_BILINEAR_LINEAR_MIPMAPS,
   GFX_TEXTUREFILTER_POINT_MAX_BILINEAR_MIN

};

enum {
   GFX_TEXTUREWRAP_REPEAT = 0,
   GFX_TEXTUREWRAP_REPEAT_MIRRORED,
   GFX_TEXTUREWRAP_CLAMP

};

enum {
   GFX_BLENDMODE_MIX = 0,
   GFX_BLENDMODE_ADD,
   GFX_BLENDMODE_SUB,
   GFX_BLENDMODE_MUL,
   GFX_BLENDMODE_NONE

};

enum {
   GFX_DEPTHMODE_LESS_THAN = 0,
   GFX_DEPTHMODE_LESS_THAN_OR_EQUAL,
   GFX_DEPTHMODE_GREATER_THAN,
   GFX_DEPTHMODE_GREATER_THAN_OR_EQUAL,
   GFX_DEPTHMODE_EQUAL_TO,
   GFX_DEPTHMODE_NOT_EQUAL_TO,
   GFX_DEPTHMODE_ALWAYS_OVER,
   GFX_DEPTHMODE_NONE

};

enum {
   GFX_ACCESS_READ = 0,
   GFX_ACCESS_WRITE,
   GFX_ACCESS_READ_AND_WRITE

};

typedef struct AdvancedBindOptions_t
{
   u32 mip_level;
   i32 layer;
   u8 cubemap_face;
   u8 access_type;

} AdvancedBindOptions;

typedef struct Uniform_t
{
   u32 uniform_type;
   u32 location;

   union {
      f32 as_float[16];
      vec2 as_vec2;
      vec3 as_vec3;
      vec4 as_vec4;
      mat3x3 as_mat3;
      mat4x4 as_mat4;

      u32 as_uint[4];
      i32 texslot;

   };

} Uniform;

typedef struct UniformBlock_t
{
   uS size;
   u32 binding;
   Buffer ubo;

} UniformBlock;

#define GFX_MAX_UNIFORM_BLOCKS 8

typedef struct UniformBlockList_t
{
   UniformBlock blocks[GFX_MAX_UNIFORM_BLOCKS];
   u32 count;

} UniformBlockList;

typedef struct TextureDesc_t
{
   resolution2d size;
   i32 depth;
   u16 mipmap_count;
   u8 texture_type;
   u8 texture_format;

} TextureDesc;

typedef struct TextureInterpolation_t
{
   u16 texture_anisotropy;
   u8 texture_filter;
   u8 texture_wrap;

} TextureInterpolation;

typedef struct Graphics_t Graphics;

Graphics* Graphics_Init(void);
void Graphics_Free(Graphics* graphics);
void Graphics_CheckErrors(Graphics* graphics);

Shader Graphics_CreateShader(Graphics* graphics, const char* vertex_shader, const char* fragment_shader);
Shader Graphics_CreateComputeShader(Graphics* graphics, const char* compute_shader);
Shader Graphics_LoadShaderFromFile(Graphics* graphics, const char* file_path, const char* defines[], const u32 define_count, bool is_compute);
void Graphics_FreeShader(Graphics* graphics, Shader res_shader);
void Graphics_SetUniform(Graphics* graphics, Uniform uniform);
void Graphics_Dispatch(Graphics* graphics, Shader res_shader, u32 size_x, u32 size_y, u32 size_z, UniformBlockList uniform_blocks);
void Graphics_DispatchBarrier(Graphics* graphics);

Buffer Graphics_CreateBuffer(Graphics* graphics, void* data, u32 length, uS type_size, u8 draw_mode, u8 buffer_type);
Buffer Graphics_CreateBufferExplicit(Graphics* graphics, void* data, uS total_size, u8 draw_mode, u8 buffer_type);
void Graphics_FreeBuffer(Graphics* graphics, Buffer res_buffer);
void Graphics_UpdateBuffer(Graphics* graphics, Buffer res_buffer, void* data, u32 length, uS type_size);
void Graphics_UpdateBufferRange(Graphics* graphics, Buffer res_buffer, void* data, u32 offset, u32 length, uS type_size);
void Graphics_UpdateBufferExplicit(Graphics* graphics, Buffer res_buffer, void* data, uS offset_bytes, uS total_size);
void Graphics_BindBuffer(Graphics* graphics, Buffer res_buffer, u32 slot);

Geometry Graphics_CreateGeometry(Graphics* graphics, Mesh mesh, u8 draw_mode);
void Graphics_FreeGeometry(Graphics* graphics, Geometry res_geometry);
void Graphics_SetGeometryFaceCullMode(Graphics* graphics, Geometry res_geometry, u8 face_cull_mode);

Texture Graphics_CreateTexture(Graphics* graphics, u8* data, TextureDesc desc);
void Graphics_FreeTexture(Graphics* graphics, Texture res_texture);
void Graphics_UpdateTexture(Graphics* graphics, u8* data, Texture res_texture);
void Graphics_BindTexture(Graphics* graphics, Texture res_texture, u32 bind_slot);
void Graphics_BindTextureView(Graphics* graphics, Texture res_texture, u32 bind_slot, const AdvancedBindOptions* bind_options);
void Graphics_UnbindTextures(Graphics* graphics, u8 texture_type);
void Graphics_SetTextureInterpolation(Graphics* graphics, Texture res_texture, TextureInterpolation interpolation_settings);

// NOTE: this function allocates memory.
Image Graphics_GetTextureImageData(Graphics* graphics, Texture res_texture, u32 mip_level, u8 cubemap_face);

Framebuffer Graphics_CreateFramebuffer(Graphics* graphics, resolution2d size, bool depthstencil_renderbuffer);
void Graphics_FreeFramebuffer(Graphics* graphics, Framebuffer res_framebuffer);
void Graphics_DrawToFramebufferTargets(Graphics* graphics, Framebuffer res_framebuffer, u32 target_count, u8 target_ids[]);
void Graphics_BindFramebuffer(Graphics* graphics, Framebuffer res_framebuffer);
void Graphics_UnbindFramebuffers(Graphics *graphics);
void Graphics_AttachMultipleTexturesToFramebuffer(Graphics* graphics, Framebuffer res_framebuffer, u32 texture_count, Texture res_textures[]);
void Graphics_AttachTextureToFramebuffer(Graphics* graphics, Framebuffer res_framebuffer, Texture res_texture, const AdvancedBindOptions* bind_options, u8 attachment_slot);

void Graphics_Clear(Graphics* graphics);
void Graphics_Viewport(Graphics* graphics, resolution2d size);
void Graphics_OffsetViewport(Graphics* graphics, resolution2d size, i32 offset_x, i32 offset_y);
void Graphics_EnableColorClear(Graphics* graphics, bool enable_color_clear);
void Graphics_EnableDepthClear(Graphics* graphics, bool enable_depth_clear);
void Graphics_EnableStencilClear(Graphics* graphics, bool enable_stencil_clear);
void Graphics_SetClearColor(Graphics* graphics, color8 clear_color);
void Graphics_SetClearDepth(Graphics* graphics, f32 clear_depth);
void Graphics_SetClearStencilId(Graphics* graphics, i32 clear_stencil_id);
void Graphics_SetBlending(Graphics* graphics, u8 blend_mode);
void Graphics_SetDepthTest(Graphics* graphics, u8 depth_mode);
void Graphics_SetDepthMask(Graphics* graphics, bool depth_mask);

void Graphics_Draw(Graphics* graphics, Shader res_shader, Geometry res_geometry, UniformBlockList uniform_blocks);
void Graphics_DrawInstanced(Graphics* graphics, Shader res_shader, Geometry res_geometry, u32 instance_count, UniformBlockList uniform_blocks);

#endif
