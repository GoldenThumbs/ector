#ifndef GFX_INTERNAL
#define GFX_INTERNAL

#include "util/types.h"

#include "graphics.h"

typedef struct gfx_Shader_t
{
   struct {
      u32 program;
   } id;

   bool is_compute;

   handle compare;
} gfx_Shader;

typedef struct gfx_Buffer_t
{
   struct {
      u32 buf;
   } id;

   u8 type;
   u8 draw_mode;

   handle compare;
} gfx_Buffer;

typedef struct gfx_Geometry_t
{
   struct {
      u32 vao;
      u32 v_buf, i_buf;
   } id;

   u8 draw_mode: 4;
   u8 face_cull: 4;
   u8 primitive;
   u16 element_count;

   handle compare;
} gfx_Geometry;

struct GraphicsContext_t
{
   gfx_Shader* shaders;
   gfx_Buffer* buffers;
   gfx_Geometry* geometries;
   struct {
      color8 clear_color;
      u16 ref;
      struct {
         u16 color: 1;
         u16 depth: 1;
         u16 stencil: 1;
      } clear_buffers;
   };
};

u32 GFX_AttributeType(u8 attribute);
i32 GFX_AttributeTypeCount(u8 attribute);
bool GFX_AttributeTypeNormalized(u8 attribute);
uS GFX_AttributeTypeSize(u8 attribute);
uS GFX_VertexBufferSize(u16 vertex_count, u8* attributes, u16 attribute_count);

u32 GFX_Primitive(u8 primitive_type);
u32 GFX_DrawMode(u8 draw_mode);
u32 GFX_BufferType(u8 buffer_type);
void GFX_SetUniform(Uniform uniform);

#endif
