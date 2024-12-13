#ifndef ECT_RENDERER_H
#define ECT_RENDERER_H

#include "ect_types.h"

#include <stdbool.h>

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

typedef enum EctGFXType_t
{
   ECT_GFX_F32 = 0,
   ECT_GFX_F32_V2,
   ECT_GFX_F32_V3,
   ECT_GFX_F32_V4,
   ECT_GFX_F32_M2,
   ECT_GFX_F32_M3,
   ECT_GFX_F32_M4,

   ECT_GFX_I8,
   ECT_GFX_I16,
   ECT_GFX_I32,

   ECT_GFX_U8,
   ECT_GFX_U16,
   ECT_GFX_U32
} EctGFXType;

typedef enum EctVertexType_t
{
   ECT_VRT_F32 = 0,
   ECT_VRT_F32_V2,
   ECT_VRT_F32_V3,
   ECT_VRT_F32_V4,

   ECT_VRT_I16,
   ECT_VRT_I16_V2,
   ECT_VRT_I16_V3,
   ECT_VRT_I16_V4,

   ECT_VRT_IN16,
   ECT_VRT_IN16_V2,
   ECT_VRT_IN16_V3,
   ECT_VRT_IN16_V4,

   ECT_VRT_U16,
   ECT_VRT_U16_V2,
   ECT_VRT_U16_V3,
   ECT_VRT_U16_V4,

   ECT_VRT_UN16,
   ECT_VRT_UN16_V2,
   ECT_VRT_UN16_V3,
   ECT_VRT_UN16_V4,

   ECT_VRT_I32,
   ECT_VRT_I32_V2,
   ECT_VRT_I32_V3,
   ECT_VRT_I32_V4,

   ECT_VRT_IN32,
   ECT_VRT_IN32_V2,
   ECT_VRT_IN32_V3,
   ECT_VRT_IN32_V4,

   ECT_VRT_U32,
   ECT_VRT_U32_V2,
   ECT_VRT_U32_V3,
   ECT_VRT_U32_V4,

   ECT_VRT_UN32,
   ECT_VRT_UN32_V2,
   ECT_VRT_UN32_V3,
   ECT_VRT_UN32_V4
} EctVertexType;

typedef enum EctIndexType_t
{
   ECT_IDX_NONE = 0,
   ECT_IDX_U16,
   ECT_IDX_U32
} EctIndexType;

typedef struct EctVertexDesc_t
{
   u32 count;
   EctIndexType index_type;
   struct {
      u32 offset;
      EctVertexType type;
   } attribute[8];
} EctVertexDesc;

typedef struct EctGeometry_t
{
   struct {
      u32 id;
      u32 count;
      u32 size;
   } vertices;
   struct {
      u32 id;
      u32 count;
      EctIndexType type;
   } indices;
   u32 id;
} EctGeometry;

typedef struct EctRenderer_t EctRenderer;

EctRenderer* EctRendererInit(void);
void EctRendererFree(EctRenderer* renderer);

frame EctRendererGetSize(EctRenderer* renderer);

EctGeometry EctRendererCreateGeometry(const EctBuffer vertices, const EctBuffer indices, const EctVertexDesc vertex_desc);
void EctRendererFreeGeometry(EctGeometry* geometry);
void EctRendererDrawGeometry(EctGeometry geometry);

#endif
