#include "util/files.h"
#include "util/types.h"
#include "util/resource.h"
#include "util/files.h"

#include "graphics.h"
#include "graphics/internal.h"

#include <glad/gl.h>

#include <stdlib.h>

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

      error err = { 0 };
      err.general = ERR_ERROR;
      err.extra = ERR_GFX_SHADER_COMPILATION_FAILED;

      Util_Log(NULL, GRAPHICS_MODULE, err, "Shader Failed To Compile!\n%s", shd_log);

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
      
      error err = { 0 };
      err.general = ERR_ERROR;
      err.extra = ERR_GFX_SHADER_COMPILATION_FAILED;

      Util_Log(NULL, GRAPHICS_MODULE, err, "Compute Shader Failed To Compile!\n%s", shd_log);

      return (handle){ .id = INVALID_HANDLE_ID };
   }

   glDeleteShader(shd_cmp);

   shader.id.program = shd_id;

   if (graphics->freed_shader_root == INVALID_HANDLE)
      return ADD_RESOURCE(graphics->shaders, shader);

   return REUSE_RESOURCE(graphics->shaders, shader, graphics->freed_shader_root);
}

Shader Graphics_LoadShaderFromFile(Graphics* graphics, const char* file_path, const char* defines[], const u32 define_count, bool is_compute)
{
   if (graphics == NULL || file_path == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   memblob shader_data = Util_LoadFileIntoMemory(file_path, false);
   if (shader_data.data == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   Shader res_shader = (handle){ .id = INVALID_HANDLE_ID };

   error err = { 0 };

   if (is_compute)
   {
      memblob compute_shader_code = Util_PrependShaderDefines(shader_data, defines, define_count, NULL);

      res_shader = Graphics_CreateComputeShader(graphics, compute_shader_code.data);

      if (res_shader.id == INVALID_HANDLE_ID)
      {
         
         err.general = ERR_WARN;
         err.extra = ERR_GFX_SHADER_COMPILATION_FAILED;
         
      }

      free(compute_shader_code.data);

   } else {
      memblob vert_shader_code = Util_PrependShaderDefines(shader_data, defines, define_count, "#define VERT");
      memblob frag_shader_code = Util_PrependShaderDefines(shader_data, defines, define_count, "#define FRAG");

      res_shader = Graphics_CreateShader(graphics, vert_shader_code.data, frag_shader_code.data);

      if (res_shader.id == INVALID_HANDLE_ID)
      {
         err.general = ERR_WARN;
         err.extra = ERR_GFX_SHADER_COMPILATION_FAILED;

      }

      free(vert_shader_code.data);
      free(frag_shader_code.data);

   }

   if (res_shader.id == INVALID_HANDLE_ID)
      Util_Log(NULL, GRAPHICS_MODULE, err, "Shader File [%s] Failed To Compile!", file_path);

   free(shader_data.data);

   return res_shader;
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
   
   GFX_BindUniformBlocks(graphics, uniform_blocks);

   glDispatchCompute(size_x, size_y, size_z);

   glUseProgram(0);

}

void Graphics_DispatchBarrier(Graphics* graphics)
{
   if (graphics == NULL)
      return;

   glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

}

void GFX_BindUniformBlocks(Graphics* graphics, UniformBlockList uniform_blocks)
{
   if (graphics == NULL)
      return;

   for (u32 i=0; i<uniform_blocks.count; i++)
      Graphics_BindBuffer(graphics, uniform_blocks.blocks[i].ubo, uniform_blocks.blocks[i].binding);

}
