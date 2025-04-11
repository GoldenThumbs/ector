#include "util/types.h"
#include "util/resource.h"

#include "graphics.h"
#include "graphics/internal.h"

#include <glad/gl.h>

#include <string.h>

StorageBuffer Graphics_CreateStorageBuffer(GraphicsContext* context, void* data, u32 length, uS type_size, u8 draw_mode)
{
   gfx_StorageBuffer buffer = { 0 };
   buffer.compare.ref = context->ref;

   glGenBuffers(1, &buffer.id.buf);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer.id.buf);

   glBufferData(
      GL_SHADER_STORAGE_BUFFER,
      (uS)length * type_size,
      data,
      GFX_DrawMode(draw_mode)
   );
   if (data == NULL)
      glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, NULL);

   glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
   return Util_AddResource(&context->ref, REF(context->storage_buffers), &buffer);
}

void Graphics_FreeStorageBuffer(GraphicsContext* context, StorageBuffer res_buffer)
{
   gfx_StorageBuffer buffer = context->storage_buffers[res_buffer.handle];
   if (buffer.compare.ref != res_buffer.ref)
      return;

   glDeleteBuffers(1, &buffer.id.buf);
}

void Graphics_UpdateStorageBuffer(GraphicsContext* context, StorageBuffer res_buffer, void* data, u32 length, uS type_size)
{
   gfx_StorageBuffer buffer = context->storage_buffers[res_buffer.handle];
   if (buffer.compare.ref != res_buffer.ref)
      return;

   glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer.id.buf);
   void* ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
   memcpy(ptr, data, (uS)length * type_size);
   glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Graphics_UseStorageBuffer(GraphicsContext* context, StorageBuffer res_buffer, u32 slot)
{
   gfx_StorageBuffer buffer = context->storage_buffers[res_buffer.handle];
   if (buffer.compare.ref != res_buffer.ref)
      return;

   glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer.id.buf);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot, buffer.id.buf);
}