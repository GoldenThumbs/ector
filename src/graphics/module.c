#include "util/types.h"
#include "util/array.h"
#include "module_glue.h"

#include "graphics.h"
#include "graphics/internal.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GraphicsContext* MOD_InitGraphics(error* err)
{
   if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress))
   {
      err->general = ERR_FATAL;
      err->extra = ERR_GFX_CONTEXT_FAILED;
      err->flags = ERR_FLAG_GRAPHICS_FAILED;
      return NULL;
   }

   GraphicsContext* context = malloc(sizeof(GraphicsContext));
   context->shaders = NEW_ARRAY(gfx_Shader);
   context->buffers = NEW_ARRAY(gfx_Buffer);
   context->geometries = NEW_ARRAY(gfx_Geometry);
   context->ref = 0;
   context->clear_buffers.color = 1;
   context->clear_buffers.depth = 1;
   context->clear_buffers.stencil = 0;
   context->clear_color.hex = 0;

   Graphics_SetClearColor(context, (color8){ 127, 127, 127, 255 });
   glEnable(GL_CULL_FACE);

   return context;
}

void MOD_FreeGraphics(GraphicsContext* context)
{
   if (context == NULL)
      return;
   
   for (u32 i=0; i<Util_ArrayLength(context->shaders); i++)
      glDeleteProgram(context->shaders[i].id.program);

   for (u32 i=0; i<Util_ArrayLength(context->buffers); i++)
      glDeleteBuffers(1, &context->buffers[i].id.buf);

   for (u32 i=0; i<Util_ArrayLength(context->buffers); i++)
   {
      if (context->geometries[i].id.vao == 0)
         continue;
      
      glDeleteVertexArrays(1, &context->geometries[i].id.vao);

      if (context->geometries[i].id.v_buf != 0)
         glDeleteBuffers(1, &context->geometries[i].id.v_buf);
      if (context->geometries[i].id.i_buf != 0)
         glDeleteBuffers(1, &context->geometries[i].id.i_buf);
   }
   FREE_ARRAY(context->geometries);
   context->ref = 0;
   free(context);
}

void Graphics_SetClearColor(GraphicsContext* context, color8 clear_color)
{
   context->clear_color = clear_color;

   const f32 rcp_byte = 1.0f / 255.0f;
   f32 r = (f32)clear_color.r * rcp_byte;
   f32 g = (f32)clear_color.g * rcp_byte;
   f32 b = (f32)clear_color.b * rcp_byte;
   f32 a = (f32)clear_color.a * rcp_byte;

   glClearColor(r, g, b, a);
}

void Graphics_Viewport(GraphicsContext* context, resolution2d size)
{
   glViewport(0, 0, size.width, size.height);
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LESS);

   u32 color_bit = context->clear_buffers.color * GL_COLOR_BUFFER_BIT;
   u32 depth_bit = context->clear_buffers.depth * GL_DEPTH_BUFFER_BIT;
   u32 stencil_bit = context->clear_buffers.stencil * GL_STENCIL_BUFFER_BIT;
   glClear(color_bit | depth_bit | stencil_bit);
}

void Graphics_Draw(GraphicsContext* context, Shader res_shader, Geometry res_geometry, Uniforms uniforms)
{
   gfx_Shader shader = context->shaders[res_shader.handle];
   if (shader.compare.ref != res_shader.ref)
      return;
   if (shader.is_compute)
      return;

   gfx_Geometry geometry = context->geometries[res_geometry.handle];
   if (geometry.compare.ref != res_geometry.ref)
      return;

   switch (geometry.face_cull)
   {
      case GFX_FACECULL_BACK:
         glCullFace(GL_BACK);
      break;

      case GFX_FACECULL_FRONT:
         glCullFace(GL_FRONT);
      break;

      default:
      break;
   }

   glUseProgram(shader.id.program);
   for (u32 i=0; i<uniforms.count; i++)
      Graphics_UseBuffer(context, uniforms.blocks[i].ubo, uniforms.blocks[i].binding);

   u32 prim = GFX_Primitive(geometry.primitive);
   glBindVertexArray(geometry.id.vao);
   if (geometry.id.i_buf == 0)
      glDrawArrays(prim, 0, geometry.element_count);
   else {
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.id.i_buf);
      glDrawElements(prim, geometry.element_count, GL_UNSIGNED_SHORT, (void*)0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
   }

   glBindVertexArray(0);
   glUseProgram(0);
}

u32 GFX_AttributeType(u8 attribute)
{
   switch (attribute)
   {
      case GFX_ATTRIBUTE_F32_1X:
      case GFX_ATTRIBUTE_F32_2X:
      case GFX_ATTRIBUTE_F32_3X:
      case GFX_ATTRIBUTE_F32_4X:
         return GL_FLOAT;

      case GFX_ATTRIBUTE_U8_4X_NORM:
         return GL_BYTE;

      default:
         return 0;
   }
}

i32 GFX_AttributeTypeCount(u8 attribute)
{
   switch (attribute)
   {
      case GFX_ATTRIBUTE_F32_1X:
         return 1;

      case GFX_ATTRIBUTE_F32_2X:
         return 2;

      case GFX_ATTRIBUTE_F32_3X:
         return 3;

      case GFX_ATTRIBUTE_F32_4X:
      case GFX_ATTRIBUTE_U8_4X_NORM:
         return 4;
      
      default:
         return 0;
   }
}

bool GFX_AttributeTypeNormalized(u8 attribute)
{
   {
      switch (attribute)
      {
         case GFX_ATTRIBUTE_U8_4X_NORM:
            return true;
         
         default:
            return false;
      }
   }
}

uS GFX_AttributeTypeSize(u8 attribute)
{
   switch (attribute)
   {
      case GFX_ATTRIBUTE_F32_1X:
      case GFX_ATTRIBUTE_F32_2X:
      case GFX_ATTRIBUTE_F32_3X:
      case GFX_ATTRIBUTE_F32_4X:
         return sizeof(f32);
      
      case GFX_ATTRIBUTE_U8_4X_NORM:
         return sizeof(u8);
      
      default:
         return 0;
   }
}

uS GFX_VertexBufferSize(u16 vertex_count, u8* attributes, u16 attribute_count)
{
   uS buffer_size = 0;
   for (u16 i=0; i<attribute_count; i++)
   {
      u8 a = attributes[i];
      buffer_size += GFX_AttributeTypeSize(a) * (uS)GFX_AttributeTypeCount(a) * (uS)vertex_count;
   }

   return buffer_size;
}

u32 GFX_Primitive(u8 primitive_type)
{
   switch (primitive_type)
   {
      case GFX_PRIMITIVE_POINT:
         return GL_POINTS;
      
      case GFX_PRIMITIVE_LINE:
         return GL_LINES;
      
      case GFX_PRIMITIVE_TRIANGLE:
         return GL_TRIANGLES;
      
      default:
         return GL_TRIANGLES;
   }
}

u32 GFX_DrawMode(u8 draw_mode)
{
   switch (draw_mode)
   {
      case GFX_DRAWMODE_STATIC:
         return GL_STATIC_DRAW;
      case GFX_DRAWMODE_DYNAMIC:
         return GL_DYNAMIC_DRAW;
      case GFX_DRAWMODE_STREAM:
         return GL_STREAM_DRAW;
      case GFX_DRAWMODE_STATIC_READ:
         return GL_STATIC_READ;
      case GFX_DRAWMODE_STATIC_COPY:
         return GL_STATIC_COPY;
      case GFX_DRAWMODE_DYNAMIC_READ:
         return GL_DYNAMIC_READ;
      case GFX_DRAWMODE_DYNAMIC_COPY:
         return GL_DYNAMIC_COPY;
      case GFX_DRAWMODE_STREAM_READ:
         return GL_STREAM_READ;
      case GFX_DRAWMODE_STREAM_COPY:
         return GL_STREAM_COPY;
      
      default:
         return GL_DYNAMIC_DRAW;
   }
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

void GFX_SetUniform(Uniform uniform)
{
   switch (uniform.uniform_type)
   {
      case GFX_UNIFORMTYPE_F32_1X:
         glUniform1fv(uniform.location, 1, uniform.as_float);
      break;
      
      case GFX_UNIFORMTYPE_F32_2X:
         glUniform2fv(uniform.location, 1, uniform.as_float);
      break;

      case GFX_UNIFORMTYPE_F32_3X:
         glUniform3fv(uniform.location, 1, uniform.as_float);
      break;

      case GFX_UNIFORMTYPE_F32_4X:
         glUniform4fv(uniform.location, 1, uniform.as_float);
      break;

      case GFX_UNIFORMTYPE_MAT3:
         glUniformMatrix3fv(uniform.location, 1, GL_FALSE, uniform.as_float);
      break;
      
      case GFX_UNIFORMTYPE_MAT4:
         glUniformMatrix4fv(uniform.location, 1, GL_FALSE, uniform.as_float);
      break;

      case GFX_UNIFORMTYPE_U32_1X:
         glUniform1uiv(uniform.location, 1, uniform.as_uint);
      break;

      case GFX_UNIFORMTYPE_U32_2X:
         glUniform2uiv(uniform.location, 1, uniform.as_uint);
      break;

      case GFX_UNIFORMTYPE_U32_3X:
         glUniform3uiv(uniform.location, 1, uniform.as_uint);
      break;

      case GFX_UNIFORMTYPE_U32_4X:
         glUniform4uiv(uniform.location, 1, uniform.as_uint);
      break;

      case GFX_UNIFORMTYPE_TEX_SLOT:
         glUniform1i(uniform.location, uniform.texslot);
      break;
      
      default:
         break;
   }
}
