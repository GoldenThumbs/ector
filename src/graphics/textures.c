#include "util/types.h"
#include "util/math.h"
#include "util/array.h"
#include "util/resource.h"

#include "graphics.h"
#include "graphics/internal.h"

#include <glad/gl.h>
#include <stdlib.h>

Texture Graphics_CreateTexture(Graphics* graphics, u8* data, TextureDesc desc)
{
   gfx_Texture texture = { 0 };
   texture.width = M_MAX(0, desc.size.width);
   texture.height = M_MAX(0, desc.size.height);
   texture.depth = M_MAX(0, desc.depth);
   texture.mipmap_count = M_MAX(1u, desc.mipmap_count);
   texture.type = desc.texture_type;
   texture.format = desc.texture_format;
   texture.compare.ref = graphics->ref;

   GFX_CreateTexture(&texture, data);

   texture.compare.handle = Util_ArrayLength(graphics->textures);

   return Util_AddResource(&graphics->ref, REF(graphics->textures), &texture);
}

void Graphics_ReuseTexture(Graphics* graphics, u8* data, TextureDesc desc, Texture res_texture)
{
   gfx_Texture texture = graphics->textures[res_texture.handle];
   if (texture.compare.ref != res_texture.ref)
      return;

   glDeleteTextures(1, &texture.id.tex);

   texture.width = M_MAX(0, desc.size.width);
   texture.height = M_MAX(0, desc.size.height);
   texture.depth = M_MAX(0, desc.depth);
   texture.mipmap_count = M_MAX(1u, desc.mipmap_count);
   texture.type = desc.texture_type;
   texture.format = desc.texture_format;

   GFX_CreateTexture(&texture, data);
}

void Graphics_FreeTexture(Graphics* graphics, Texture res_texture)
{
   gfx_Texture texture = graphics->textures[res_texture.handle];
   if (texture.compare.ref != res_texture.ref)
      return;

   glDeleteTextures(1, &texture.id.tex);
}

void Graphics_BindTexture(Graphics *graphics, Texture res_texture, u32 bind_slot)
{
   gfx_Texture texture = graphics->textures[res_texture.handle];
   if (texture.compare.ref != res_texture.ref)
      return;

   u32 gl_target = GFX_TextureType(texture.type);

   glActiveTexture(GL_TEXTURE0 + bind_slot);
   glBindTexture(gl_target, texture.id.tex);
}

void Graphics_UnbindTextures(Graphics* graphics, u8 texture_type)
{
   u32 gl_target = GFX_TextureType(texture_type);

   glBindTexture(gl_target, 0);
}

void Graphics_SetTextureInterpolation(Graphics* graphics, Texture res_texture, TextureInterpolation interpolation_settings)
{
   gfx_Texture texture = graphics->textures[res_texture.handle];
   if (texture.compare.ref != res_texture.ref)
      return;

   u32 gl_target = GFX_TextureType(texture.type);

   glBindTexture(gl_target, texture.id.tex);

   u32 wrap = GFX_TextureWrap(interpolation_settings.texture_wrap);
   struct gfx_Filtering_s filter = GFX_TextureFilter(interpolation_settings.texture_filter);

   glTexParameteri(gl_target, GL_TEXTURE_WRAP_R, wrap);
   glTexParameteri(gl_target, GL_TEXTURE_WRAP_S, wrap);
   glTexParameteri(gl_target, GL_TEXTURE_WRAP_T, wrap);
   glTexParameteri(gl_target, GL_TEXTURE_MIN_FILTER, filter.min_filter);
   glTexParameteri(gl_target, GL_TEXTURE_MAG_FILTER, filter.mag_filter);

   glBindTexture(gl_target, 0);
}

void Graphics_GenerateTextureMipmaps(Graphics* graphics, Texture res_texture)
{
   gfx_Texture texture = graphics->textures[res_texture.handle];
   if (texture.compare.ref != res_texture.ref)
      return;

   u32 gl_target = GFX_TextureType(texture.type);

   glBindTexture(gl_target, texture.id.tex);
   glGenerateMipmap(gl_target);
   glTexParameteri(gl_target, GL_TEXTURE_MAX_LEVEL, -1);
   glBindTexture(gl_target, 0);
}

Framebuffer Graphics_CreateFramebuffer(Graphics* graphics, resolution2d size, bool depthstencil_renderbuffer)
{
   gfx_Framebuffer framebuffer = { 0 };
   framebuffer.compare.ref = graphics->ref;

   glGenFramebuffers(1, &framebuffer.id.fbo);
   
   glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id.fbo);
   glGenRenderbuffers(1, &framebuffer.id.rbo);

   if (depthstencil_renderbuffer)
   {
      glBindRenderbuffer(GL_RENDERBUFFER, framebuffer.id.rbo);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size.width, size.height);

      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, framebuffer.id.rbo);
   }

   if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
   {
      return (handle){ 0 };
   }

   framebuffer.compare.handle = Util_ArrayLength(graphics->textures);

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);

   return Util_AddResource(&graphics->ref, REF(graphics->framebuffers), &framebuffer);
}

void Graphics_ReuseFramebuffer(Graphics* graphics, resolution2d size, bool depthstencil_renderbuffer, Framebuffer res_framebuffer)
{
   gfx_Framebuffer framebuffer = graphics->framebuffers[res_framebuffer.handle];
   if (framebuffer.compare.ref != res_framebuffer.ref)
      return;

   glDeleteFramebuffers(1, &framebuffer.id.fbo);
   if (framebuffer.id.rbo != 0)
      glDeleteRenderbuffers(1, &framebuffer.id.rbo);

   framebuffer.id.fbo = 0;
   framebuffer.id.rbo = 0;

   glGenFramebuffers(1, &framebuffer.id.fbo);
   
   glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id.fbo);
   glGenRenderbuffers(1, &framebuffer.id.rbo);

   if (depthstencil_renderbuffer)
   {
      glBindRenderbuffer(GL_RENDERBUFFER, framebuffer.id.rbo);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size.width, size.height);

      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, framebuffer.id.rbo);
   }

   if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
   {
      framebuffer.id.fbo = 0;
      framebuffer.id.rbo = 0;

      return;
   }

   framebuffer.compare.handle = Util_ArrayLength(graphics->textures);

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void Graphics_FreeFramebuffer(Graphics* graphics, Framebuffer res_framebuffer)
{
   gfx_Framebuffer framebuffer = graphics->framebuffers[res_framebuffer.handle];
   if (framebuffer.compare.ref != res_framebuffer.ref)
      return;

   glDeleteFramebuffers(1, &framebuffer.id.fbo);
   if (framebuffer.id.rbo != 0)
      glDeleteRenderbuffers(1, &framebuffer.id.rbo);
}

void Graphics_BindFramebuffer(Graphics* graphics, Framebuffer res_framebuffer)
{
   gfx_Framebuffer framebuffer = graphics->framebuffers[res_framebuffer.handle];
   if (framebuffer.compare.ref != res_framebuffer.ref)
      return;

   glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id.fbo);
   glBindRenderbuffer(GL_RENDERBUFFER, framebuffer.id.rbo);
}

void Graphics_UnbindFramebuffers(Graphics *graphics)
{
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void Graphics_AttachTexturesToFramebuffer(Graphics* graphics, Framebuffer res_framebuffer, u32 texture_count, Texture res_textures[])
{
   gfx_Framebuffer framebuffer = graphics->framebuffers[res_framebuffer.handle];
   if (framebuffer.compare.ref != res_framebuffer.ref)
      return;
   
   glBindTexture(GL_TEXTURE_2D, 0);

   glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id.fbo);
   glBindRenderbuffer(GL_RENDERBUFFER, framebuffer.id.rbo);

   for (u32 i=0; i<texture_count; i++)
   {
      gfx_Texture texture = graphics->textures[res_textures[i].handle];
      if (texture.compare.ref != res_textures[i].ref)
         continue;

      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GFX_TextureType(texture.type), texture.id.tex, 0);
   }

   if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      return;

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

uS GFX_PixelSize(u8 format)
{
   switch (format)
   {
      case GFX_TEXTUREFORMAT_R_U8_NORM:
         return 8;
      case GFX_TEXTUREFORMAT_RG_U8_NORM:
         return 16;
      case GFX_TEXTUREFORMAT_RGB_U8_NORM:
      case GFX_TEXTUREFORMAT_SRGB:
         return 24;
      case GFX_TEXTUREFORMAT_RGBA_U8_NORM:
      case GFX_TEXTUREFORMAT_SRGB_ALPHA:
         return 32;
      
      case GFX_TEXTUREFORMAT_R_U16_NORM:
         return 16;
      case GFX_TEXTUREFORMAT_RG_U16_NORM:
         return 32;
      case GFX_TEXTUREFORMAT_RGB_U16_NORM:
         return 48;
      case GFX_TEXTUREFORMAT_RGBA_U16_NORM:
         return 64;
      
      case GFX_TEXTUREFORMAT_R_F16:
         return 16;
      case GFX_TEXTUREFORMAT_RG_F16:
         return 32;
      case GFX_TEXTUREFORMAT_RGB_F16:
         return 48;
      case GFX_TEXTUREFORMAT_RGBA_F16:
         return 64;
      
      case GFX_TEXTUREFORMAT_R_F32:
         return 32;
      case GFX_TEXTUREFORMAT_RG_F32:
         return 64;
      case GFX_TEXTUREFORMAT_RGB_F32:
         return 96;
      case GFX_TEXTUREFORMAT_RGBA_F32:
         return 128;
      
      case GFX_TEXTUREFORMAT_R11F_G11F_B10F:
         return 32;
      
      case GFX_TEXTUREFORMAT_DEPTH_16:
         return 16;

      case GFX_TEXTUREFORMAT_DEPTH_24:
         return 24;

      case GFX_TEXTUREFORMAT_DEPTH_F32:
         return 32;

      case GFX_TEXTUREFORMAT_DEPTH_24_STENCIL_8:
         return 32;
      
      case GFX_TEXTUREFORMAT_DEPTH_F32_STENCIL_8:
         return 40;
      
      // TODO: compressed formats
      
      default:
         return 32;
   }
}

i32 GFX_TextureInternalFormat(u8 format)
{
   switch (format)
   {
      case GFX_TEXTUREFORMAT_R_U8_NORM:
         return GL_R8;
      case GFX_TEXTUREFORMAT_R_U16_NORM:
         return GL_R16;
      case GFX_TEXTUREFORMAT_R_F16:
         return GL_R16;
      case GFX_TEXTUREFORMAT_R_F32:
         return GL_R32F;

      case GFX_TEXTUREFORMAT_RG_U8_NORM:
         return GL_RG8;
      case GFX_TEXTUREFORMAT_RG_U16_NORM:
         return GL_RG16;
      case GFX_TEXTUREFORMAT_RG_F16:
         return GL_RG16;
      case GFX_TEXTUREFORMAT_RG_F32:
         return GL_RG32F;
      
      case GFX_TEXTUREFORMAT_RGB_U8_NORM:
         return GL_RGB8;
      case GFX_TEXTUREFORMAT_RGB_U16_NORM:
         return GL_RGB16;
      case GFX_TEXTUREFORMAT_RGB_F16:
         return GL_RGB16;
      case GFX_TEXTUREFORMAT_RGB_F32:
         return GL_RGB32F;
      
      case GFX_TEXTUREFORMAT_RGBA_U8_NORM:
         return GL_RGBA8;
      case GFX_TEXTUREFORMAT_RGBA_U16_NORM:
         return GL_RGBA16;
      case GFX_TEXTUREFORMAT_RGBA_F16:
         return GL_RGBA16;
      case GFX_TEXTUREFORMAT_RGBA_F32:
         return GL_RGBA32F;
      
      case GFX_TEXTUREFORMAT_R11F_G11F_B10F:
         return GL_R11F_G11F_B10F;
      
      case GFX_TEXTUREFORMAT_DEPTH_16:
      case GFX_TEXTUREFORMAT_DEPTH_24:
      case GFX_TEXTUREFORMAT_DEPTH_F32:
         return GL_DEPTH_COMPONENT;

      case GFX_TEXTUREFORMAT_DEPTH_24_STENCIL_8:
      case GFX_TEXTUREFORMAT_DEPTH_F32_STENCIL_8:
         return GL_DEPTH_STENCIL;
      
      case GFX_TEXTUREFORMAT_SRGB:
         return GL_SRGB8;
      
      case GFX_TEXTUREFORMAT_SRGB_ALPHA:
         return GL_SRGB8_ALPHA8;

      // TODO: compressed formats
      
      default:
         return GL_RGBA8;
   }
}

u32 GFX_TexturePixelFormat(u8 format)
{
   switch (format)
   {
      case GFX_TEXTUREFORMAT_R_U8_NORM:
      case GFX_TEXTUREFORMAT_R_U16_NORM:
      case GFX_TEXTUREFORMAT_R_F16:
      case GFX_TEXTUREFORMAT_R_F32:
         return GL_RED;

      case GFX_TEXTUREFORMAT_RG_U8_NORM:
      case GFX_TEXTUREFORMAT_RG_U16_NORM:
      case GFX_TEXTUREFORMAT_RG_F16:
      case GFX_TEXTUREFORMAT_RG_F32:
         return GL_RG;
      
      case GFX_TEXTUREFORMAT_RGB_U8_NORM:
      case GFX_TEXTUREFORMAT_RGB_U16_NORM:
      case GFX_TEXTUREFORMAT_RGB_F16:
      case GFX_TEXTUREFORMAT_RGB_F32:
      case GFX_TEXTUREFORMAT_R11F_G11F_B10F:
      case GFX_TEXTUREFORMAT_SRGB:
         return GL_RGB;
      
      case GFX_TEXTUREFORMAT_RGBA_U8_NORM:
      case GFX_TEXTUREFORMAT_RGBA_U16_NORM:
      case GFX_TEXTUREFORMAT_RGBA_F16:
      case GFX_TEXTUREFORMAT_RGBA_F32:
      case GFX_TEXTUREFORMAT_SRGB_ALPHA:
         return GL_RGBA;
      
      case GFX_TEXTUREFORMAT_DEPTH_16:
      case GFX_TEXTUREFORMAT_DEPTH_24:
      case GFX_TEXTUREFORMAT_DEPTH_F32:
         return GL_DEPTH_COMPONENT;

      case GFX_TEXTUREFORMAT_DEPTH_24_STENCIL_8:
      case GFX_TEXTUREFORMAT_DEPTH_F32_STENCIL_8:
         return GL_DEPTH_STENCIL;

      // TODO: compressed formats
      
      default:
         return GL_RGBA;
   }
}

u32 GFX_TextureFormatType(u8 format)
{
   switch (format)
   {
      case GFX_TEXTUREFORMAT_R_U8_NORM:
      case GFX_TEXTUREFORMAT_RG_U8_NORM:
      case GFX_TEXTUREFORMAT_RGB_U8_NORM:
      case GFX_TEXTUREFORMAT_RGBA_U8_NORM:
      case GFX_TEXTUREFORMAT_SRGB:
      case GFX_TEXTUREFORMAT_SRGB_ALPHA:
         return  GL_UNSIGNED_BYTE;
      
      case GFX_TEXTUREFORMAT_R_U16_NORM:
      case GFX_TEXTUREFORMAT_RG_U16_NORM:
      case GFX_TEXTUREFORMAT_RGB_U16_NORM:
      case GFX_TEXTUREFORMAT_RGBA_U16_NORM:
         return GL_UNSIGNED_SHORT;
      
      case GFX_TEXTUREFORMAT_R_F16:
      case GFX_TEXTUREFORMAT_RG_F16:
      case GFX_TEXTUREFORMAT_RGB_F16:
      case GFX_TEXTUREFORMAT_RGBA_F16:
         return GL_HALF_FLOAT;
      
      case GFX_TEXTUREFORMAT_R_F32:
      case GFX_TEXTUREFORMAT_RG_F32:
      case GFX_TEXTUREFORMAT_RGB_F32:
      case GFX_TEXTUREFORMAT_RGBA_F32:
         return GL_FLOAT;
      
      case GFX_TEXTUREFORMAT_R11F_G11F_B10F:
         return GL_UNSIGNED_INT;
      
      case GFX_TEXTUREFORMAT_DEPTH_16:
         return GL_UNSIGNED_SHORT;

      case GFX_TEXTUREFORMAT_DEPTH_24:
         return GL_UNSIGNED_INT;

      case GFX_TEXTUREFORMAT_DEPTH_F32:
         return GL_FLOAT;

      case GFX_TEXTUREFORMAT_DEPTH_24_STENCIL_8:
         return GL_UNSIGNED_INT;
      
      case GFX_TEXTUREFORMAT_DEPTH_F32_STENCIL_8:
         return GL_FLOAT;
      
      // TODO: compressed formats
      
      default:
         return GL_UNSIGNED_BYTE;
   }
}

u32 GFX_TextureType(u8 type)
{
   switch (type)
   {
      case GFX_TEXTURETYPE_2D:
         return GL_TEXTURE_2D;
      
      case GFX_TEXTURETYPE_3D:
         return GL_TEXTURE_3D;
      
      case GFX_TEXTURETYPE_CUBEMAP:
         return GL_TEXTURE_CUBE_MAP;
      
      case GFX_TEXTURETYPE_2D_ARRAY:
         return GL_TEXTURE_2D_ARRAY;
      
      case GFX_TEXTURETYPE_CUBEMAP_ARRAY:
         return GL_TEXTURE_CUBE_MAP_ARRAY;

      default:
         return GL_TEXTURE_2D;
   }
}

u32 GFX_TextureWrap(u8 wrap)
{
   switch (wrap)
   {
      case GFX_TEXTUREWRAP_REPEAT:
         return GL_REPEAT;
      
      case GFX_TEXTUREWRAP_REPEAT_MIRRORED:
         return  GL_MIRRORED_REPEAT;
      
      case GFX_TEXTUREWRAP_CLAMP:
         return GL_CLAMP_TO_EDGE;
      
      default:
         return GL_REPEAT;
   }
}

struct gfx_Filtering_s GFX_TextureFilter(u8 filter)
{
   switch (filter)
   {
      case GFX_TEXTUREFILTER_NEAREST_NO_MIPMAPS:
         return (struct gfx_Filtering_s){ GL_NEAREST, GL_NEAREST };
      
      case GFX_TEXTUREFILTER_NEAREST_NEAREST_MIPMAPS:
         return (struct gfx_Filtering_s){ GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST };
      
      case GFX_TEXTUREFILTER_NEAREST_LINEAR_MIPMAPS:
         return (struct gfx_Filtering_s){ GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST };
      
      case GFX_TEXTUREFILTER_BILINEAR_NO_MIPMAPS:
         return (struct gfx_Filtering_s){ GL_LINEAR, GL_LINEAR };
      
      case GFX_TEXTUREFILTER_BILINEAR_NEAREST_MIPMAPS:
         return (struct gfx_Filtering_s){ GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR };
      
      case GFX_TEXTUREFILTER_BILINEAR_LINEAR_MIPMAPS:
         return (struct gfx_Filtering_s){ GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR };
      
      case GFX_TEXTUREFILTER_NEAREST_MAX_BILINEAR_MIN:
         return (struct gfx_Filtering_s){ GL_LINEAR, GL_NEAREST };

      default:
         return (struct gfx_Filtering_s){ GL_NEAREST, GL_NEAREST };
   }
}

void GFX_CreateTexture(gfx_Texture* texture, u8* data)
{
   u32 gl_target = GFX_TextureType(texture->type);

   glGenTextures(1, &texture->id.tex);
   glBindTexture(gl_target, texture->id.tex);

   glTexParameteri(gl_target, GL_TEXTURE_WRAP_R, GL_REPEAT);
   glTexParameteri(gl_target, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(gl_target, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(gl_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(gl_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(gl_target, GL_TEXTURE_MAX_LEVEL, (i32)(texture->mipmap_count));

   i32 width = texture->width;
   i32 height = texture->height;
   i32 depth = texture->depth;

   i32 internal_format = GFX_TextureInternalFormat(texture->format);
   u32 pixel_format = GFX_TexturePixelFormat(texture->format);
   u32 format_type = GFX_TextureFormatType(texture->format);

   if (data == NULL)
   {
      switch (texture->type) {
         case GFX_TEXTURETYPE_2D:
         case GFX_TEXTURETYPE_CUBEMAP:
            glTexStorage2D(gl_target, texture->mipmap_count, internal_format, width, height);
            break;
         
         case GFX_TEXTURETYPE_3D:
         case GFX_TEXTURETYPE_2D_ARRAY:
         case GFX_TEXTURETYPE_CUBEMAP_ARRAY:
            glTexStorage3D(gl_target, texture->mipmap_count, internal_format, width, height, depth);
            break;
         
         default:
            glTexStorage2D(gl_target, 1, internal_format, width, height);
      }
   } else {
      bool is_cubemap = ((texture->type == GFX_TEXTURETYPE_CUBEMAP) || (texture->type == GFX_TEXTURETYPE_CUBEMAP_ARRAY));
      u32 face_count = is_cubemap ? 6 : 1;
      u32 gl_face = is_cubemap ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : gl_target;

      uS pixel_size = GFX_PixelSize(texture->format);

      uS offset = 0;
      i32 mip_divisor = 1;

      switch (texture->type) {
         case GFX_TEXTURETYPE_2D:
         case GFX_TEXTURETYPE_CUBEMAP:
            for (u32 mip_i = 0; mip_i < texture->mipmap_count; mip_i++)
            {
               i32 mip_width = width / mip_divisor;
               i32 mip_height = height / mip_divisor;
               for (u32 face_i = 0; face_i < face_count; face_i++)
               {
                  glTexImage2D(gl_face + face_i, mip_i, internal_format, mip_width, mip_height, 0, pixel_format, format_type, data + offset);
                  offset += mip_width * mip_height * pixel_size;
               }

               mip_divisor *= 2;
            }
            break;
         
         case GFX_TEXTURETYPE_3D:
         case GFX_TEXTURETYPE_2D_ARRAY:
         case GFX_TEXTURETYPE_CUBEMAP_ARRAY:
            for (u32 mip_i = 0; mip_i < texture->mipmap_count; mip_i++)
            {
               i32 mip_width = width / mip_divisor;
               i32 mip_height = height / mip_divisor;
               i32 mip_depth = depth / mip_divisor;

               for (u32 face_i = 0; face_i < face_count; face_i++)
               {
                  glTexImage3D(gl_face + face_i, mip_i, internal_format, mip_width, mip_height, mip_depth, 0, pixel_format, format_type, data + offset);
                  offset += mip_width * mip_height * pixel_size;
               }

               mip_divisor *= 2;
            }
            break;
         
         default:
            glTexImage2D(gl_face, 0, internal_format, width, height, 0, pixel_format, format_type, data);
      }
   }
}
