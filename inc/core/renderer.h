#ifndef ECT_RENDERER_H
#define ECT_RENDERER_H

#include "ect_types.h"

#include <sokol_gfx.h>

typedef struct EctView_t
{
   mat4x4 view_proj;
   frame frame_size;
} EctView;

typedef struct EctBuffer_t
{
   const void* data;
   u32 size;
   u32 count;
} EctBuffer;

typedef struct EctGeometry_t
{
   sg_bindings bind;
   u32 vertex_count;
   u32 vertex_size;
   u32 index_count;
   u32 index_size;
} EctGeometry;

typedef struct EctRenderer_t EctRenderer;

EctRenderer* EctRendererInit(void);
void EctRendererFree(EctRenderer* renderer);

frame EctRendererGetSize(EctRenderer* renderer);
sg_swapchain EctRendererGetSwapchain(EctRenderer* renderer);

EctGeometry EctRendererCreateGeometry(const EctBuffer vertices, const EctBuffer indices);
void EctRendererDrawGeometry(EctGeometry geometry);

#endif
