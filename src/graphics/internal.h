#ifndef GFX_INTERNAL
#define GFX_INTERNAL

#include "util/types.h"

#include "graphics.h"

typedef struct gfx_Shader_t
{
   struct {
      u32 program;
   } id;

   struct {
      u16 is_compute: 1;
   };
   
   u16 next_freed;
   handle compare;

} gfx_Shader;

typedef struct gfx_Buffer_t
{
   struct {
      u32 buf;
   } id;

   struct {
      u32 type: 8;
      u32 draw_mode: 8;
   };
   
   handle compare;
   u16 next_freed;

} gfx_Buffer;

typedef struct gfx_Geometry_t
{
   struct {
      u32 vao;
      u32 v_buf, i_buf;

   } id;

   u32 element_count;

   u8 draw_mode: 4;
   u8 face_cull_mode: 4;
   u8 primitive: 7;
   u8 index_type: 1;
   
   
   u16 next_freed;
   handle compare;

} gfx_Geometry;

typedef struct gfx_Texture_t
{
   struct {
      u32 tex;
   } id;

   i32 width;
   i32 height;
   i32 depth;
   u16 mipmap_count;
   u8 type;
   u8 format;
   
   handle compare;
   u16 next_freed;

} gfx_Texture;

typedef struct gfx_Framebuffer_t
{
   struct {
      u32 fbo;
      u32 rbo;
   } id;

   handle compare;
   u16 next_freed;

} gfx_Framebuffer;

typedef union gfx_State_t
{
   u16 state_id;

   struct {
      u16 blend_enable: 1;
      u16 face_cull_enable: 1;
      u16 depthtest_enable: 1;
      u16 depthmask_enable: 1;
      u16 stenciltest_enable: 1;
      u16 face_cull_mode: 1;
      u16 blend_mode: 3;
      u16 depth_mode: 3;
      u16 enable_color_clear: 1;
      u16 enable_depth_clear: 1;
      u16 enable_stencil_clear: 1;

   };

} gfx_State;

typedef struct gfx_Filtering_t
{
   u32 min_filter;
   u32 mag_filter;

} gfx_Filtering;

struct Graphics_t
{
   gfx_Shader* shaders;
   gfx_Buffer* buffers;
   gfx_Geometry* geometries;
   gfx_Texture* textures;
   gfx_Framebuffer* framebuffers;

   u16 freed_shader_root;
   u16 freed_buffer_root;
   u16 freed_geometry_root;
   u16 freed_texture_root;
   u16 freed_framebuffer_root;

   gfx_State state;
   color8 clear_color;
   f32 clear_depth;
   i32 clear_stencil_id;

};

static inline bool GFX_IsDepthFormat(u8 texture_format)
{
   switch (texture_format)
   {
      default:
         break;
      
      case GFX_TEXTUREFORMAT_DEPTH_16:
      case GFX_TEXTUREFORMAT_DEPTH_24:
      case GFX_TEXTUREFORMAT_DEPTH_F32:
      case GFX_TEXTUREFORMAT_DEPTH_24_STENCIL_8:
      case GFX_TEXTUREFORMAT_DEPTH_F32_STENCIL_8:
         return true;

   }

   return false;
}

static inline bool GFX_IsStencilFormat(u8 texture_format)
{
   switch (texture_format)
   {
      default:
         break;
      
      case GFX_TEXTUREFORMAT_DEPTH_24_STENCIL_8:
      case GFX_TEXTUREFORMAT_DEPTH_F32_STENCIL_8:
         return true;

   }

   return false;
}

void GFX_CheckOpenGLError(void);

void GFX_BindUniformBlocks(Graphics* graphics, UniformBlockList uniform_blocks);

u8 GFX_MeshPrimitive(u8 mesh_primitive);
u8 GFX_MeshAttribute(u8 mesh_attribute);

u32 GFX_AttributeType(u8 attribute);
i32 GFX_AttributeTypeCount(u8 attribute);
bool GFX_AttributeTypeNormalized(u8 attribute);
uS GFX_AttributeTypeSize(u8 attribute);
uS GFX_VertexBufferSize(u32 vertex_count, u8* attributes, u16 attribute_count);

void GFX_CreateGeometry(gfx_Geometry* geometry, Mesh mesh);

uS GFX_PixelSize(u8 format);
i32 GFX_TextureInternalFormat(u8 format);
u32 GFX_TexturePixelFormat(u8 format);
u32 GFX_TextureFormatType(u8 format);
u32 GFX_TextureType(u8 type);
u32 GFX_TextureWrap(u8 wrap);
gfx_Filtering GFX_TextureFilter(u8 filter);

void GFX_CreateTexture(gfx_Texture* texture, u8* data, bool is_update);

void GFX_SetFaceCullMode(Graphics* graphics, u8 face_cull_mode);
void GFX_DrawVertices(u8 primitive, u32 element_count, bool use_index_buffer, u8 index_type, u32 gl_vertex_array, i32 offset, u32 instance_count);

u32 GFX_Primitive(u8 primitive_type);
u32 GFX_DrawMode(u8 draw_mode);
u32 GFX_BufferType(u8 buffer_type);

#endif
