#include "util/types.h"
#include "util/math.h"
#include "util/array.h"
#include "util/resource.h"

#include "graphics.h"
#include "graphics/internal.h"

#include <glad/gl.h>

Texture Graphics_CreateTexture(Graphics* graphics, u8* data, resolution2d size, u8 channels)
{
   gfx_Texture texture = { 0 };
   texture.width = (u32)size.width;
   texture.height = (u32)size.height;
   texture.mipmap_count = 1;
   texture.compare.ref = graphics->ref;

   glGenTextures(1, &texture.id.tex);
   glBindTexture(GL_TEXTURE_2D, texture.id.tex);

   channels = M_MAX(channels, 3);
   const u32 format[] = {
      GL_RED,
      GL_RG,
      GL_RGB,
      GL_RGBA
   };

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

   glTexImage2D(GL_TEXTURE_2D, 0, format[channels], size.width, size.height, 0, format[channels], GL_UNSIGNED_BYTE, data);
   if (data != NULL)
      glGenerateMipmap(GL_TEXTURE_2D);

   texture.compare.handle = Util_ArrayLength(graphics->textures);

   return Util_AddResource(&graphics->ref, REF(graphics->textures), &texture);
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

   glActiveTexture(GL_TEXTURE0 + bind_slot);
   glBindTexture(GL_TEXTURE_2D, texture.id.tex);
}

void Graphics_UnbindTextures(Graphics* graphics)
{
   glBindTexture(GL_TEXTURE_2D, 0);
}

Framebuffer Graphics_CreateFramebuffer(Graphics* graphics, resolution2d size)
{
   gfx_Framebuffer framebuffer = { 0 };
   framebuffer.compare.ref = graphics->ref;

   glGenFramebuffers(1, &framebuffer.id.fbo);
   
   glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id.fbo);
   glGenRenderbuffers(1, &framebuffer.id.rbo);

   glBindRenderbuffer(GL_RENDERBUFFER, framebuffer.id.rbo);
   glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size.width, size.height);

   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, framebuffer.id.rbo);

   if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
   {
      return (handle){ 0 };
   }

   framebuffer.compare.handle = Util_ArrayLength(graphics->textures);

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);

   return Util_AddResource(&graphics->ref, REF(graphics->framebuffers), &framebuffer);
}

void Graphics_FreeFramebuffer(Graphics* graphics, Framebuffer res_framebuffer)
{
   gfx_Framebuffer framebuffer = graphics->framebuffers[res_framebuffer.handle];
   if (framebuffer.compare.ref != res_framebuffer.ref)
      return;

   glDeleteFramebuffers(1, &framebuffer.id.fbo);
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

      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, texture.id.tex, 0);
   }

   if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      return;

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);
}
