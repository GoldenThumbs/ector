#ifndef ECT_RENDERERI_H
#define ECT_RENDERERI_H

#include "util/types.h"
#include "core/renderer.h"

typedef struct RNDR_DrawCall_t
{
   UniformData uniforms[SHD_MAX_UNIFORMS];
   u32 uniform_count;
   Texture textures[SHD_MAX_TEX_SLOTS];
   u32 texture_count;
   u32 shader_id;
   Geometry geometry;
} RNDR_DrawCall;

u32 RNDR_MipScaled(u32 size, i32 mip);

u32 RNDR_FilterType(FilterType filter);
u32 RNDR_WrapType(WrapType wrap);
u32 RNDR_TextureType(TextureType texture);

u8 RNDR_AttributeNormalized(AttributeFormat format);
i32 RNDR_AttributeSize(AttributeFormat format);
u32 RNDR_AttributeType(AttributeFormat format);

u32 RNDR_PixelInternalFormat(PixelFormat format);
u32 RNDR_PixelFormat(PixelFormat format);
u32 RNDR_PixelType(PixelFormat format);

u32 RNDR_TargetType(TargetType target_type);
u32 RNDR_TargetTypeCubemap(CubemapFace face);

uS RNDR_BytesPerPixel(PixelFormat format);

u32 RNDR_CubemapFace(CubemapFace face);

void RNDR_SetTexture(u32 slot, Texture texture);
void RNDR_SetTextureData(Texture texture, CubemapFace face, i32 mip, i32 samples, void* data);
void RNDR_AttachFramebufferTexture(u32 fbo, TargetType target_type, Texture texture, i32 mip, i32 layer, CubemapFace face);
void RNDR_SetUniform(u32 location, u32 count, UniformType type, void* data);
void RNDR_DrawGeometry(Geometry geometry);

void RNDR_MarkFramebufferDirty(Renderer* renderer);
void RNDR_InitFramebuffer(Renderer* renderer, size2i size);
void RNDR_BeginFrame(Renderer* renderer, vec4 clear_color, ClearTargets clear_targets);
void RNDR_RenderFrame(Renderer* renderer);
void RNDR_FinishFrame(Renderer* renderer, size2i render_size);

#endif
