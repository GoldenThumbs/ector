#ifndef ECT_RENDERER_H
#define ECT_RENDERER_H

#include "util/types.h"
#include "util/math.h"
#include "graphics.h"

#define LIGHTS_PER_CLUSTER 127

typedef struct Cluster_t
{
   vec4 bounds_min;
   vec4 bounds_max;
   u32 count;
   u32 indices[LIGHTS_PER_CLUSTER];
} Cluster;

#define LIGHT_DIR   0
#define LIGHT_POINT 1
#define LIGHT_SPOT  2

typedef struct Light_t
{
   vec3 origin;
   f32 radius;
   quat rotation;
   u32 light_type;
   color8 color;
   f32 intensity;
   f32 softness;
   f32 cos_half_angle;
   f32 importance_bias;
   f32 importance_scale;
   f32 fade_weight;
} Light;

static inline f32 Renderer_SpotAngle(f32 angle)
{
   return M_COS(angle * 0.5f) * 0.5f + 0.5f;
}

Shader Renderer_LitShader(GraphicsContext *context);
Shader Renderer_ClusterShader(GraphicsContext *context);
Shader Renderer_CullShader(GraphicsContext *context);

#define RNDR_CAMERA_GLSL \
"layout(std140, binding=1) uniform CameraUBO\n" \
"{\n" \
"   mat4 mat_view;\n" \
"   mat4 mat_proj;\n" \
"   mat4 mat_invview;\n" \
"   mat4 mat_invproj;\n" \
"   vec2 u_near_far;\n" \
"   uvec2 u_screen_size;\n" \
"};\n"

#define RNDR_CLUSTER_GLSL \
"#define LIGHTS_PER_CLUSTER "ECT_STRINGIFY(LIGHTS_PER_CLUSTER)"\n" \
"struct Cluster\n" \
"{\n" \
"   vec4 bounds_min;\n" \
"   vec4 bounds_max;\n" \
"   uint count;\n" \
"   uint indices[LIGHTS_PER_CLUSTER];\n" \
"};\n" \
"layout(std430, binding=2) restrict buffer ClusterSSBO\n" \
"{\n" \
"   Cluster clusters[];\n" \
"};\n"

#define RNDR_LIGHT_GLSL \
"struct Light\n" \
"{\n" \
"   vec4 origin;\n" \
"   vec4 rotation;\n" \
"   uint light_type;\n" \
"   uint color;\n" \
"   float intensity;\n" \
"   float softness;\n" \
"   float cos_half_angle;\n" \
"   float importance_bias;\n" \
"   float importance_scale;\n" \
"   float fade_weight;\n" \
"};\n" \
"#define LIGHT_DIR "ECT_STRINGIFY(LIGHT_DIR)"\n" \
"#define LIGHT_POINT "ECT_STRINGIFY(LIGHT_POINT)"\n" \
"#define LIGHT_SPOT "ECT_STRINGIFY(LIGHT_SPOT)"\n" \
"layout(std430, binding=3) restrict buffer LightSSBO\n" \
"{\n" \
"   Light lights[];\n" \
"};\n"

#endif
