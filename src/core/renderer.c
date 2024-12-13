#include "ect_math.h"
#include "ect_types.h"
// #include "ect_math.h"

#include "core/engine.h"
#include "core/renderer.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

struct EctRenderer_t
{
   struct {
      AR(EctView) available;
      EctView* active;
   } view;
   struct {
      u32 color_format;
      u32 depth_format;
      i32 msaa_samples;
      u32 _padding;
   } framebuffer;
   vec2 render_scale;
};

EctRenderer* EctRendererInit(void)
{
   EctRenderer* renderer = (EctRenderer*)calloc(1, sizeof(EctRenderer));
   renderer->view.available = NULL;
   renderer->view.active = NULL;
   renderer->framebuffer.color_format = GL_RGBA8;
   renderer->framebuffer.depth_format = GL_DEPTH24_STENCIL8;
   renderer->framebuffer.msaa_samples = 1;
   renderer->render_scale = VEC2(1, 1);

   assert(gladLoadGL((GLADloadfunc)glfwGetProcAddress));

   return renderer;
}

void EctRendererFree(EctRenderer* renderer)
{
   free(renderer);
}

frame EctRendererGetSize(EctRenderer* renderer)
{
   frame r_size = EctGetSize();
   vec2 r_sizef = VEC2(r_size.width, r_size.height);
   r_sizef = EctMulVec2(r_sizef, renderer->render_scale);

   return (frame){ (i32)r_sizef.x, (i32)r_sizef.y };
}

EctGeometry EctRendererCreateGeometry(const EctBuffer vertices, const EctBuffer indices, const EctVertexDesc vertex_desc)
{
   assert(vertex_desc.count > 0);
   assert(vertex_desc.count <= 8);
   u32 vao = 0;
   glGenVertexArrays(1, &vao);
   glBindVertexArray(vao);

   u32 vbuf, ibuf = 0;
   glGenBuffers(1, &vbuf);
   glGenBuffers(1, &ibuf);

   glBindBuffer(GL_ARRAY_BUFFER, vbuf);
   glBufferData(GL_ARRAY_BUFFER, (uS)vertices.count * (uS)vertices.size, vertices.data, GL_STATIC_DRAW);

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuf);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, (uS)indices.count * (uS)indices.size, indices.data, GL_STATIC_DRAW);

   for (u32 i=0; i<vertex_desc.count; i++)
   {
      u32 base_type = vertex_desc.attribute[i].type / 4u;

      i32 size = (i32)(vertex_desc.attribute[i].type - base_type * 4u) + 1;
      u32 type = 0;
      u8 normalized = GL_FALSE;

      switch(base_type)
      {
         case 0:
            type = GL_FLOAT;
            break;
         case 1:
            type = GL_SHORT;
            break;
         case 2:
            type = GL_SHORT;
            normalized = GL_TRUE;
            break;
         case 3:
            type = GL_UNSIGNED_SHORT;
            break;
         case 4:
            type = GL_UNSIGNED_SHORT;
            normalized = GL_TRUE;
            break;
         case 5:
            type = GL_INT;
            break;
         case 6:
            type = GL_INT;
            normalized = GL_TRUE;
            break;
         case 7:
            type = GL_UNSIGNED_INT;
            break;
         case 8:
            type = GL_UNSIGNED_INT;
            normalized = GL_TRUE;
            break;

         default:
            abort();
      }

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
   glBindVertexArray(0);

   return (EctGeometry){
      .vertices = {
         .id = vbuf,
         .count = vertices.count,
         .size = vertices.size
      },
      .indices = {
         .id = ibuf,
         .count = indices.count,
         .type = vertex_desc.index_type,
      },
      .id = vao
   };
}

void EctRendererFreeGeometry(EctGeometry* geometry)
{
   glDeleteVertexArrays(1, &geometry->id);
   glDeleteBuffers(1, &geometry->vertices.id);
   glDeleteBuffers(1, &geometry->indices.id);
}

void EctRendererDrawGeometry(EctGeometry geometry)
{
   glBindVertexArray(geometry.id);
   if (geometry.indices.type != 0)
   {
      const u32 type[2] = { GL_UNSIGNED_SHORT, GL_UNSIGNED_INT };
      glDrawElements(GL_TRIANGLES, geometry.indices.count, type[geometry.indices.type - 1u], 0);
   } else
      glDrawArrays(GL_TRIANGLES, 0, geometry.vertices.count);
   glBindVertexArray(0);
}
