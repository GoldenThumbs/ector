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
   graphics->framebuffers = NEW_ARRAY(gfx_Framebuffer);
   graphics->freed_shader_root = INVALID_HANDLE;
   graphics->freed_buffer_root = INVALID_HANDLE;
   graphics->freed_geometry_root = INVALID_HANDLE;
   graphics->freed_texture_root = INVALID_HANDLE;
   graphics->freed_framebuffer_root = INVALID_HANDLE;
   graphics->state.state_id = 0;
   graphics->state.enable_color_clear = true;
   graphics->state.enable_depth_clear = true;
   graphics->state.enable_stencil_clear = false;
   graphics->clear_color.hex = 0;

   Graphics_SetClearColor(graphics, (color8){ 127, 127, 127, 255 });
   Graphics_SetClearDepth(graphics, 1.0f);
   Graphics_SetClearStencilId(graphics, 0);
   Graphics_SetBlending(graphics, GFX_BLENDMODE_MIX);
   Graphics_SetDepthTest(graphics, GFX_DEPTHMODE_LESS_THAN);
   GFX_SetFaceCullMode(graphics, GFX_FACECULL_BACK);

   return graphics;
}

void Graphics_Free(Graphics* graphics)
{
   if (graphics == NULL)
      return;
   
   for (u32 i=0; i<Util_ArrayLength(graphics->shaders); i++)
      Graphics_FreeShader(graphics, graphics->shaders[i].compare);

   for (u32 i=0; i<Util_ArrayLength(graphics->buffers); i++)
      Graphics_FreeBuffer(graphics, graphics->buffers[i].compare);

   for (u32 i=0; i<Util_ArrayLength(graphics->geometries); i++)
      Graphics_FreeGeometry(graphics, graphics->geometries[i].compare);

   for (u32 i=0; i<Util_ArrayLength(graphics->textures); i++)
      Graphics_FreeTexture(graphics, graphics->textures[i].compare);

   for (u32 i=0; i<Util_ArrayLength(graphics->framebuffers); i++)
      Graphics_FreeFramebuffer(graphics, graphics->framebuffers[i].compare);

   FREE_ARRAY(graphics->shaders);
   FREE_ARRAY(graphics->buffers);
   FREE_ARRAY(graphics->geometries);
   FREE_ARRAY(graphics->textures);
   FREE_ARRAY(graphics->framebuffers);

   free(graphics);
   

   
}

void Graphics_Clear(Graphics* graphics)
{
   if (graphics == NULL)
      return;

   u32 color_bit = (graphics->state.enable_color_clear) ? GL_COLOR_BUFFER_BIT : 0;
   u32 depth_bit = (graphics->state.enable_depth_clear) ? GL_DEPTH_BUFFER_BIT : 0;
   u32 stencil_bit = (graphics->state.enable_stencil_clear) ? GL_STENCIL_BUFFER_BIT : 0;

   glClear(color_bit | depth_bit | stencil_bit);

}

void Graphics_Viewport(Graphics* graphics, resolution2d size)
{
   glViewport(0, 0, size.width, size.height);

}

void Graphics_OffsetViewport(Graphics* graphics, resolution2d size, i32 offset_x, i32 offset_y)
{
   glViewport(offset_x, offset_y, size.width, size.height);

}

void Graphics_EnableColorClear(Graphics* graphics, bool enable_color_clear)
{
   if (graphics == NULL)
      return;

   graphics->state.enable_color_clear = enable_color_clear;
   
}

void Graphics_EnableDepthClear(Graphics* graphics, bool enable_depth_clear)
{
   if (graphics == NULL)
      return;

   graphics->state.enable_depth_clear = enable_depth_clear;

}

void Graphics_EnableStencilClear(Graphics* graphics, bool enable_stencil_clear)
{
   if (graphics == NULL)
      return;

   graphics->state.enable_stencil_clear = enable_stencil_clear;
   
}

void Graphics_SetClearColor(Graphics* graphics, color8 clear_color)
{
   if (graphics == NULL)
      return;

   graphics->clear_color = clear_color;

   f32 r = (f32)BYTE_TO_F32(clear_color.r);
   f32 g = (f32)BYTE_TO_F32(clear_color.g);
   f32 b = (f32)BYTE_TO_F32(clear_color.b);
   f32 a = (f32)BYTE_TO_F32(clear_color.a);

   glClearColor(r, g, b, a);

}

void Graphics_SetClearDepth(Graphics* graphics, f32 clear_depth)
{
   if (graphics == NULL)
      return;

   graphics->clear_depth = clear_depth;

   glClearDepthf(clear_depth);

}

void Graphics_SetClearStencilId(Graphics* graphics, i32 clear_stencil_id)
{
   if (graphics == NULL)
      return;

   graphics->clear_stencil_id = clear_stencil_id;

   glClearStencil(clear_stencil_id);

}

void Graphics_SetBlending(Graphics* graphics, u8 blend_mode)
{
   if (graphics == NULL)
      return;

   bool blend_enable = (blend_mode != GFX_BLENDMODE_NONE);

   if ((bool)graphics->state.blend_enable != blend_enable)
   {
      graphics->state.blend_enable = (u16)blend_enable;

      if(blend_enable)
         glEnable(GL_BLEND);
      else
         glDisable(GL_BLEND);
   }

   if ((graphics->state.blend_mode != blend_mode) && blend_enable)
   {
      switch (blend_mode)
      {
         case GFX_BLENDMODE_MIX:
            glBlendEquation(GL_FUNC_ADD);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
         break;

         case GFX_BLENDMODE_ADD:
            glBlendEquation(GL_FUNC_ADD);
            glBlendFunc(GL_ONE, GL_ONE);
         break;

         case GFX_BLENDMODE_SUB:
            glBlendEquation(GL_FUNC_SUBTRACT);
            glBlendFunc(GL_ONE, GL_ONE);
         break;

         case GFX_BLENDMODE_MUL:
            glBlendEquation(GL_FUNC_ADD);
            glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
         break;

         default:
            break;
      }
   }

   graphics->state.blend_mode = blend_mode;

}

void Graphics_SetDepthTest(Graphics* graphics, u8 depth_mode)
{
   if (graphics == NULL)
      return;

   bool depthtest_enable = (depth_mode != GFX_DEPTHMODE_NONE);

   if ((bool)graphics->state.depthtest_enable != depthtest_enable)
   {
      graphics->state.depthtest_enable = (u16)depthtest_enable;

      if(depthtest_enable)
         glEnable(GL_DEPTH_TEST);
      else
         glDisable(GL_DEPTH_TEST);
   }

   if ((graphics->state.depth_mode != depth_mode) && depthtest_enable)
   {
      switch (depth_mode)
      {
         case GFX_DEPTHMODE_LESS_THAN:
            glDepthFunc(GL_LESS);
         break;

         case GFX_DEPTHMODE_LESS_THAN_OR_EQUAL:
            glDepthFunc(GL_LEQUAL);
         break;

         case GFX_DEPTHMODE_GREATER_THAN:
            glDepthFunc(GL_GREATER);
         break;

         case GFX_DEPTHMODE_GREATER_THAN_OR_EQUAL:
            glDepthFunc(GL_GEQUAL);
         break;

         case GFX_DEPTHMODE_EQUAL_TO:
            glDepthFunc(GL_EQUAL);
         break;

         case GFX_DEPTHMODE_NOT_EQUAL_TO:
            glDepthFunc(GL_NOTEQUAL);
         break;

         case GFX_DEPTHMODE_ALWAYS_OVER:
            glDepthFunc(GL_ALWAYS);
         break;

         default:
            break;
      }
   }

   graphics->state.depth_mode = depth_mode;

}

void Graphics_SetDepthMask(Graphics* graphics, bool depth_mask)
{
   if (graphics == NULL)
      return;

   if ((bool)graphics->state.depthmask_enable != depth_mask)
      glDepthMask((GLboolean)depth_mask);
   
   graphics->state.depthmask_enable = depth_mask;

}

void Graphics_Draw(Graphics* graphics, Shader res_shader, Geometry res_geometry, UniformBlockList uniforms)
{
   Graphics_DrawInstanced(graphics, res_shader, res_geometry, 0, uniforms);

}

void Graphics_DrawInstanced(Graphics* graphics, Shader res_shader, Geometry res_geometry, u32 instance_count, UniformBlockList uniforms)
{
   if (graphics == NULL || res_shader.id == INVALID_HANDLE_ID || res_geometry.id == INVALID_HANDLE_ID)
      return;

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
   GFX_DrawVertices(geometry.primitive, geometry.element_count, (geometry.id.i_buf != 0), geometry.index_type, geometry.id.vao, 0, instance_count);

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
