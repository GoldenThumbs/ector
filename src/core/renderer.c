#include "util/math.h"
#include "util/types.h"

#include "util/array.h"
#include "core/renderer.h"
#include "core/renderer_i.h"

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>

struct Renderer_t
{
   RNDR_DrawCall* draw_call_array;

   View* view_array;
   u32 active_view;

   struct {
      u32 samples;
      PixelFormat color_format;
      PixelFormat depth_format;
      Target target;
      bool is_dirty;
   } screen_out;

   Texture no_texture;
   Shader pfx_passthru;
   Geometry pfx_quad;
   vec2 render_scale;
};

Renderer* Renderer_Init(RendererDesc* renderer_desc)
{
   Renderer* renderer = calloc(1, sizeof(Renderer));
   renderer->draw_call_array = NEW_ARRAY_N(RNDR_DrawCall, 512);
   renderer->view_array = NEW_ARRAY_N(View, 1);
   renderer->active_view = 0;
   renderer->screen_out.samples = 1;
   renderer->screen_out.color_format = renderer_desc->color_format;
   renderer->screen_out.depth_format = renderer_desc->depth_format;
   renderer->screen_out.target = (Target){ .id = 0 };
   renderer->screen_out.is_dirty = true;
   renderer->render_scale = VEC2(1, 1);

   SET_ARRAY_LENGTH(renderer->draw_call_array, 0);

   if (renderer_desc->samples != 0)
      renderer->screen_out.samples = renderer_desc->samples;
   if (renderer_desc->render_scale.x > 0)
      renderer->render_scale.x = renderer_desc->render_scale.x;
   if (renderer_desc->render_scale.y > 0)
      renderer->render_scale.y = renderer_desc->render_scale.y;

   (void)gladLoadGL((GLADloadfunc)glfwGetProcAddress);

   u8 no_tex_img[8*8*4] = { 0 };
   for (i32 i=0; i<(8*8); i++)
   {
      i32 x = (i / 8);
      i32 y = i - (x * 8);
      i32 bars_h = x % 2;
      i32 bars_v = y % 2;
      i32 boxes = ((x / 2) % 2) ^ ((y / 2) % 2);
      u8 gray = (u8)(64 + boxes * 64 + (bars_v ^ bars_h) * 4);
      no_tex_img[i*4 + 0] = gray;
      no_tex_img[i*4 + 1] = gray;
      no_tex_img[i*4 + 2] = gray;
      no_tex_img[i*4 + 3] = 255;
   }

   renderer->no_texture = Renderer_CreateTexture((Image){ (void*)no_tex_img, { 8, 8 }, 1, 1, TEX_2D, PIX_RGBA_8BPC });

   renderer->pfx_passthru = Renderer_CreateShader((ShaderDesc){
      .stage_src = {
         [SHD_VERTEX] =
            "#version 330 core\n"
            "layout(location = 0) in vec3 vrt_position;\n"
            "layout(location = 1) in vec2 vrt_texcoord;\n"
            "out vec2 v2f_texcoord;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = vec4(vrt_position, 1);\n"
            "   v2f_texcoord = vrt_texcoord;\n"
            "}\n",
         [SHD_FRAGMENT] =
            "#version 330 core\n"
            "uniform sampler2D u_texture;\n"
            "in vec2 v2f_texcoord;\n"
            "out vec4 frg_color;\n"
            "void main()\n"
            "{\n"
            "   frg_color = textureLod(u_texture, v2f_texcoord, 0);\n"
            "}\n"
      }
   });

   f32 quad_vrt[][5] = {
      {-1,-1, 0,   0, 0 },
      {-1, 1, 0,   0, 1 },
      { 1, 1, 0,   1, 1 },
      { 1,-1, 0,   1, 0 }
   };

   u16 quad_idx[] = {
      0, 1, 2,
      2, 3, 0
   };

   renderer->pfx_quad = Renderer_CreateGeometry(
      (Buffer){ .data = (void*)quad_vrt, .count = 4, .size = sizeof(f32) * 5u },
      (Buffer){ .data = (void*)quad_idx, .count = 6, .size = sizeof(u16) },
      (VertexDesc){
         .count = 2,
         .index_type = IDX_U16,
         .attribute = {
            [0] = { .format = ATR_F32_V3 },
            [1] = { .format = ATR_F32_V2, .offset = sizeof(f32) * 3 }
         }
      }
   );

   return renderer;
}

void Renderer_Free(Renderer* renderer)
{
   FREE_ARRAY(renderer->draw_call_array);
   FREE_ARRAY(renderer->view_array);
   Renderer_FreeTarget(&renderer->screen_out.target);
   Renderer_FreeShader(&renderer->pfx_passthru);
   Renderer_FreeGeometry(&renderer->pfx_quad);
   free(renderer);
}

size2i Renderer_ScaleFrameSize(Renderer* renderer, size2i frame_size)
{
   vec2 render_sizef = VEC2((f32)frame_size.width, (f32)frame_size.height);
   render_sizef = Util_MulVec2(render_sizef, renderer->render_scale);

   return (size2i){ (i32)render_sizef.x, (i32)render_sizef.y };
}

void Renderer_SetFrameSize(Renderer* renderer, size2i size)
{
   View* view = &renderer->view_array[renderer->active_view];
   view->frame_size = size;
}

size2i Renderer_GetFrameSize(Renderer* renderer)
{
   View view = renderer->view_array[renderer->active_view];
   return view.frame_size;
}

void Renderer_SetViewProjMatrix(Renderer* renderer, mat4x4 view_proj)
{
   View* view = &renderer->view_array[renderer->active_view];
   view->view_proj = view_proj;
}

mat4x4 Renderer_GetViewProjMatrix(Renderer* renderer)
{
   View view = renderer->view_array[renderer->active_view];
   return view.view_proj;
}

void Renderer_SubmitDrawCall(Renderer* renderer, DrawCall drawcall)
{
   RNDR_DrawCall drawcall_internal = {
      .uniform_count = drawcall.uniform_count,
      .texture_count = drawcall.texture_count,
      .shader_id = drawcall.shader.id,
      .geometry = drawcall.geometry,
      .uniforms = { 0 },
      .textures = { 0 }
   };

   for (i32 i=0; i<drawcall.uniform_count; i++)
   {
      u32 location = drawcall.uniforms[i].location;
      if (drawcall.uniforms[i].name != NULL)
         location = glGetUniformLocation(drawcall.shader.id, drawcall.uniforms[i].name);
      drawcall_internal.uniforms[i].location = location;
      drawcall_internal.uniforms[i].type = drawcall.uniforms[i].type;
      drawcall_internal.uniforms[i].datablob = drawcall.uniforms[i].datablob;
   }

   for (i32 i=0; i<drawcall.texture_count; i++)
   {
      drawcall_internal.textures[i] = drawcall.textures[i];

   }

   ADD_BACK_ARRAY(renderer->draw_call_array, drawcall_internal);
}

Texture Renderer_CreateTexture(Image image)
{
   ImageDesc desc = {
      .size = image.size,
      .depth = image.depth,
      .mipmaps = image.mipmaps,
      .type = image.type,
      .format = image.format
   };

   Texture texture = {
      .size = desc.size,
      .depth = desc.depth,
      .mipmaps = desc.mipmaps,
      .type = desc.type,
      .format = desc.format,
      .id = 0
   };

   glGenTextures(1, &texture.id);

   void* data = image.data;
   for (i32 mip=0; mip<desc.mipmaps; mip++)
   {
      RNDR_SetTextureData(texture, CBE_LEFT, mip, 1, data);
      uS pixel_count = (uS)RNDR_MipScaled(desc.size.width * desc.size.height * desc.depth, mip);
      data += pixel_count * RNDR_BytesPerPixel(desc.format);
   }

   if (texture.id == 0)
   {
      fprintf(stderr, "WARNING [Texture]: Texture Creation Failed!");
      texture.err.general = ERR_WARN;
   }

   Renderer_TextureSampler(texture, (TextureSampler){ FTR_LINEAR_NO_MIPMAP, FTR_LINEAR_NO_MIPMAP, WRP_REPEAT, WRP_REPEAT });

   return texture;
}

void Renderer_FreeTexture(Texture* texture)
{
   if (texture->id > 0)
      glDeleteTextures(1, &texture->id);
   texture->id = 0;
}

void Renderer_TextureSampler(Texture texture, TextureSampler sampler)
{
   u32 attachment = GL_TEXTURE_2D; // RNDR_TextureType(texture.type);
   glBindTexture(attachment, texture.id);

   glTexParameteri(attachment, GL_TEXTURE_MIN_FILTER, RNDR_FilterType(sampler.filtering.minify));
   glTexParameteri(attachment, GL_TEXTURE_MAG_FILTER, RNDR_FilterType(sampler.filtering.magnify));
   glTexParameteri(attachment, GL_TEXTURE_WRAP_S, RNDR_WrapType(sampler.wrapping.s));
   glTexParameteri(attachment, GL_TEXTURE_WRAP_T, RNDR_WrapType(sampler.wrapping.t));

   glBindTexture(attachment, 0);
}

Target Renderer_CreateTarget(TargetDesc target_desc)
{
   Target target = {
      .samples = target_desc.samples,
      .texture_count = target_desc.texture_count,
      .textures = { 0 },
      .id = 0
   };

   glGenFramebuffers(1, &target.id);

   for (i32 i=0; i<target.texture_count; i++)
   {
      TargetTextureDesc desc = target_desc.textures[i];

      Texture texture = {
         .size = desc.size,
         .depth = desc.depth,
         .mipmaps = desc.mipmaps,
         .type = desc.type,
         .format = desc.format,
         .id = 0
      };

      glGenTextures(1, &texture.id);

      for (i32 mip=0; mip<desc.mipmaps; mip++)
      {
         switch (desc.type)
         {
            case TEX_3D: {
               RNDR_SetTextureData(texture, CBE_LEFT, mip, 1, NULL);
               for (u32 layer=0; layer<desc.depth; layer++)
                  RNDR_AttachFramebufferTexture(target.id, desc.target_type, texture, mip, layer, CBE_LEFT);
               break;
            }

            case TEX_CUBEMAP: {
               for (CubemapFace face=0; face<6; face++)
               {
                  RNDR_SetTextureData(texture, face, mip, 1, NULL);
                  RNDR_AttachFramebufferTexture(target.id, desc.target_type, texture, mip, 1, face);
               }
               break;
            }

            default: {
               RNDR_SetTextureData(texture, CBE_LEFT, mip, target.samples, NULL);
               RNDR_AttachFramebufferTexture(target.id, desc.target_type, texture, mip, 1, CBE_LEFT);
               break;
            }
         }
      }

      target.textures[i] = texture;
   }

   glBindFramebuffer(GL_FRAMEBUFFER, target.id);
   if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
   {
      fprintf(stderr, "WARNING [TARGET]: Target Creation Failed!");
      target.err.general = ERR_WARN;
   }
   glBindFramebuffer(GL_FRAMEBUFFER, 0);

   return target;
}

void Renderer_FreeTarget(Target* target)
{
   for (i32 i=0; i<target->texture_count; i++)
      Renderer_FreeTexture(&target->textures[i]);
   if (target->id > 0)
      glDeleteFramebuffers(1, &target->id);
   target->id = 0;
}

void Renderer_BeginTarget(Target target)
{
   glBindFramebuffer(GL_FRAMEBUFFER, target.id);
}

void Renderer_FinishTarget(void)
{
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Shader Renderer_CreateShader(const ShaderDesc shader_desc)
{
   Shader shader = { 0 };

   u32 shader_id = glCreateProgram();

   for (i32 i=0; i<SHD_MAX_STAGES; i++)
   {
      if (shader_desc.stage_src[i] == NULL)
         continue;

      u32 stage[SHD_MAX_STAGES] = {
         GL_COMPUTE_SHADER,
         GL_VERTEX_SHADER,
         GL_GEOMETRY_SHADER,
         GL_FRAGMENT_SHADER
      };

      u32 stage_id = glCreateShader(stage[i]);
      glShaderSource(stage_id, 1, &shader_desc.stage_src[i], NULL);
      glCompileShader(stage_id);

      i32 stage_success = 0;
      glGetShaderiv(stage_id, GL_COMPILE_STATUS, &stage_success);

      if (!stage_success)
      {
         const char* stage_name[SHD_MAX_STAGES] = {
            "Compute",
            "Vertex",
            "Geometry",
            "Fragment"
         };

         char stage_log[512];
         glGetShaderInfoLog(stage_id, 512, NULL, stage_log);
         fprintf(stderr, "WARNING [SHADER]: %s Shader Stage Compilation Failed! Info:\n%s\n", stage_name[i], stage_log);
         shader.err.general = ERR_WARN;
         shader.err.flags |= (1u<<((u32)i + 1u));

         if (stage_id > 0)
            glDeleteShader(stage_id);

         continue;
      }

      shader.stage_id[i] = stage_id;
      glAttachShader(shader_id, stage_id);
   }

   glLinkProgram(shader_id);

   i32 shader_success = 0;
   glGetProgramiv(shader_id, GL_LINK_STATUS, &shader_success);

   shader.id = shader_id;

   if (!shader_success)
   {
      char shader_log[512];
      glGetProgramInfoLog(shader_id, 512, NULL, shader_log);
      fprintf(stderr, "WARNING [SHADER]: Shader Compilation Failed! Info:\n%s\n", shader_log);
      shader.err.general = ERR_ERROR;
      shader.err.extra = ERR_SHADER_COMPILE_FAILED;
   }

   return shader;
}

void Renderer_FreeShader(Shader* shader)
{
   for (i32 i=0; i<SHD_MAX_STAGES; i++)
   {
      if (shader->stage_id[i] == 0)
         continue;

      glDeleteShader(shader->stage_id[i]);
      shader->stage_id[i] = 0;
   }

   if (shader->id > 0)
      glDeleteProgram(shader->id);
   shader->id = 0;
}

Geometry Renderer_CreateGeometry(const Buffer vertices, const Buffer indices, const VertexDesc vertex_desc)
{
   if ((vertex_desc.count <= 0) || (vertex_desc.count > 8))
      abort();

   Geometry geometry = {
      .vertices.count = vertices.count,
      .indices.count = indices.count,
      .index_type = vertex_desc.index_type,
      .vertex_size = vertices.size,
      .vertices.id = 0,
      .indices.id = 0,
      .id = 0
   };

   u32 vao = 0;
   glGenVertexArrays(1, &vao);
   glBindVertexArray(vao);

   u32 vbo, ebo = 0;
   glGenBuffers(1, &vbo);
   glGenBuffers(1, &ebo);

   glBindBuffer(GL_ARRAY_BUFFER, vbo);
   glBufferData(GL_ARRAY_BUFFER, (uS)vertices.count * (uS)vertices.size, vertices.data, GL_STATIC_DRAW);

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, (uS)indices.count * (uS)indices.size, indices.data, GL_STATIC_DRAW);

   for (u32 i=0; i<vertex_desc.count; i++)
   {
      u8 normalized = RNDR_AttributeNormalized(vertex_desc.attribute[i].format);
      i32 size = RNDR_AttributeSize(vertex_desc.attribute[i].format);
      u32 type = RNDR_AttributeType(vertex_desc.attribute[i].format);

      void* offset = (void*)((uS)vertex_desc.attribute[i].offset);
      glVertexAttribPointer(i,
         size, type,
         normalized,
         (uS)vertices.size,
         offset
      );
      glEnableVertexAttribArray(i);
   }

   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
   glBindVertexArray(0);

   geometry.vertices.id = vbo;
   geometry.indices.id = ebo;
   geometry.id = vao;

   return geometry;
}

void Renderer_FreeGeometry(Geometry* geometry)
{
   glBindVertexArray(geometry->id);
   if (geometry->id > 0)
      glDeleteVertexArrays(1, &geometry->id);
   geometry->id = 0;
   geometry->vertices.id = 0;
   geometry->indices.id = 0;
}

// void Renderer_DrawGeometry(Geometry geometry)
// {
//    glBindVertexArray(geometry.id);
//    if (geometry.index_type != 0)
//    {
//       const u32 type[2] = { GL_UNSIGNED_SHORT, GL_UNSIGNED_INT };
//       glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.indices.id);
//       glDrawElements(GL_TRIANGLES, geometry.indices.count, type[geometry.index_type - 1u], 0);
//    } else {
//       glBindBuffer(GL_ARRAY_BUFFER, geometry.vertices.id);
//       glDrawArrays(GL_TRIANGLES, 0, geometry.vertices.count);
//    }
// }

void Renderer_Clear(vec4 color, ClearTargets clear_targets)
{
   glClearColor(color.r, color.g, color.b, color.a);

   u32 mask =
      (clear_targets.color ? GL_COLOR_BUFFER_BIT : 0u) |
      (clear_targets.depth ? GL_DEPTH_BUFFER_BIT : 0u) |
      (clear_targets.stencil ? GL_STENCIL_BUFFER_BIT : 0u);

   glClear(mask);
}

void Renderer_SetViewport(size2i viewport_size, i32 offset_x, i32 offset_y)
{
   glViewport(offset_x, offset_y, viewport_size.width, viewport_size.height);
}

u32 RNDR_CubemapFace(CubemapFace face)
{
   switch (face)
   {
      case CBE_LEFT:
         return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
      case CBE_RIGHT:
         return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
      case CBE_BOTTOM:
         return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
      case CBE_TOP:
         return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
      case CBE_FRONT:
         return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
      case CBE_BACK:
         return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
   }
}

void RNDR_SetTexture(u32 slot, Texture texture)
{
   glActiveTexture(GL_TEXTURE0 + slot);
   glBindTexture(RNDR_TextureType(texture.type), texture.id);
}

void RNDR_SetTextureData(Texture texture, CubemapFace face, i32 mip, i32 samples, void* data)
{
   u32 attatchment = RNDR_TextureType(texture.type);

   glBindTexture(attatchment, texture.id);

   switch (texture.type)
   {
      case TEX_1D: {
         glTexImage1D(
            GL_TEXTURE_1D,
            mip,
            RNDR_PixelInternalFormat(texture.format),
            RNDR_MipScaled(texture.size.width, mip),
            0,
            RNDR_PixelFormat(texture.format),
            RNDR_PixelType(texture.format),
            data
         );
         break;
      }

      case TEX_2D: {
         glTexImage2D(
            GL_TEXTURE_2D,
            mip,
            RNDR_PixelInternalFormat(texture.format),
            RNDR_MipScaled(texture.size.width, mip),
            RNDR_MipScaled(texture.size.height, mip),
            0,
            RNDR_PixelFormat(texture.format),
            RNDR_PixelType(texture.format),
            data
         );
         break;
      }

      case TEX_2D_MSAA: {
         glTexImage2DMultisample(
            GL_TEXTURE_2D_MULTISAMPLE,
            samples,
            RNDR_PixelInternalFormat(texture.format),
            texture.size.width,
            texture.size.height,
            GL_FALSE
         );
         break;
      }

      case TEX_3D: {
         glTexImage3D(
            GL_TEXTURE_3D,
            mip,
            RNDR_PixelInternalFormat(texture.format),
            RNDR_MipScaled(texture.size.width, mip),
            RNDR_MipScaled(texture.size.height, mip),
            RNDR_MipScaled(texture.depth, mip),
            0,
            RNDR_PixelFormat(texture.format),
            RNDR_PixelType(texture.format),
            data
         );
         break;
      }

      case TEX_CUBEMAP: {
         glTexImage2D(
            RNDR_CubemapFace(face),
            mip,
            RNDR_PixelInternalFormat(texture.format),
            RNDR_MipScaled(texture.size.width, mip),
            RNDR_MipScaled(texture.size.height, mip),
            0,
            RNDR_PixelFormat(texture.format),
            RNDR_PixelType(texture.format),
            data
         );
         break;
      }
   }

   glBindTexture(attatchment, 0);
}

void RNDR_SetUniform(u32 location, u32 count, UniformType type, void* data)
{
   switch (type)
   {
      case UNI_F32:
         glUniform1fv(location, count, data);
         break;
      case UNI_F32_V2:
         glUniform2fv(location, count, data);
         break;
      case UNI_F32_V3:
         glUniform3fv(location, count, data);
         break;
      case UNI_F32_V4:
         glUniform4fv(location, count, data);
         break;

      case UNI_I32:
      case UNI_SAMPLER:
         glUniform1iv(location, count, data);
         break;
      case UNI_I32_V2:
         glUniform2iv(location, count, data);
         break;
      case UNI_I32_V3:
         glUniform3iv(location, count, data);
         break;
      case UNI_I32_V4:
         glUniform4iv(location, count, data);
         break;

      case UNI_U32:
         glUniform1uiv(location, count, data);
         break;
      case UNI_U32_V2:
         glUniform2uiv(location, count, data);
         break;
      case UNI_U32_V3:
         glUniform3uiv(location, count, data);
         break;
      case UNI_U32_V4:
         glUniform4uiv(location, count, data);
         break;

      case UNI_MATRIX:
         glUniformMatrix4fv(location, count, GL_FALSE, data);
         break;

      default:
         break;
   }
}

void RNDR_DrawGeometry(Geometry geometry)
{
   glBindVertexArray(geometry.id);
   if (geometry.index_type != 0)
   {
      const u32 type[2] = { GL_UNSIGNED_SHORT, GL_UNSIGNED_INT };
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.indices.id);
      glDrawElements(GL_TRIANGLES, geometry.indices.count, type[geometry.index_type - 1u], 0);
   } else {
      glBindBuffer(GL_ARRAY_BUFFER, geometry.vertices.id);
      glDrawArrays(GL_TRIANGLES, 0, geometry.vertices.count);
   }
}

void RNDR_AttachFramebufferTexture(u32 fbo, TargetType target_type, Texture texture, i32 mip, i32 layer, CubemapFace face)
{
   glBindFramebuffer(GL_FRAMEBUFFER, fbo);

   switch (texture.type)
   {
      case TEX_1D:
         glFramebufferTexture1D(GL_FRAMEBUFFER, RNDR_TargetType(target_type), GL_TEXTURE_1D, texture.id, mip);
         break;
      case TEX_2D:
         glFramebufferTexture2D(GL_FRAMEBUFFER, RNDR_TargetType(target_type), GL_TEXTURE_2D, texture.id, mip);
         break;
      case TEX_2D_MSAA:
         glFramebufferTexture2D(GL_FRAMEBUFFER, RNDR_TargetType(target_type), GL_TEXTURE_2D_MULTISAMPLE, texture.id, 0);
         break;
      case TEX_3D:
         glFramebufferTexture3D(GL_FRAMEBUFFER, RNDR_TargetType(target_type), GL_TEXTURE_3D, texture.id, mip, layer);
         break;
      case TEX_CUBEMAP:
         glFramebufferTexture2D(GL_FRAMEBUFFER, RNDR_TargetType(target_type), RNDR_TargetTypeCubemap(face), texture.id, mip);
         break;
   }

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RNDR_MarkFramebufferDirty(Renderer* renderer)
{
   renderer->screen_out.is_dirty = true;
}

void RNDR_InitFramebuffer(Renderer* renderer, size2i size)
{
   if (renderer->screen_out.target.id != 0)
      Renderer_FreeTarget(&renderer->screen_out.target);

   PixelFormat color = renderer->screen_out.color_format;
   PixelFormat depth = renderer->screen_out.depth_format;
   TargetDesc desc = {
      .samples = 1,
      .texture_count = 2,
      .textures = {
         [0] = { size, 1, 1, TEX_2D, color, TRG_TEX_COLOR0 },
         [1] = { size, 1, 1, TEX_2D, depth, TRG_TEX_DEPTH },
      }
   };
   Target target = Renderer_CreateTarget(desc);
   renderer->screen_out.target = target;
}

void RNDR_BeginFrame(Renderer* renderer, vec4 clear_color, ClearTargets clear_targets)
{
   View view = renderer->view_array[renderer->active_view];
   size2i view_size = Renderer_ScaleFrameSize(renderer, view.frame_size);

   if (renderer->screen_out.is_dirty)
   {
      RNDR_InitFramebuffer(renderer, view_size);
      renderer->screen_out.is_dirty = false;
   }

   Renderer_BeginTarget(renderer->screen_out.target);

   Renderer_SetViewport(view_size, 0, 0);
   glEnable(GL_DEPTH_TEST);
   Renderer_Clear(clear_color, clear_targets);
}

void RNDR_RenderFrame(Renderer* renderer)
{
   for (i32 i=0; i<Util_ArrayLength(renderer->draw_call_array); i++)
   {
      RNDR_DrawCall drawcall = renderer->draw_call_array[i];

      glUseProgram(drawcall.shader_id);
      for (i32 j=0; j<drawcall.uniform_count; j++)
      {
         UniformData uniform = drawcall.uniforms[j];
         RNDR_SetUniform(uniform.location, 1, uniform.type, uniform.datablob.data);
      }

      for (i32 j=0; j<drawcall.texture_count; j++)
      {
         RNDR_SetTexture(j, drawcall.textures[j]);
      }

      RNDR_DrawGeometry(drawcall.geometry);
   }

   SET_ARRAY_LENGTH(renderer->draw_call_array, 0);
}

void RNDR_FinishFrame(Renderer* renderer, size2i render_size)
{
   Renderer_FinishTarget();

   Renderer_SetViewport(render_size, 0, 0);
   glDisable(GL_DEPTH_TEST);
   Renderer_Clear((vec4){ 1, 1, 1, 1 }, (ClearTargets){ .color = true });

   Texture screentex = renderer->screen_out.target.textures[0];

   glUseProgram(renderer->pfx_passthru.id);
   RNDR_SetUniform(glGetUniformLocation(renderer->pfx_passthru.id, "u_texture"), 1, UNI_SAMPLER, (i32[]){ 0 });
   Renderer_TextureSampler(screentex, (TextureSampler){ FTR_LINEAR_NO_MIPMAP, FTR_LINEAR_NO_MIPMAP, WRP_CLAMP, WRP_CLAMP });
   RNDR_SetTexture(0, screentex);

   RNDR_DrawGeometry(renderer->pfx_quad);
}

u32 RNDR_MipScaled(u32 size, i32 mip)
{
   u32 scaled = (u32)((f64)size * pow(0.5, (f64)mip));
   return scaled;
}

u32 RNDR_FilterType(FilterType filter)
{
   switch (filter)
   {
      case FTR_NEAREST_NO_MIPMAP:
         return GL_NEAREST;
      case FTR_NEAREST_MIP_NEAREST:
         return GL_NEAREST_MIPMAP_NEAREST;
      case FTR_NEAREST_MIP_LINEAR:
         return GL_NEAREST_MIPMAP_LINEAR;
      case FTR_LINEAR_NO_MIPMAP:
         return GL_LINEAR;
      case FTR_LINEAR_MIP_NEAREST:
         return GL_LINEAR_MIPMAP_NEAREST;
      case FTR_LINEAR_MIP_LINEAR:
         return GL_LINEAR_MIPMAP_LINEAR;
   }
}

u32 RNDR_WrapType(WrapType wrap)
{
   switch (wrap)
   {
      case WRP_REPEAT:
         return GL_REPEAT;
      case WRP_CLAMP:
         return GL_CLAMP_TO_EDGE;
   }
}

u32 RNDR_TextureType(TextureType texture)
{
   switch (texture)
   {
      case TEX_1D:
         return GL_TEXTURE_1D;
      case TEX_2D:
         return GL_TEXTURE_2D;
      case TEX_2D_MSAA:
         return GL_TEXTURE_2D_MULTISAMPLE;
      case TEX_3D:
         return GL_TEXTURE_3D;
      case TEX_CUBEMAP:
         return GL_TEXTURE_CUBE_MAP;
   }
}

u8 RNDR_AttributeNormalized(AttributeFormat format)
{
   switch (format)
   {
      case ATR_F32:
      case ATR_F32_V2:
      case ATR_F32_V3:
      case ATR_F32_V4:
      case ATR_I16:
      case ATR_I16_V2:
      case ATR_I16_V3:
      case ATR_I16_V4:
      case ATR_U16:
      case ATR_U16_V2:
      case ATR_U16_V3:
      case ATR_U16_V4:
      case ATR_I32:
      case ATR_I32_V2:
      case ATR_I32_V3:
      case ATR_I32_V4:
      case ATR_U32:
      case ATR_U32_V2:
      case ATR_U32_V3:
      case ATR_U32_V4:
         return GL_FALSE;
      default:
         return GL_TRUE;
   }
}

u32 RNDR_AttributeType(AttributeFormat format)
{
   switch (format)
   {
      case ATR_F32:
      case ATR_F32_V2:
      case ATR_F32_V3:
      case ATR_F32_V4:
         return GL_FLOAT;
      case ATR_I16:
      case ATR_I16_V2:
      case ATR_I16_V3:
      case ATR_I16_V4:
      case ATR_IN16:
      case ATR_IN16_V2:
      case ATR_IN16_V3:
      case ATR_IN16_V4:
         return GL_SHORT;
      case ATR_U16:
      case ATR_U16_V2:
      case ATR_U16_V3:
      case ATR_U16_V4:
      case ATR_UN16:
      case ATR_UN16_V2:
      case ATR_UN16_V3:
      case ATR_UN16_V4:
         return GL_UNSIGNED_SHORT;
      case ATR_I32:
      case ATR_I32_V2:
      case ATR_I32_V3:
      case ATR_I32_V4:
      case ATR_IN32:
      case ATR_IN32_V2:
      case ATR_IN32_V3:
      case ATR_IN32_V4:
         return GL_INT;
      case ATR_U32:
      case ATR_U32_V2:
      case ATR_U32_V3:
      case ATR_U32_V4:
      case ATR_UN32:
      case ATR_UN32_V2:
      case ATR_UN32_V3:
      case ATR_UN32_V4:
         return GL_UNSIGNED_INT;
   }
}

i32 RNDR_AttributeSize(AttributeFormat format)
{
   switch (format)
   {
      case ATR_F32:
      case ATR_I16:
      case ATR_U16:
      case ATR_I32:
      case ATR_U32:
      case ATR_IN16:
      case ATR_UN16:
      case ATR_IN32:
      case ATR_UN32:
         return 1;
      case ATR_F32_V2:
      case ATR_I16_V2:
      case ATR_U16_V2:
      case ATR_I32_V2:
      case ATR_U32_V2:
      case ATR_IN16_V2:
      case ATR_UN16_V2:
      case ATR_IN32_V2:
      case ATR_UN32_V2:
         return 2;
      case ATR_F32_V3:
      case ATR_I16_V3:
      case ATR_U16_V3:
      case ATR_I32_V3:
      case ATR_U32_V3:
      case ATR_IN16_V3:
      case ATR_UN16_V3:
      case ATR_IN32_V3:
      case ATR_UN32_V3:
         return 3;
      case ATR_F32_V4:
      case ATR_I16_V4:
      case ATR_U16_V4:
      case ATR_I32_V4:
      case ATR_U32_V4:
      case ATR_IN16_V4:
      case ATR_UN16_V4:
      case ATR_IN32_V4:
      case ATR_UN32_V4:
         return 4;
   }
}

u32 RNDR_PixelInternalFormat(PixelFormat format)
{
   switch (format)
   {
      case PIX_R_8BPC:
         return GL_R8;
      case PIX_RG_8BPC:
         return GL_RG8;
      case PIX_RGB_8BPC:
         return GL_RGB8;
      case PIX_RGBA_8BPC:
         return GL_RGBA8;
      case PIX_R_F16BPC:
         return GL_R16F;
      case PIX_RG_F16BPC:
         return GL_RG16F;
      case PIX_RGB_F16BPC:
         return GL_RGB16F;
      case PIX_RGBA_F16BPC:
         return GL_RGBA16F;
      case PIX_R_F32BPC:
         return GL_R32F;
      case PIX_RG_F32BPC:
         return GL_RG32F;
      case PIX_RGB_F32BPC:
         return GL_RGB32F;
      case PIX_RGBA_F32BPC:
         return GL_RGBA32F;
      case PIX_DEPTH_24BPC:
         return GL_DEPTH_COMPONENT24;
      case PIX_DEPTH_F32BPC:
         return GL_DEPTH_COMPONENT32F;
      case PIX_DEPTH_STENCIL:
         return GL_DEPTH24_STENCIL8;
   }
}

u32 RNDR_PixelFormat(PixelFormat format)
{
   switch (format)
   {
      case PIX_R_8BPC:
      case PIX_R_F16BPC:
      case PIX_R_F32BPC:
         return GL_RED;
      case PIX_RG_8BPC:
      case PIX_RG_F16BPC:
      case PIX_RG_F32BPC:
         return GL_RG;
      case PIX_RGB_8BPC:
      case PIX_RGB_F16BPC:
      case PIX_RGB_F32BPC:
         return GL_RGB;
      case PIX_RGBA_8BPC:
      case PIX_RGBA_F16BPC:
      case PIX_RGBA_F32BPC:
         return GL_RGBA;
      case PIX_DEPTH_24BPC:
      case PIX_DEPTH_F32BPC:
         return GL_DEPTH_COMPONENT;
      case PIX_DEPTH_STENCIL:
         return GL_DEPTH_STENCIL;
   }
}

u32 RNDR_PixelType(PixelFormat format)
{
   switch (format)
   {
      case PIX_R_8BPC:
      case PIX_RG_8BPC:
      case PIX_RGB_8BPC:
      case PIX_RGBA_8BPC:
         return GL_UNSIGNED_BYTE;
      case PIX_R_F16BPC:
      case PIX_RG_F16BPC:
      case PIX_RGB_F16BPC:
      case PIX_RGBA_F16BPC:
         return GL_HALF_FLOAT;
      case PIX_R_F32BPC:
      case PIX_RG_F32BPC:
      case PIX_RGB_F32BPC:
      case PIX_RGBA_F32BPC:
      case PIX_DEPTH_F32BPC:
         return GL_FLOAT;
      case PIX_DEPTH_24BPC:
         return GL_UNSIGNED_INT;
      case PIX_DEPTH_STENCIL:
         return GL_UNSIGNED_INT_24_8;
   }
}

u32 RNDR_TargetType(TargetType target_type)
{
   switch (target_type)
   {
      case TRG_TEX_COLOR0:
         return GL_COLOR_ATTACHMENT0;
      case TRG_TEX_COLOR1:
         return GL_COLOR_ATTACHMENT1;
      case TRG_TEX_COLOR2:
         return GL_COLOR_ATTACHMENT2;
      case TRG_TEX_COLOR3:
         return GL_COLOR_ATTACHMENT3;
      case TRG_TEX_DEPTH:
         return GL_DEPTH_ATTACHMENT;
   }
}

u32 RNDR_TargetTypeCubemap(CubemapFace face)
{
   switch (face)
   {
      case CBE_LEFT:
         return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
      case CBE_RIGHT:
         return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
      case CBE_BOTTOM:
         return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
      case CBE_TOP:
         return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
      case CBE_FRONT:
         return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
      case CBE_BACK:
         return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
   }
}

uS RNDR_BytesPerPixel(PixelFormat format)
{
   uS bpp = 0;

   switch (RNDR_PixelType(format))
   {
      case GL_UNSIGNED_BYTE:
         bpp = 1;
         break;

      case GL_HALF_FLOAT:
         bpp = 2;
         break;

      case GL_FLOAT:
      case GL_UNSIGNED_INT:
      case GL_UNSIGNED_INT_24_8:
         bpp = 4;
         break;

      default:
         break;
   }

   switch (RNDR_PixelFormat(format))
   {
      case GL_RG:
         bpp *= 2;
         break;

      case GL_RGB:
         bpp *= 3;
         break;

      case GL_RGBA:
         bpp *= 4;
         break;

      default:
         break;
   }

   return bpp;
}
