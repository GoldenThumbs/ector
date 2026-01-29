#include "util/types.h"
#include "util/resource.h"

#include "graphics.h"
#include "graphics/internal.h"

#include <glad/gl.h>

#include <string.h>

Buffer Graphics_CreateBuffer(Graphics* graphics, void* data, u32 length, uS type_size, u8 draw_mode, u8 buffer_type)
{
   return Graphics_CreateBufferExplicit(graphics, data, (uS)length * type_size, draw_mode, buffer_type);
}

Buffer Graphics_CreateBufferExplicit(Graphics* graphics, void* data, uS size, u8 draw_mode, u8 buffer_type)
{
   if (graphics == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   gfx_Buffer buffer = { 0 };
   buffer.type = buffer_type;
   buffer.draw_mode = draw_mode;

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

   if (graphics->freed_buffer_root == INVALID_HANDLE)
      return ADD_RESOURCE(graphics->buffers, buffer);

   return REUSE_RESOURCE(graphics->buffers, buffer, graphics->freed_buffer_root);
}

void Graphics_ReuseBuffer(Graphics* graphics, void* data, u32 length, uS type_size, Buffer res_buffer)
{
   if (graphics == NULL || res_buffer.id == INVALID_HANDLE_ID)
      return;

   gfx_Buffer buffer = graphics->buffers[res_buffer.handle];
   if (buffer.compare.ref != res_buffer.ref)
      return;

   u32 gl_target = GFX_BufferType(buffer.type);

   glBindBuffer(gl_target, buffer.id.buf);

   glBufferData(
      gl_target,
      (uS)length * type_size,
      data,
      GFX_DrawMode(buffer.draw_mode)
   );

}

void Graphics_FreeBuffer(Graphics* graphics, Buffer res_buffer)
{
   if (graphics == NULL || res_buffer.id == INVALID_HANDLE_ID)
      return;

   gfx_Buffer buffer = graphics->buffers[res_buffer.handle];
   if (buffer.compare.ref != res_buffer.ref)
      return;

   buffer.next_freed = graphics->freed_buffer_root;
   graphics->freed_buffer_root = (u32)res_buffer.handle;

   glDeleteBuffers(1, &buffer.id.buf);

}

void Graphics_UpdateBuffer(Graphics* graphics, Buffer res_buffer, void* data, u32 length, uS type_size)
{
   Graphics_UpdateBufferRange(graphics, res_buffer, data, 0, length, type_size);

}

void Graphics_UpdateBufferRange(Graphics* graphics, Buffer res_buffer, void* data, u32 offset, u32 length, uS type_size)
{
   if (graphics == NULL || res_buffer.id == INVALID_HANDLE_ID)
      return;

   gfx_Buffer buffer = graphics->buffers[res_buffer.handle];
   if (buffer.compare.ref != res_buffer.ref)
      return;

   u32 gl_target = GFX_BufferType(buffer.type);
   
   glBindBuffer(gl_target, buffer.id.buf);
   glBufferSubData(gl_target, (uS)offset * type_size, (uS)length * type_size, data);
   glBindBuffer(gl_target, 0);
   
}

void Graphics_UseBuffer(Graphics* graphics, Buffer res_buffer, u32 slot)
{
   if (graphics == NULL || res_buffer.id == INVALID_HANDLE_ID)
      return;

   gfx_Buffer buffer = graphics->buffers[res_buffer.handle];
   if (buffer.compare.ref != res_buffer.ref)
      return;

   u32 gl_target = GFX_BufferType(buffer.type);

   glBindBuffer(gl_target, buffer.id.buf);
   glBindBufferBase(gl_target, slot, buffer.id.buf);

}

u32 GFX_BufferType(u8 buffer_type)
{
   switch (buffer_type) {
      case GFX_BUFFERTYPE_UNIFORM:
         return GL_UNIFORM_BUFFER;
      case GFX_BUFFERTYPE_STORAGE:
         return GL_SHADER_STORAGE_BUFFER;
      
      default:
         return GL_UNIFORM_BUFFER;
   }
}