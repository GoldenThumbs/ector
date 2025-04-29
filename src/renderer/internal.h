#ifndef RNDR_INTERNAL
#define RNDR_INTERNAL

#include "util/types.h"
#include "util/extra_types.h"
#include "graphics.h"

#include "renderer.h"

struct rndr_CameraData_s
{
   mat4x4 view;
   mat4x4 proj;
   mat4x4 inv_view;
   mat4x4 inv_proj;
   f32 near;
   f32 far;
   u32 width;
   u32 height;
   u32 cluster_size[4];
};

struct rndr_ModelData_s
{
   mat4x4 model;
   mat4x4 inv_model;
   vec4 normal[3];
};

typedef struct rndr_Cluster_t
{
   vec4 center;
   vec4 extents;
   u32 count;
   u32 indices[LIGHTS_PER_CLUSTER];
} rndr_Cluster;

typedef struct rndr_Object_t
{
   Shader shader;
   Geometry geometry;
   Uniforms uniforms;

   struct {
      mat4x4 model;
      mat4x4 inv_model;
      mat3x3 normal;
   } matrix;
   
   BBox bounds;
   BBox aabb;

   handle compare;
} rndr_Object;

// SSBO requires 16 byte alignment
// total size of struct is 48 bytes
typedef struct rndr_Light_t
{
   // 16 bytes for light origin and radius
   vec3 origin;
   f32 radius;

   // 16 bytes for light orientation
   quat rotation;
   
   // 16 bytes for this block of parameters
   // -- 4 bytes -> rgbe encoded color
   // -- 1 byte  -> cosine(cone angle / 2) * 0.5 + 0.5
   // -- 1 byte  -> softness factor
   // -- 1 byte  -> light fading factor
   // -- 1 byte  -> light type enum
   // -- 4 bytes -> biasing value for light importance calc.
   // -- 4 bytes -> scaling value for light importance calc.
   color8 color;
   u8 cos_half_angle;
   u8 softness;
   u8 fade_weight;
   u8 light_type;
   f32 importance_bias;
   f32 importance_scale;
   
} rndr_Light;

#define RNDR_INIT_LIGHT_COUNT 256

#define RNDR_CLUSTER_X 24
#define RNDR_CLUSTER_Y 16
#define RNDR_CLUSTER_Z 32
#define RNDR_CLUSTER_COUNT (RNDR_CLUSTER_X * RNDR_CLUSTER_Y * RNDR_CLUSTER_Z)

struct Renderer_t
{
   rndr_Object* objects;
   rndr_Light* lights;

   GraphicsContext* graphics_context;

   struct {
      mat4x4 view;
      mat4x4 proj;
      mat4x4 inv_view;
      mat4x4 inv_proj;
      f32 fov;
      f32 aspect_ratio;
      f32 near;
      f32 far;
   } camera;

   struct {
      Buffer camera_ubo;
      Buffer model_ubo;
      Buffer culling_ubo;
      Buffer cluster_ssbo;
      Buffer light_ssbo;

      Shader cluster_comp;
      Shader culling_comp;

   } gfx;

   u16 ref;
};



#endif
