#include "util/types.h"
#include "util/resource.h"

#include "graphics.h"
#include "graphics/internal.h"

#include <glad/gl.h>

#include <stdio.h>

Shader Graphics_CreateShader(Graphics* graphics, const char* vertex_shader, const char* fragment_shader)
{
   if (graphics == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };
   
   gfx_Shader shader = { 0 };
   shader.is_compute = false;

   u32 shd_vrt = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(shd_vrt, 1, &vertex_shader, NULL);
   glCompileShader(shd_vrt);

   u32 shd_frg = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(shd_frg, 1, &fragment_shader, NULL);
   glCompileShader(shd_frg);

   u32 shd_id = glCreateProgram();
   glAttachShader(shd_id, shd_vrt);
   glAttachShader(shd_id, shd_frg);
   glLinkProgram(shd_id);

   i32 shd_sucess = 1;
   glGetProgramiv(shd_id, GL_LINK_STATUS, &shd_sucess);
   if (!shd_sucess)
   {
      char shd_log[1024] = { 0 };
      glGetProgramInfoLog(shd_id, sizeof(shd_log), NULL, shd_log);
      fprintf(stderr, "%s\n", shd_log);

      return (handle){ .id = INVALID_HANDLE_ID };
   }

   glDeleteShader(shd_vrt);
   glDeleteShader(shd_frg);
   
   shader.id.program = shd_id;

   if (graphics->freed_shader_root == INVALID_HANDLE)
      return ADD_RESOURCE(graphics->shaders, shader);

   return REUSE_RESOURCE(graphics->shaders, shader, graphics->freed_shader_root);
}

Shader Graphics_CreateComputeShader(Graphics* graphics, const char* compute_shader)
{
   if (graphics == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };
   
   gfx_Shader shader = { 0 };
   shader.is_compute = true;

   u32 shd_cmp = glCreateShader(GL_COMPUTE_SHADER);
   glShaderSource(shd_cmp, 1, &compute_shader, NULL);
   glCompileShader(shd_cmp);

   u32 shd_id = glCreateProgram();
   glAttachShader(shd_id, shd_cmp);
   glLinkProgram(shd_id);

   i32 shd_sucess = 1;
   glGetProgramiv(shd_id, GL_LINK_STATUS, &shd_sucess);
   if (!shd_sucess)
   {
      char shd_log[1024] = { 0 };
      glGetProgramInfoLog(shd_id, sizeof(shd_log), NULL, shd_log);
      fprintf(stderr, "%s\n", shd_log);

      return (handle){ .id = INVALID_HANDLE_ID };
   }

   glDeleteShader(shd_cmp);

   shader.id.program = shd_id;

   if (graphics->freed_shader_root == INVALID_HANDLE)
      return ADD_RESOURCE(graphics->shaders, shader);

   return REUSE_RESOURCE(graphics->shaders, shader, graphics->freed_shader_root);
}

void Graphics_FreeShader(Graphics* graphics, Shader res_shader)
{
   if (graphics == NULL || res_shader.id == INVALID_HANDLE_ID)
      return;
   gfx_Shader shader = graphics->shaders[res_shader.handle];
   if (shader.compare.ref != res_shader.ref)
      return;

   shader.next_freed = graphics->freed_shader_root;
   graphics->freed_shader_root = (u32)res_shader.handle;

   glDeleteProgram(shader.id.program);
}

u32 Graphics_GetUniformLocation(Graphics* graphics, Shader res_shader, const char* name)
{
   if (graphics == NULL || res_shader.id == INVALID_HANDLE_ID)
      return UINT32_MAX;

   gfx_Shader shader = graphics->shaders[res_shader.handle];
   if (shader.compare.ref != res_shader.ref)
      return UINT32_MAX;

   return glGetUniformLocation(shader.id.program, name);
}

void Graphics_SetUniform(Graphics* graphics, Uniform uniform)
{
   if (graphics == NULL)
      return;

   GFX_SetUniform(uniform);
}

void Graphics_Dispatch(Graphics* graphics, Shader res_shader, u32 size_x, u32 size_y, u32 size_z, UniformBlockList uniform_blocks)
{
   if (graphics == NULL || res_shader.id == INVALID_HANDLE_ID)
      return;

   gfx_Shader shader = graphics->shaders[res_shader.handle];
   if (shader.compare.ref != res_shader.ref)
      return;
   if (!shader.is_compute)
      return;

   glUseProgram(shader.id.program);
   
   GFX_UseUniformBlocks(graphics, uniform_blocks);

   glDispatchCompute(size_x, size_y, size_z);

   glUseProgram(0);
}

void Graphics_DispatchBarrier(void)
{
   glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void GFX_UseUniformBlocks(Graphics* graphics, UniformBlockList uniform_blocks)
{
   if (graphics == NULL)
      return;

   for (u32 i=0; i<uniform_blocks.count; i++)
      Graphics_UseBuffer(graphics, uniform_blocks.blocks[i].ubo, uniform_blocks.blocks[i].binding);
}
