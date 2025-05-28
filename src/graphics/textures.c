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
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

   glTexImage2D(GL_TEXTURE_2D, 0, format[channels], size.width, size.height, 0, format[channels], GL_UNSIGNED_BYTE, data);
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
   texture.id.tex = 0;
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
