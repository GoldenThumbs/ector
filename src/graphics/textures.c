#include "image.h"
#include "util/types.h"
#include "util/math.h"
#include "util/array.h"
#include "util/resource.h"
#include "util/files.h"

#include "graphics.h"
#include "graphics/internal.h"

#include <glad/gl.h>

#include <assert.h>
#include <stdlib.h>
// #include <string.h>

Texture Graphics_CreateTexture(Graphics* graphics, u8* data, TextureDesc desc)
{
   if (graphics == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   gfx_Texture texture = { 0 };
   texture.width = M_MAX(1, desc.size.width);
   texture.height = M_MAX(1, desc.size.height);
   texture.depth = M_MAX(1, desc.depth);
   texture.mipmap_count = M_MAX(1, desc.mipmap_count);
   texture.type = desc.texture_type;
   texture.format = desc.texture_format;

   glGenTextures(1, &texture.id.tex);
   
   u32 gl_target = GFX_TextureType(texture.type);
   glTexParameteri(gl_target, GL_TEXTURE_WRAP_R, GL_REPEAT);
   glTexParameteri(gl_target, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(gl_target, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(gl_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(gl_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(gl_target, GL_TEXTURE_MAX_LEVEL, (i32)(texture.mipmap_count));

   GFX_CreateTexture(&texture, data, false);

   texture.compare.handle = Util_ArrayLength(graphics->textures);

   if (graphics->freed_texture_root == INVALID_HANDLE)
      return ADD_RESOURCE(graphics->textures, texture);

   return REUSE_RESOURCE(graphics->textures, texture, graphics->freed_texture_root);
}

void Graphics_FreeTexture(Graphics* graphics, Texture res_texture)
{
   if (graphics == NULL || res_texture.id == INVALID_HANDLE_ID)
      return;

   gfx_Texture* texture = &graphics->textures[res_texture.handle];
   if (texture->compare.ref != res_texture.ref)
   {
      Util_Log(NULL, GRAPHICS_MODULE, (error){ .general = ERR_ERROR }, "Invalid handle! Handle ID: %u (Texture)", res_texture.id);

      return;
   }

   texture->next_freed = graphics->freed_texture_root;
   graphics->freed_texture_root = (u32)res_texture.handle;

   glDeleteTextures(1, &texture->id.tex);

}

void Graphics_UpdateTexture(Graphics* graphics, u8* data, Texture res_texture)
{
   if (graphics == NULL || res_texture.id == INVALID_HANDLE_ID)
      return;

   gfx_Texture texture = graphics->textures[res_texture.handle];
   if (texture.compare.ref != res_texture.ref)
   {
      Util_Log(NULL, GRAPHICS_MODULE, (error){ .general = ERR_ERROR }, "Invalid handle! Handle ID: %u (Texture)", res_texture.id);

      return;
   }

   GFX_CreateTexture(&texture, data, true);

}

void Graphics_BindTexture(Graphics *graphics, Texture res_texture, u32 bind_slot)
{
   if (graphics == NULL || res_texture.id == INVALID_HANDLE_ID)
      return;

   gfx_Texture texture = graphics->textures[res_texture.handle];
   if (texture.compare.ref != res_texture.ref)
   {
      Util_Log(NULL, GRAPHICS_MODULE, (error){ .general = ERR_ERROR }, "Invalid handle! Handle ID: %u (Texture)", res_texture.id);

      return;
   }

   u32 gl_target = GFX_TextureType(texture.type);

   glActiveTexture(GL_TEXTURE0 + bind_slot);
   glBindTexture(gl_target, texture.id.tex);

}

void Graphics_BindTextureView(Graphics* graphics, Texture res_texture, u32 bind_slot, const AdvancedBindOptions* bind_options)
{
   if (graphics == NULL || res_texture.id == INVALID_HANDLE_ID)
      return;

   gfx_Texture texture = graphics->textures[res_texture.handle];
   if (texture.compare.ref != res_texture.ref)
   {
      Util_Log(NULL, GRAPHICS_MODULE, (error){ .general = ERR_ERROR }, "Invalid handle! Handle ID: %u (Texture)", res_texture.id);

      return;
   }

   bool is_layered = false;
   i32 desired_layer = 0;
   i32 mip_level = 0;
   u32 access_type = GL_READ_ONLY;

   if (bind_options != NULL)
   {
      is_layered = (bind_options->layer < 0);
      desired_layer = (is_layered) ? 0 : bind_options->layer;
      mip_level = (i32)bind_options->mip_level;
      access_type += bind_options->access_type;

   }

   u32 gl_target = GFX_TextureType(texture.type);

   u32 format = GFX_TextureInternalFormat(texture.format);
   glBindImageTexture(bind_slot, texture.id.tex, mip_level, is_layered, desired_layer, access_type, format);

}

void Graphics_UnbindTextures(Graphics* graphics, u8 texture_type)
{
   if (graphics == NULL)
      return;

   u32 gl_target = GFX_TextureType(texture_type);

   glBindTexture(gl_target, 0);

}

void Graphics_SetTextureInterpolation(Graphics* graphics, Texture res_texture, TextureInterpolation interpolation_settings)
{
   if (graphics == NULL || res_texture.id == INVALID_HANDLE_ID)
      return;

   gfx_Texture texture = graphics->textures[res_texture.handle];
   if (texture.compare.ref != res_texture.ref)
   {
      Util_Log(NULL, GRAPHICS_MODULE, (error){ .general = ERR_ERROR }, "Invalid handle! Handle ID: %u (Texture)", res_texture.id);

      return;
   }

   u32 gl_target = GFX_TextureType(texture.type);

   glBindTexture(gl_target, texture.id.tex);

   f32 aniso = M_CLAMP((f32)interpolation_settings.texture_anisotropy, 1.0f, GL_MAX_TEXTURE_MAX_ANISOTROPY);
   u32 wrap = GFX_TextureWrap(interpolation_settings.texture_wrap);
   gfx_Filtering filter = GFX_TextureFilter(interpolation_settings.texture_filter);

   glTexParameteri(gl_target, GL_TEXTURE_WRAP_R, wrap);
   glTexParameteri(gl_target, GL_TEXTURE_WRAP_S, wrap);
   glTexParameteri(gl_target, GL_TEXTURE_WRAP_T, wrap);
   glTexParameteri(gl_target, GL_TEXTURE_MIN_FILTER, filter.min_filter);
   glTexParameteri(gl_target, GL_TEXTURE_MAG_FILTER, filter.mag_filter);
   glTexParameterf(gl_target, GL_TEXTURE_MAX_ANISOTROPY, aniso);

}

Image Graphics_GetTextureImageData(Graphics* graphics, Texture res_texture, u32 mip_level, u8 cubemap_face)
{
   if (graphics == NULL || res_texture.id == INVALID_HANDLE_ID)
      return (Image){ NULL };

   gfx_Texture texture = graphics->textures[res_texture.handle];
   if (texture.compare.ref != res_texture.ref)
   {
      Util_Log(NULL, GRAPHICS_MODULE, (error){ .general = ERR_ERROR }, "Invalid handle! Handle ID: %u (Texture)", res_texture.id);

      return (Image){ NULL };;
   }

   i32 mip_divisor = 1 << mip_level;

   Image image = { 0 };
   image.size.width = M_MAX(1, texture.width / mip_divisor);
   image.size.height = M_MAX(1, texture.height / mip_divisor) * M_MAX(1, texture.depth / mip_divisor);
   image.depth = 1;
   image.mipmap_count = 1;
   image.channel_count = 4;
   image.image_type = IMG_TYPE_2D;
   image.image_format = IMG_FORMAT_F32;

   u32 gl_type = GFX_TextureFormatType(texture.format);
   gl_type = (gl_type == GL_HALF_FLOAT) ? GL_FLOAT : ((gl_type != GL_FLOAT) ? GL_UNSIGNED_BYTE : GL_FLOAT);
   
   u32 gl_format = GFX_TexturePixelFormat(texture.format);
   gl_format = (gl_format == GL_DEPTH_COMPONENT) ? GL_RED : ((gl_format == GL_DEPTH_STENCIL) ? GL_RG : gl_format);

   switch (gl_format)
   {
      case GL_RG:
         image.channel_count = 2;
         break;

      case GL_RGB:
         image.channel_count = 3;
         break;

      case GL_RGBA:
         image.channel_count = 4;
         break;

      default:
      case GL_RED:
         image.channel_count = 1;
      
   }

   image.image_format = (gl_type == GL_FLOAT) ? IMG_FORMAT_F32 : IMG_FORMAT_U8;

   u32 gl_target = GFX_TextureType(texture.type);
   if (texture.format == GFX_TEXTURETYPE_CUBEMAP)
      gl_target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + cubemap_face;

   uS pixel_size = ((gl_type == GL_FLOAT) ? 4 : 1) * image.channel_count;
   uS num_bytes = image.size.width * image.size.height * pixel_size;
   
   f32* image_data = malloc(num_bytes);
   assert(image_data != NULL);

   glBindTexture(gl_target, texture.id.tex);
   glGetTexImage(gl_target, mip_level, gl_format, gl_type, image_data);

   GFX_CheckOpenGLError();

   image.data = (u8*)image_data;

   return image;
}

Framebuffer Graphics_CreateFramebuffer(Graphics* graphics, resolution2d size, bool depthstencil_renderbuffer)
{
   if (graphics == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   gfx_Framebuffer framebuffer = { 0 };

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
      return (handle){ 0 };

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);

   if (graphics->freed_framebuffer_root == INVALID_HANDLE)
      return ADD_RESOURCE(graphics->framebuffers, framebuffer);

   return REUSE_RESOURCE(graphics->framebuffers, framebuffer, graphics->freed_framebuffer_root);
}

void Graphics_ReuseFramebuffer(Graphics* graphics, resolution2d size, bool depthstencil_renderbuffer, Framebuffer res_framebuffer)
{
   if (graphics == NULL || res_framebuffer.id == INVALID_HANDLE_ID)
      return;

   gfx_Framebuffer framebuffer = graphics->framebuffers[res_framebuffer.handle];
   if (framebuffer.compare.ref != res_framebuffer.ref)
   {
      Util_Log(NULL, GRAPHICS_MODULE, (error){ .general = ERR_ERROR }, "Invalid handle! Handle ID: %u (Framebuffer)", res_framebuffer.id);

      return;
   }

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
   if (graphics == NULL || res_framebuffer.id == INVALID_HANDLE_ID)
      return;

   gfx_Framebuffer* framebuffer = &graphics->framebuffers[res_framebuffer.handle];
   if (framebuffer->compare.ref != res_framebuffer.ref)
   {
      Util_Log(NULL, GRAPHICS_MODULE, (error){ .general = ERR_ERROR }, "Invalid handle! Handle ID: %u (Framebuffer)", res_framebuffer.id);

      return;
   }

   framebuffer->next_freed = graphics->freed_framebuffer_root;
   graphics->freed_framebuffer_root = (u32)res_framebuffer.handle;

   glDeleteFramebuffers(1, &framebuffer->id.fbo);
   if (framebuffer->id.rbo != 0)
      glDeleteRenderbuffers(1, &framebuffer->id.rbo);

}

void Graphics_DrawToFramebufferTargets(Graphics* graphics, Framebuffer res_framebuffer, u32 target_count, u8 target_ids[])
{
   if (graphics == NULL || res_framebuffer.id == INVALID_HANDLE_ID || target_ids == NULL || target_count > 8)
      return;

   gfx_Framebuffer framebuffer = graphics->framebuffers[res_framebuffer.handle];
   if (framebuffer.compare.ref != res_framebuffer.ref)
   {
      Util_Log(NULL, GRAPHICS_MODULE, (error){ .general = ERR_ERROR }, "Invalid handle! Handle ID: %u (Framebuffer)", res_framebuffer.id);

      return;
   }

   u32 targets[8] = { 0 };
   for (u32 target_i = 0; target_i < target_count; target_i++)
      targets[target_i] = GL_COLOR_ATTACHMENT0 + (u32)target_ids[target_i];

   glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id.fbo);

   glDrawBuffers(target_count, targets);
   
}

void Graphics_BindFramebuffer(Graphics* graphics, Framebuffer res_framebuffer)
{
   if (graphics == NULL || res_framebuffer.id == INVALID_HANDLE_ID)
      return;

   gfx_Framebuffer framebuffer = graphics->framebuffers[res_framebuffer.handle];
   if (framebuffer.compare.ref != res_framebuffer.ref)
   {
      Util_Log(NULL, GRAPHICS_MODULE, (error){ .general = ERR_ERROR }, "Invalid handle! Handle ID: %u (Framebuffer)", res_framebuffer.id);

      return;
   }

   glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id.fbo);
   glBindRenderbuffer(GL_RENDERBUFFER, framebuffer.id.rbo);

}

void Graphics_UnbindFramebuffers(Graphics *graphics)
{
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);

}

void Graphics_AttachMultipleTexturesToFramebuffer(Graphics* graphics, Framebuffer res_framebuffer, u32 texture_count, Texture res_textures[])
{
   if (graphics == NULL || res_framebuffer.id == INVALID_HANDLE_ID || res_textures == NULL)
      return;

   for (u32 texture_i = 0; texture_i < texture_count; texture_i++)
      Graphics_AttachTextureToFramebuffer(graphics, res_framebuffer, res_textures[texture_i], &(AdvancedBindOptions){ 0 }, texture_i);

}

void Graphics_AttachTextureToFramebuffer(Graphics* graphics, Framebuffer res_framebuffer, Texture res_texture, const AdvancedBindOptions* bind_options, u8 attachment_slot)
{
   if (graphics == NULL || res_framebuffer.id == INVALID_HANDLE_ID || res_texture.id == INVALID_HANDLE_ID)
      return;

   gfx_Framebuffer framebuffer = graphics->framebuffers[res_framebuffer.handle];
   if (framebuffer.compare.ref != res_framebuffer.ref)
   {
      Util_Log(NULL, GRAPHICS_MODULE, (error){ .general = ERR_ERROR }, "Invalid handle! Handle ID: %u (Framebuffer)", res_framebuffer.id);

      return;
   }

   gfx_Texture texture = graphics->textures[res_texture.handle];
   if (texture.compare.ref != res_texture.ref || (texture.type != GFX_TEXTURETYPE_2D_ARRAY && texture.type != GFX_TEXTURETYPE_CUBEMAP_ARRAY))
   {
      Util_Log(NULL, GRAPHICS_MODULE, (error){ .general = ERR_ERROR }, "Invalid handle! Handle ID: %u (Texture)", res_texture.id);

      return;
   }

   bool is_layered = false;
   i32 desired_layer = 0;
   i32 mip_level = 0;
   u8 cubemap_face = GFX_CUBEMAPFACE_NONE;

   if (bind_options != NULL)
   {
      is_layered = (bind_options->layer < 0);
      desired_layer = (is_layered) ? 0 : bind_options->layer;
      mip_level = (i32)bind_options->mip_level;
      cubemap_face = bind_options->cubemap_face;

   }

   if (is_layered)
   {
      error err = { .general = ERR_ERROR };
      err.extra = ERR_GFX_FRAMEBUFFER_ATTACHMENT_FAILED;
      err.flags |= ERR_INFO_GFX_INVALID_FRAMEBUFFER_ATTACHMENT;
      
      Util_Log(NULL, GRAPHICS_MODULE, err, "Invalid framebuffer attachment! Framebuffer attachments cannot be layered.");

      return;
   }

   bool is_cubemap = (texture.type == GFX_TEXTURETYPE_CUBEMAP_ARRAY || texture.type == GFX_TEXTURETYPE_CUBEMAP);
   bool is_array = (texture.type == GFX_TEXTURETYPE_2D_ARRAY || texture.type == GFX_TEXTURETYPE_CUBEMAP_ARRAY);

   u32 attachment = GL_COLOR_ATTACHMENT0 + attachment_slot;
   if (GFX_IsDepthFormat(texture.format))
      attachment = (GFX_IsStencilFormat(texture.format)) ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;

   if (is_cubemap && is_array)
      desired_layer = desired_layer * 6 + cubemap_face;

   glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id.fbo);
   glBindRenderbuffer(GL_RENDERBUFFER, framebuffer.id.rbo);

   if (is_array || texture.format == GFX_TEXTURETYPE_3D)
      glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, texture.id.tex, mip_level, desired_layer);
   else {
      u32 gl_target = 0;
      if (!is_cubemap)
         gl_target = GFX_TextureType(texture.type);
      else
         gl_target =  GL_TEXTURE_CUBE_MAP_POSITIVE_X + cubemap_face;

      glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, gl_target, texture.id.tex, mip_level);

   }

   if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
   {
      error err = { .general = ERR_ERROR };
      err.extra = ERR_GFX_FRAMEBUFFER_ATTACHMENT_FAILED;

      Util_Log(NULL, GRAPHICS_MODULE, err, "Texture failed to attach to framebuffer! Intended attachment slot: %u", attachment_slot);

      return;
   }

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);

   GFX_CheckOpenGLError();

}

uS GFX_PixelSize(u8 format)
{
   switch (format)
   {
      case GFX_TEXTUREFORMAT_R_U8_NORM:
         return 1;
      case GFX_TEXTUREFORMAT_RG_U8_NORM:
         return 2;
      case GFX_TEXTUREFORMAT_RGB_U8_NORM:
      case GFX_TEXTUREFORMAT_SRGB:
         return 3;
      case GFX_TEXTUREFORMAT_RGBA_U8_NORM:
      case GFX_TEXTUREFORMAT_SRGB_ALPHA:
         return 4;
      
      case GFX_TEXTUREFORMAT_R_U16_NORM:
         return 2;
      case GFX_TEXTUREFORMAT_RG_U16_NORM:
         return 4;
      case GFX_TEXTUREFORMAT_RGB_U16_NORM:
         return 6;
      case GFX_TEXTUREFORMAT_RGBA_U16_NORM:
         return 8;
      
      case GFX_TEXTUREFORMAT_R_F16:
         return 2;
      case GFX_TEXTUREFORMAT_RG_F16:
         return 4;
      case GFX_TEXTUREFORMAT_RGB_F16:
         return 6;
      case GFX_TEXTUREFORMAT_RGBA_F16:
         return 8;
      
      case GFX_TEXTUREFORMAT_R_F32:
         return 4;
      case GFX_TEXTUREFORMAT_RG_F32:
         return 8;
      case GFX_TEXTUREFORMAT_RGB_F32:
         return 12;
      case GFX_TEXTUREFORMAT_RGBA_F32:
         return 16;
      
      case GFX_TEXTUREFORMAT_R11F_G11F_B10F:
         return 4;
      
      case GFX_TEXTUREFORMAT_DEPTH_16:
         return 2;

      case GFX_TEXTUREFORMAT_DEPTH_24:
         return 3;

      case GFX_TEXTUREFORMAT_DEPTH_F32:
         return 4;

      case GFX_TEXTUREFORMAT_DEPTH_24_STENCIL_8:
         return 4;
      
      case GFX_TEXTUREFORMAT_DEPTH_F32_STENCIL_8:
         return 5;
      
      // TODO: compressed formats
      
      default:
         return 4;
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

gfx_Filtering GFX_TextureFilter(u8 filter)
{
   switch (filter)
   {
      case GFX_TEXTUREFILTER_POINT_NO_MIPMAPS:
         return (gfx_Filtering){ GL_NEAREST, GL_NEAREST };
      
      case GFX_TEXTUREFILTER_POINT_NEAREST_MIPMAPS:
         return (gfx_Filtering){ GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST };
      
      case GFX_TEXTUREFILTER_POINT_LINEAR_MIPMAPS:
         return (gfx_Filtering){ GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST };
      
      case GFX_TEXTUREFILTER_BILINEAR_NO_MIPMAPS:
         return (gfx_Filtering){ GL_LINEAR, GL_LINEAR };
      
      case GFX_TEXTUREFILTER_BILINEAR_NEAREST_MIPMAPS:
         return (gfx_Filtering){ GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR };
      
      case GFX_TEXTUREFILTER_BILINEAR_LINEAR_MIPMAPS:
         return (gfx_Filtering){ GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR };
      
      case GFX_TEXTUREFILTER_POINT_MAX_BILINEAR_MIN:
         return (gfx_Filtering){ GL_LINEAR, GL_NEAREST };

      default:
         return (gfx_Filtering){ GL_NEAREST, GL_NEAREST };
   }
}

void GFX_CreateTexture(gfx_Texture* texture, u8* data, bool is_update)
{
   if (texture == NULL)
      return;
   
   u32 gl_target = GFX_TextureType(texture->type);
   glBindTexture(gl_target, texture->id.tex);

   i32 width = texture->width;
   i32 height = texture->height;
   i32 depth = texture->depth;

   i32 internal_format = GFX_TextureInternalFormat(texture->format);
   u32 pixel_format = GFX_TexturePixelFormat(texture->format);
   u32 format_type = GFX_TextureFormatType(texture->format);

   if (data == NULL)
   {
      switch (texture->type) {
         case GFX_TEXTURETYPE_3D:
         case GFX_TEXTURETYPE_2D_ARRAY:
         case GFX_TEXTURETYPE_CUBEMAP_ARRAY:
            glTexStorage3D(gl_target, texture->mipmap_count, internal_format, width, height, depth);
            break;
         
         default:
            glTexStorage2D(gl_target, texture->mipmap_count, internal_format, width, height);
         
      }
      
   } else {
      bool is_cubemap = ((texture->type == GFX_TEXTURETYPE_CUBEMAP) || (texture->type == GFX_TEXTURETYPE_CUBEMAP_ARRAY));
      u32 face_count = is_cubemap ? 6 : 1;
      u32 gl_face = is_cubemap ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : gl_target;

      uS pixel_size = GFX_PixelSize(texture->format);

      uS offset = 0;
      i32 mip_width = M_MAX(width, 1);
      i32 mip_height = M_MAX(height, 1);
      i32 mip_depth = M_MAX(depth, 1);

      for (u32 mip_i = 0; mip_i < texture->mipmap_count; mip_i++)
      {
         for (u32 face_i = 0; face_i < face_count; face_i++)
         {
            switch (texture->type) {
               case GFX_TEXTURETYPE_3D:
               case GFX_TEXTURETYPE_2D_ARRAY:
               case GFX_TEXTURETYPE_CUBEMAP_ARRAY:
                  if (!is_update)
                     glTexImage3D(gl_face + face_i, mip_i, internal_format, mip_width, mip_height, mip_depth, 0, pixel_format, format_type, data + offset);
                  else
                     glTexSubImage3D(gl_face + face_i, mip_i, 0, 0, 0, mip_width, mip_height, mip_depth, pixel_format, format_type, data + offset);
                  break;
               
               default:
               case GFX_TEXTURETYPE_2D:
                  if(!is_update)
                     glTexImage2D(gl_face + face_i, mip_i, internal_format, mip_width, mip_height, 0, pixel_format, format_type, data + offset);
                  else
                     glTexSubImage2D(gl_face + face_i, mip_i, 0, 0, mip_width, mip_height, pixel_format, format_type, data + offset);
               
            }

            offset += pixel_size * (uS)(mip_width * mip_height * mip_depth);

         }

         mip_width = M_MAX(mip_width / 2, 1);
         mip_height = M_MAX(mip_height / 2, 1);
         mip_depth = M_MAX(mip_depth / 2, 1);

      }

   }

}
