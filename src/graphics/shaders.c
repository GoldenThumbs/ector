#include "util/types.h"
#include "util/array.h"
#include "util/resource.h"

#include "graphics.h"
#include "graphics/internal.h"

#include <glad/gl.h>

#include <stdio.h>
#include <stdlib.h>

Shader Graphics_CreateShader(GraphicsContext* context, const char* vertex_shader, const char* fragment_shader)
{
   gfx_Shader shader = { 0 };
   shader.is_compute = false;
   shader.compare.ref = context->ref;

   u32 shd_vrt = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(shd_vrt, 1, &vertex_shader, NULL);
   glCompileShader(shd_vrt);

   i32 vrt_sucess = 1;
   glGetShaderiv(shd_vrt, GL_COMPILE_STATUS, &vrt_sucess);
   if (!vrt_sucess)
   {
      char shd_log[1024] = { 0 };
      glGetShaderInfoLog(shd_vrt, sizeof(shd_log), NULL, shd_log);
      fprintf(stderr, "[VERT]\n%s\n", shd_log);
   }

   u32 shd_frg = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(shd_frg, 1, &fragment_shader, NULL);
   glCompileShader(shd_frg);

   i32 frg_sucess = 1;
   glGetShaderiv(shd_frg, GL_COMPILE_STATUS, &frg_sucess);
   if (!frg_sucess)
   {
      char shd_log[1024] = { 0 };
      glGetShaderInfoLog(shd_frg, sizeof(shd_log), NULL, shd_log);
      fprintf(stderr, "[FRAG]\n%s\n", shd_log);
   }

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
   }

   glDeleteShader(shd_vrt);
   glDeleteShader(shd_frg);
   
   shader.id.program = shd_id;
   shader.compare.handle = Util_ArrayLength(context->shaders);
   return Util_AddResource(&context->ref, REF(context->shaders), &shader);
}

Shader Graphics_CreateComputeShader(GraphicsContext* context, const char* compute_shader)
{
   gfx_Shader shader = { 0 };
   shader.is_compute = true;
   shader.compare.ref = context->ref;

   u32 shd_cmp = glCreateShader(GL_COMPUTE_SHADER);
   glShaderSource(shd_cmp, 1, &compute_shader, NULL);
   glCompileShader(shd_cmp);

   i32 cmp_sucess = 1;
   glGetShaderiv(shd_cmp, GL_COMPILE_STATUS, &cmp_sucess);
   if (!cmp_sucess)
   {
      char shd_log[1024] = { 0 };
      glGetShaderInfoLog(shd_cmp, sizeof(shd_log), NULL, shd_log);
      fprintf(stderr, "[COMP]\n%s\n", shd_log);
   }

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
   }

   glDeleteShader(shd_cmp);

   shader.id.program = shd_id;
   return Util_AddResource(&context->ref, REF(context->shaders), &shader);
}

void Graphics_FreeShader(GraphicsContext* context, Shader res_shader)
{
   gfx_Shader shader = context->shaders[res_shader.handle];
   if (shader.compare.ref != res_shader.ref)
      return;

   glDeleteProgram(shader.id.program);
}

u32 Graphics_GetUniformLocation(GraphicsContext* context, Shader res_shader, const char* name)
{
   gfx_Shader shader = context->shaders[res_shader.handle];
   if (shader.compare.ref != res_shader.ref)
      return UINT32_MAX;

   return glGetUniformLocation(shader.id.program, name);
}

void Graphics_Dispatch(GraphicsContext* context, Shader res_shader, u32 size_x, u32 size_y, u32 size_z, u32 uniform_count, const Uniform* uniforms)
{
   gfx_Shader shader = context->shaders[res_shader.handle];
   if (shader.compare.ref != res_shader.ref)
      return;
   if (!shader.is_compute)
      return;

   glUseProgram(shader.id.program);
   for (u32 i=0; i<uniform_count; i++)
      GFX_SetUniform(uniforms[i]);

   glDispatchCompute(size_x, size_y, size_z);

   glUseProgram(0);
}

void Graphics_DispatchBarrier(void)
{
   glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}
