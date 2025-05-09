#include "util/types.h"
#include "util/resource.h"

#include "graphics.h"
#include "graphics/internal.h"

#include <glad/gl.h>

Geometry Graphics_CreateGeometry(GraphicsContext* context, Mesh mesh, u8 draw_mode)
{
   gfx_Geometry geometry = { 0 };
   geometry.draw_mode = draw_mode;
   geometry.face_cull = mesh.face_culling;
   geometry.primitive = mesh.primitive;
   geometry.element_count = ((mesh.index_count > 0) && (mesh.primitive == GFX_PRIMITIVE_TRIANGLE)) ? mesh.index_count : mesh.vertex_count;
   geometry.compare.ref =  context->ref;

   glGenVertexArrays(1, &geometry.id.vao);
   glBindVertexArray(geometry.id.vao);

   glGenBuffers(1, &geometry.id.v_buf);
   glBindBuffer(GL_ARRAY_BUFFER, geometry.id.v_buf);
   glBufferData(
      GL_ARRAY_BUFFER,
      GFX_VertexBufferSize(mesh.vertex_count, mesh.attributes, mesh.attribute_count),
      mesh.vertex_buffer,
      GFX_DrawMode(draw_mode)
   );

   u16 atr_num = 0;
   uS atr_ofs = 0;
   for (; atr_num<mesh.attribute_count; atr_num++)
   {
      u8 a = mesh.attributes[atr_num];

      if (a == GFX_ATTRIBUTE_NULL)
         break;
      
      u32 a_type = GFX_AttributeType(a);
      i32 a_count = GFX_AttributeTypeCount(a);
      bool a_normalized = GFX_AttributeTypeNormalized(a);
      uS a_size = GFX_AttributeTypeSize(a);

      glEnableVertexAttribArray(atr_num);
      glVertexAttribPointer(
         atr_num,
         a_count,
         a_type,
         a_normalized,
         0,
         (void*)atr_ofs
      );

      atr_ofs += a_size * (uS)a_count * (uS)mesh.vertex_count;
   }

   if ((mesh.index_count > 0) && (mesh.primitive == GFX_PRIMITIVE_TRIANGLE))
   {
      glGenBuffers(1,  &geometry.id.i_buf);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.id.i_buf);
      glBufferData(
         GL_ELEMENT_ARRAY_BUFFER,
         sizeof(u16) * (uS)mesh.index_count,
         mesh.index_buffer,
         GFX_DrawMode(draw_mode)
      );
   }

   return Util_AddResource(&context->ref, REF(context->geometries), &geometry);
}

void Graphics_FreeGeometry(GraphicsContext* context, Geometry res_geometry)
{
   gfx_Geometry geometry = context->geometries[res_geometry.handle];
   if (geometry.compare.ref != res_geometry.ref)
      return;

   glDeleteVertexArrays(1, &geometry.id.vao);

   if (geometry.id.v_buf != 0)
      glDeleteBuffers(1, &geometry.id.v_buf);
   if (geometry.id.i_buf != 0)
      glDeleteBuffers(1, &geometry.id.i_buf);

   geometry.id.vao = 0;
   geometry.id.v_buf = 0;
   geometry.id.i_buf = 0;
}

void Graphics_UpdateGeometry(GraphicsContext* context, Geometry res_geometry, void* data, u32 length, uS type_size)
{
   gfx_Geometry geometry = context->geometries[res_geometry.handle];
   if (geometry.compare.ref != res_geometry.ref)
      return;
   
      glBindBuffer(GL_ARRAY_BUFFER, geometry.id.v_buf);
      glBufferSubData(GL_ARRAY_BUFFER, 0, (uS)length * type_size, data);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
}