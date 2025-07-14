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
   u8 face_cull_mode: 4;
   u8 primitive;
   u16 element_count;

   handle compare;
} gfx_Geometry;

typedef struct gfx_Texture_t
{
   struct {
      u32 tex;
   } id;

   u16 width;
   u16 height;
   u16 mipmap_count;
   u8 type;
   u8 format;

   handle compare;
} gfx_Texture;

typedef struct gfx_Framebuffer_t
{
   struct {
      u32 fbo;
      u32 rbo;
   } id;

   handle compare;
} gfx_Framebuffer;

typedef union gfx_State_t
{
   u16 state_id;
   struct {
      u16 blend_enable: 1;
      u16 face_cull_enable: 1;
      u16 depthtest_enable: 1;
      u16 stenciltest_enable: 1;
      u16 face_cull_mode: 1;
      u16 blend_mode: 3;
   };
} gfx_State;

struct gfx_Filtering_s
{
   u32 min_filter;
   u32 mag_filter;
};

struct Graphics_t
{
   gfx_Shader* shaders;
   gfx_Buffer* buffers;
   gfx_Geometry* geometries;
   gfx_Texture* textures;
   gfx_Framebuffer* framebuffers;
   struct {
      color8 clear_color;
      u16 ref;
      gfx_State state;
   };
};

void GFX_UseUniformBlocks(Graphics* graphics, UniformBlockList uniform_blocks);

u8 GFX_MeshPrimitive(u8 mesh_primitive);
u8 GFX_MeshAttribute(u8 mesh_attribute);

u32 GFX_AttributeType(u8 attribute);
i32 GFX_AttributeTypeCount(u8 attribute);
bool GFX_AttributeTypeNormalized(u8 attribute);
uS GFX_AttributeTypeSize(u8 attribute);
uS GFX_VertexBufferSize(u16 vertex_count, u8* attributes, u16 attribute_count);

uS GFX_PixelSize(u8 format);
i32 GFX_TextureInternalFormat(u8 format);
u32 GFX_TexturePixelFormat(u8 format);
u32 GFX_TextureFormatType(u8 format);
u32 GFX_TextureType(u8 type);
u32 GFX_TextureWrap(u8 wrap);
struct gfx_Filtering_s GFX_TextureFilter(u8 filter);

void GFX_SetFaceCullMode(Graphics* graphics, u8 face_cull_mode);
void GFX_DrawVertices(u8 primitive, u16 element_count, bool use_index_buffer, u32 gl_vertex_array, i32 offset);

u32 GFX_Primitive(u8 primitive_type);
u32 GFX_DrawMode(u8 draw_mode);
u32 GFX_BufferType(u8 buffer_type);
void GFX_SetUniform(Uniform uniform);

#endif
