#ifndef ECT_GRAPHICS_H
#define ECT_GRAPHICS_H

#include "util/types.h"
#include "mesh.h"

typedef handle Shader;
typedef handle Buffer;
typedef handle Geometry;

enum {
   ERR_GFX_CONTEXT_FAILED = 1
};

enum {
   GFX_DRAWMODE_STATIC = 0,
   GFX_DRAWMODE_DYNAMIC,
   GFX_DRAWMODE_STREAM,
   GFX_DRAWMODE_STATIC_READ,
   GFX_DRAWMODE_STATIC_COPY,
   GFX_DRAWMODE_DYNAMIC_READ,
   GFX_DRAWMODE_DYNAMIC_COPY,
   GFX_DRAWMODE_STREAM_READ,
   GFX_DRAWMODE_STREAM_COPY
};

enum {
   GFX_BUFFERTYPE_UNIFORM = 0,
   GFX_BUFFERTYPE_STORAGE
};

enum {
   GFX_FACECULL_BACK = 0,
   GFX_FACECULL_FRONT,
   GFX_FACECULL_NONE
};

enum {
   GFX_PRIMITIVE_POINT = 0,
   GFX_PRIMITIVE_LINE,
   GFX_PRIMITIVE_TRIANGLE
};

enum {
   GFX_ATTRIBUTE_NULL = 0,

   GFX_ATTRIBUTE_F32_1X,
   GFX_ATTRIBUTE_F32_2X,
   GFX_ATTRIBUTE_F32_3X,
   GFX_ATTRIBUTE_F32_4X,

   GFX_ATTRIBUTE_U8_4X_NORM,
};

enum {
   GFX_UNIFORMTYPE_F32_1X = 0,
   GFX_UNIFORMTYPE_F32_2X,
   GFX_UNIFORMTYPE_F32_3X,
   GFX_UNIFORMTYPE_F32_4X,
   GFX_UNIFORMTYPE_MAT3,
   GFX_UNIFORMTYPE_MAT4,
   GFX_UNIFORMTYPE_U32_1X,
   GFX_UNIFORMTYPE_U32_2X,
   GFX_UNIFORMTYPE_U32_3X,
   GFX_UNIFORMTYPE_U32_4X,
   GFX_UNIFORMTYPE_TEX_SLOT
};

typedef struct Uniform_t
{
   u16 uniform_type;
   u16 location;
   union {
      f32 as_float[16];
      vec2 as_vec2;
      vec3 as_vec3;
      vec4 as_vec4;
      mat3x3 as_mat3;
      mat4x4 as_mat4;

      u32 as_uint[4];
      i32 texslot;
   };
} Uniform;

typedef struct UniformBlock_t
{
   u32 binding;
   Buffer ubo;
} UniformBlock;

#define GFX_MAX_UNIFORM_BLOCKS 8

typedef struct Uniforms_t
{
   UniformBlock blocks[GFX_MAX_UNIFORM_BLOCKS];
   u32 count;
} Uniforms;

typedef struct GraphicsContext_t GraphicsContext;

Shader Graphics_CreateShader(GraphicsContext* context, const char* vertex_shader, const char* fragment_shader);
Shader Graphics_CreateComputeShader(GraphicsContext* context, const char* compute_shader);
void Graphics_FreeShader(GraphicsContext* context, Shader res_shader);
u32 Graphics_GetUniformLocation(GraphicsContext* context, Shader res_shader, const char* name);
void Graphics_Dispatch(GraphicsContext* context, Shader res_shader, u32 size_x, u32 size_y, u32 size_z, Uniforms uniforms);
void Graphics_DispatchBarrier(void);

Buffer Graphics_CreateBuffer(GraphicsContext* context, void* data, u32 length, uS type_size, u8 draw_mode, u8 buffer_type);
Buffer Graphics_CreateBufferExplicit(GraphicsContext* context, void* data, uS total_size, u8 draw_mode, u8 buffer_type);
void Graphics_ReuseBuffer(GraphicsContext* context, void* data, u32 length, uS type_size, Buffer res_buffer);
void Graphics_FreeBuffer(GraphicsContext* context, Buffer res_buffer);
void Graphics_UpdateBuffer(GraphicsContext* context, Buffer res_buffer, void* data, u32 length, uS type_size);
void Graphics_UpdateBufferRange(GraphicsContext* context, Buffer res_buffer, void* data, u32 offset, u32 length, uS type_size);
void Graphics_UseBuffer(GraphicsContext* context, Buffer res_buffer, u32 slot);

Geometry Graphics_CreateGeometry(GraphicsContext* context, Mesh mesh, u8 draw_mode);
void Graphics_FreeGeometry(GraphicsContext* context, Geometry res_geometry);

void Graphics_SetClearColor(GraphicsContext* context, color8 clear_color);
void Graphics_Viewport(GraphicsContext* context, size2i size);
void Graphics_Draw(GraphicsContext* context, Shader res_shader, Geometry res_geometry, Uniforms uniforms);

#endif
