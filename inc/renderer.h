#ifndef ECT_RENDERER_H
#define ECT_RENDERER_H

#include "util/types.h"
#include "util/extra_types.h"
// #include "util/math.h"
#include "graphics.h"

#define RENDERER_MODULE "renderer"

typedef handle Object;
typedef handle Light;

enum {
   RNDR_LIGHT_DIR = 1,
   RNDR_LIGHT_POINT,
   RNDR_LIGHT_SPOT,
   RNDR_NULL_LIGHT = 0
};

typedef struct ObjectDesc_t
{
   Shader shader;
   Geometry geometry;
   BBox bounds;
   UniformBlockList uniforms;
} ObjectDesc;

typedef struct LightDesc_t
{
   u32 light_type;
   vec3 origin;
   f32 radius;
   vec3 color;
   f32 strength;
   quat rotation;
   f32 cone_angle;
   f32 softness_fac;
   struct {
      f32 bias;
      f32 scale;
   } importance;
} LightDesc;

#define LIGHTS_PER_CLUSTER 127

typedef struct Renderer_t Renderer;

Renderer* Renderer_Init(Graphics* graphics);
void Renderer_Free(Renderer* renderer);

void Renderer_SetView(Renderer* renderer, mat4x4 view);
void Renderer_RenderLit(Renderer* renderer, resolution2d size);

Object Renderer_AddObject(Renderer* renderer, ObjectDesc* desc, Transform3D transform);
void Renderer_RemoveObject(Renderer* renderer, Object object);

Light Renderer_AddLight(Renderer* renderer, LightDesc* desc);
void Renderer_RemoveLight(Renderer* renderer, Light res_light);

Shader Renderer_LitShader(Graphics* graphics);

#define RNDR_CAMERA_GLSL \
"layout(std140, binding=1) uniform CameraUBO\n" \
"{\n" \
"   mat4 mat_view;\n" \
"   mat4 mat_proj;\n" \
"   mat4 mat_invview;\n" \
"   mat4 mat_invproj;\n" \
"   vec2 u_near_far;\n" \
"   uvec2 u_screen_size;\n" \
"   uvec4 u_cluster_count;\n" \
"};\n"

#define RNDR_MODEL_GLSL \
"layout(std140, binding=2) uniform ModelUBO\n" \
"{\n" \
"   mat4 mat_model;\n" \
"   mat4 mat_invmodel;\n" \
"   mat3x4 mat_normal_model;\n" \
"};\n"

#define RNDR_CLUSTER_GLSL \
"#define LIGHTS_PER_CLUSTER "ECT_STRINGIFY(LIGHTS_PER_CLUSTER)"\n" \
"struct Cluster\n" \
"{\n" \
"   vec4 center;\n" \
"   vec4 extents;\n" \
"   uint count;\n" \
"   uint indices[LIGHTS_PER_CLUSTER];\n" \
"};\n" \
"layout(std430, binding=1) restrict buffer ClusterSSBO\n" \
"{\n" \
"   Cluster clusters[];\n" \
"};\n"

#define RNDR_LIGHT_GLSL \
"struct Light\n" \
"{\n" \
"   vec4 origin;\n" \
"   vec4 rotation;\n" \
"   uint color;\n" \
"   uint params;\n" \
"   vec2 importance;\n" \
"};\n" \
"#define LIGHT_DIR 1\n" \
"#define LIGHT_POINT 2\n" \
"#define LIGHT_SPOT 3\n" \
"#define NULL_LIGHT 0\n" \
"layout(std430, binding=2) restrict buffer LightSSBO\n" \
"{\n" \
"   Light lights[];\n" \
"};\n" \
"void SetFadeWeight(uint index, float fade_weight)\n" \
"{\n" \
"   const uint byte_mask = 255u;\n" \
"   const uint byte_ofs = 16u;\n" \
"   const uint param_mask = (byte_mask << byte_ofs);\n" \
"   uint i_w = uint(fade_weight * 255.0);\n" \
"   lights[index].params &= ~param_mask;\n" \
"   lights[index].params |= (i_w << byte_ofs) & param_mask;\n" \
"}\n" \
"vec4 DecodeParameters(uint params)\n" \
"{\n" \
"   const uint byte_mask = (1u << 8) - 1;\n" \
"   const float rcp_maxbyte = 1.0 / float(byte_mask);\n" \
"   return vec4(\n" \
"      rcp_maxbyte * ((params >>  0) & byte_mask),\n" \
"      rcp_maxbyte * ((params >>  8) & byte_mask),\n" \
"      rcp_maxbyte * ((params >> 16) & byte_mask),\n" \
"      (params >> 24) & byte_mask\n" \
"   );\n" \
"}\n" \
"vec3 DecodeColor(uint rgbe_color)\n" \
"{\n" \
"   const uint byte_mask = 255u;\n" \
"   if (rgbe_color == 0) return vec3(0.0);\n" \
"   uvec4 v = uvec4(\n" \
"      (rgbe_color >>  0) & byte_mask,\n" \
"      (rgbe_color >>  8) & byte_mask,\n" \
"      (rgbe_color >> 16) & byte_mask,\n" \
"      (rgbe_color >> 24) & byte_mask\n" \
"   );\n" \
"   float f = pow(2.0, float(v.w) - 136.0)\n;" \
"   return vec3(v.xyz) * f;\n" \
"}\n"

#endif
