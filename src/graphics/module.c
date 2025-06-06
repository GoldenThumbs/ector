#include "util/types.h"
#include "util/array.h"

#include "graphics.h"
#include "graphics/internal.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <string.h>

Graphics* Graphics_Init(void)
{
   if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress))
   {
      return NULL;
   }

   Graphics* graphics = malloc(sizeof(Graphics));
   graphics->shaders = NEW_ARRAY(gfx_Shader);
   graphics->buffers = NEW_ARRAY(gfx_Buffer);
   graphics->geometries = NEW_ARRAY(gfx_Geometry);
   graphics->textures = NEW_ARRAY(gfx_Texture);
   graphics->ref = 0;
   graphics->clear_color.hex = 0;

   Graphics_SetClearColor(graphics, (color8){ 127, 127, 127, 255 });
   glEnable(GL_CULL_FACE);

   return graphics;
}

void Graphics_Free(Graphics* graphics)
{
   if (graphics == NULL)
      return;
   
   for (u32 i=0; i<Util_ArrayLength(graphics->shaders); i++)
      glDeleteProgram(graphics->shaders[i].id.program);

   for (u32 i=0; i<Util_ArrayLength(graphics->buffers); i++)
      glDeleteBuffers(1, &graphics->buffers[i].id.buf);

   for (u32 i=0; i<Util_ArrayLength(graphics->buffers); i++)
   {
      if (graphics->geometries[i].id.vao == 0)
         continue;
      
      glDeleteVertexArrays(1, &graphics->geometries[i].id.vao);

      if (graphics->geometries[i].id.v_buf != 0)
         glDeleteBuffers(1, &graphics->geometries[i].id.v_buf);
      if (graphics->geometries[i].id.i_buf != 0)
         glDeleteBuffers(1, &graphics->geometries[i].id.i_buf);
   }

   for (u32 i=0; i<Util_ArrayLength(graphics->textures); i++)
      glDeleteTextures(1, &graphics->textures[i].id.tex);

   FREE_ARRAY(graphics->geometries);
   graphics->ref = 0;
   free(graphics);
}

void Graphics_SetClearColor(Graphics* graphics, color8 clear_color)
{
   graphics->clear_color = clear_color;

   const f32 rcp_byte = 1.0f / 255.0f;
   f32 r = (f32)clear_color.r * rcp_byte;
   f32 g = (f32)clear_color.g * rcp_byte;
   f32 b = (f32)clear_color.b * rcp_byte;
   f32 a = (f32)clear_color.a * rcp_byte;

   glClearColor(r, g, b, a);
}

void Graphics_Viewport(Graphics* graphics, resolution2d size)
{
   glViewport(0, 0, size.width, size.height);
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LESS);

   u32 color_bit = GL_COLOR_BUFFER_BIT;
   u32 depth_bit = GL_DEPTH_BUFFER_BIT;
   u32 stencil_bit = GL_STENCIL_BUFFER_BIT;
   glClear(color_bit | depth_bit | stencil_bit);
}

void Graphics_Draw(Graphics* graphics, Shader res_shader, Geometry res_geometry, UniformBlockList uniforms)
{
   gfx_Shader shader = graphics->shaders[res_shader.handle];
   if (shader.compare.ref != res_shader.ref)
      return;
   if (shader.is_compute)
      return;

   gfx_Geometry geometry = graphics->geometries[res_geometry.handle];
   if (geometry.compare.ref != res_geometry.ref)
      return;

   GFX_SetFaceCullMode(graphics, geometry.face_cull_mode);

   glUseProgram(shader.id.program);

   GFX_UseUniformBlocks(graphics, uniforms);
   GFX_DrawVertices(geometry.primitive, geometry.element_count, (geometry.id.i_buf != 0), geometry.id.vao, 0);

   glUseProgram(0);
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
