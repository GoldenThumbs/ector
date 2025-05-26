#include "util/types.h"
#include "util/resource.h"

#include "graphics.h"
#include "graphics/internal.h"

#include <glad/gl.h>

Geometry Graphics_CreateGeometry(Graphics* graphics, Mesh mesh, u8 draw_mode)
{
   gfx_Geometry geometry = { 0 };
   geometry.draw_mode = draw_mode;
   geometry.face_cull_mode = GFX_FACECULL_BACK;
   geometry.primitive = mesh.primitive;
   geometry.element_count = ((mesh.index_count > 0) && (mesh.primitive == GFX_PRIMITIVE_TRIANGLE)) ? mesh.index_count : mesh.vertex_count;
   geometry.compare.ref =  graphics->ref;

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

   return Util_AddResource(&graphics->ref, REF(graphics->geometries), &geometry);
}

void Graphics_FreeGeometry(Graphics* graphics, Geometry res_geometry)
{
   gfx_Geometry geometry = graphics->geometries[res_geometry.handle];
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

void GFX_SetFaceCullMode(Graphics* graphics, u8 face_cull_mode)
{
   bool face_cull_enable = (face_cull_mode == GFX_FACECULL_NONE);

   if ((bool)graphics->state.face_cull_enable != face_cull_enable)
      graphics->state.face_cull_enable = (u16)face_cull_enable;

   if ((graphics->state.face_cull_mode != face_cull_mode) && !face_cull_enable)
      graphics->state.face_cull_mode = (u16)face_cull_mode;
}

void GFX_DrawVertices(u8 primitive, u16 element_count, bool use_index_buffer, u32 gl_vertex_array, i32 offset)
{
   glBindVertexArray(gl_vertex_array);

   u32 gl_primitive = GFX_Primitive(primitive);

   if (use_index_buffer)
      glDrawElements(gl_primitive, element_count, GL_UNSIGNED_SHORT, (void*)((u64)offset));
   else
      glDrawArrays(gl_primitive, offset, element_count);

   glBindVertexArray(0);
}