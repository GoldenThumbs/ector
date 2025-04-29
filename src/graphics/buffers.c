#include "util/types.h"
#include "util/resource.h"

#include "graphics.h"
#include "graphics/internal.h"

#include <glad/gl.h>

#include <string.h>

Buffer Graphics_CreateBuffer(GraphicsContext* context, void* data, u32 length, uS type_size, u8 draw_mode, u8 buffer_type)
{
   gfx_Buffer buffer = { 0 };
   buffer.type = buffer_type;
   buffer.draw_mode = draw_mode;
   buffer.compare.ref = context->ref;

   u32 gl_target = GFX_BufferType(buffer.type);

   glGenBuffers(1, &buffer.id.buf);
   glBindBuffer(gl_target, buffer.id.buf);

   glBufferData(
      gl_target,
      (uS)length * type_size,
      data,
      GFX_DrawMode(buffer.draw_mode)
   );
   if (data == NULL)
      glClearBufferData(gl_target, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, NULL);

   //glBindBuffer(gl_target, 0);
   return Util_AddResource(&context->ref, REF(context->buffers), &buffer);
}

Buffer Graphics_CreateBufferExplicit(GraphicsContext* context, void* data, uS size, u8 draw_mode, u8 buffer_type)
{
   gfx_Buffer buffer = { 0 };
   buffer.type = buffer_type;
   buffer.draw_mode = draw_mode;
   buffer.compare.ref = context->ref;

   u32 gl_target = GFX_BufferType(buffer.type);

   glGenBuffers(1, &buffer.id.buf);
   glBindBuffer(gl_target, buffer.id.buf);

   glBufferData(
      gl_target,
      size,
      data,
      GFX_DrawMode(buffer.draw_mode)
   );
   if (data == NULL)
      glClearBufferData(gl_target, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, NULL);

   return Util_AddResource(&context->ref, REF(context->buffers), &buffer);
}

void Graphics_ReuseBuffer(GraphicsContext* context, void* data, u32 length, uS type_size, Buffer res_buffer)
{
   gfx_Buffer buffer = context->buffers[res_buffer.handle];
   if (buffer.compare.ref != res_buffer.ref)
      return;

   // glDeleteBuffers(1, &buffer.id.buf);

   u32 gl_target = GFX_BufferType(buffer.type);

   // glGenBuffers(1, &buffer.id.buf);
   glBindBuffer(gl_target, buffer.id.buf);

   glBufferData(
      gl_target,
      (uS)length * type_size,
      data,
      GFX_DrawMode(buffer.draw_mode)
   );
}

void Graphics_FreeBuffer(GraphicsContext* context, Buffer res_buffer)
{
   gfx_Buffer buffer = context->buffers[res_buffer.handle];
   if (buffer.compare.ref != res_buffer.ref)
      return;

   glDeleteBuffers(1, &buffer.id.buf);
   buffer.id.buf = 0;
}

void Graphics_UpdateBuffer(GraphicsContext* context, Buffer res_buffer, void* data, u32 length, uS type_size)
{
   gfx_Buffer buffer = context->buffers[res_buffer.handle];
   if (buffer.compare.ref != res_buffer.ref)
      return;

   u32 gl_target = GFX_BufferType(buffer.type);
   
   glBindBuffer(gl_target, buffer.id.buf);
   glBufferSubData(gl_target, 0, (uS)length * type_size, data);
   glBindBuffer(gl_target, 0);
}

void Graphics_UpdateBufferRange(GraphicsContext* context, Buffer res_buffer, void* data, u32 offset, u32 length, uS type_size)
{
   gfx_Buffer buffer = context->buffers[res_buffer.handle];
   if (buffer.compare.ref != res_buffer.ref)
      return;

   u32 gl_target = GFX_BufferType(buffer.type);
   
   glBindBuffer(gl_target, buffer.id.buf);
   glBufferSubData(gl_target, (uS)offset * type_size, (uS)length * type_size, data);
   glBindBuffer(gl_target, 0);
}

void Graphics_UseBuffer(GraphicsContext* context, Buffer res_buffer, u32 slot)
{
   gfx_Buffer buffer = context->buffers[res_buffer.handle];
   if (buffer.compare.ref != res_buffer.ref)
      return;

   u32 gl_target = GFX_BufferType(buffer.type);

   glBindBuffer(gl_target, buffer.id.buf);
   glBindBufferBase(gl_target, slot, buffer.id.buf);
}