#include "util/types.h"
#include "util/resource.h"

#include "graphics.h"
#include "graphics/internal.h"

#include <glad/gl.h>

#include <string.h>

Buffer Graphics_CreateBuffer(Graphics* graphics, void* data, u32 length, uS type_size, u8 draw_mode, u8 buffer_type)
{
   gfx_Buffer buffer = { 0 };
   buffer.type = buffer_type;
   buffer.draw_mode = draw_mode;
   buffer.compare.ref = graphics->ref;

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
   if (graphics->freed_buffer_root == GFX_INVALID_INDEX)
      return Util_AddResource(&graphics->ref, REF(graphics->buffers), &buffer);

   u16 index = (u16)graphics->freed_buffer_root;
   graphics->freed_buffer_root = graphics->buffers[index].next_freed;
   Buffer buffer_handle = { .handle = index, .ref = graphics->ref++ };
   graphics->buffers[index] = buffer;

   return buffer_handle;
}

Buffer Graphics_CreateBufferExplicit(Graphics* graphics, void* data, uS size, u8 draw_mode, u8 buffer_type)
{
   gfx_Buffer buffer = { 0 };
   buffer.type = buffer_type;
   buffer.draw_mode = draw_mode;
   buffer.compare.ref = graphics->ref;

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

   if (graphics->freed_buffer_root == GFX_INVALID_INDEX)
      return Util_AddResource(&graphics->ref, REF(graphics->buffers), &buffer);

   u16 index = (u16)graphics->freed_buffer_root;
   graphics->freed_buffer_root = graphics->buffers[index].next_freed;
   Buffer buffer_handle = { .handle = index, .ref = graphics->ref++ };
   graphics->buffers[index] = buffer;

   return buffer_handle;
}

void Graphics_ReuseBuffer(Graphics* graphics, void* data, u32 length, uS type_size, Buffer res_buffer)
{
   gfx_Buffer buffer = graphics->buffers[res_buffer.handle];
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

void Graphics_FreeBuffer(Graphics* graphics, Buffer res_buffer)
{
   gfx_Buffer buffer = graphics->buffers[res_buffer.handle];
   if (buffer.compare.ref != res_buffer.ref)
      return;

   buffer.next_freed = graphics->freed_buffer_root;
   graphics->freed_buffer_root = (u32)res_buffer.handle;

   glDeleteBuffers(1, &buffer.id.buf);
}

void Graphics_UpdateBuffer(Graphics* graphics, Buffer res_buffer, void* data, u32 length, uS type_size)
{
   gfx_Buffer buffer = graphics->buffers[res_buffer.handle];
   if (buffer.compare.ref != res_buffer.ref)
      return;

   u32 gl_target = GFX_BufferType(buffer.type);
   
   glBindBuffer(gl_target, buffer.id.buf);
   glBufferSubData(gl_target, 0, (uS)length * type_size, data);
   glBindBuffer(gl_target, 0);
}

void Graphics_UpdateBufferRange(Graphics* graphics, Buffer res_buffer, void* data, u32 offset, u32 length, uS type_size)
{
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