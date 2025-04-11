#ifndef ECT_RENDERER_H
#define ECT_RENDERER_H

#include "util/types.h"

#define LIGHTS_PER_CLUSTER 127

typedef struct Cluster_t
{
   vec4 bounds_min;
   vec4 bounds_max;
   u32 count;
   u32 indices[LIGHTS_PER_CLUSTER];
} Cluster;

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

#define GFX_CLUSTER_GLSL \
"#define LIGHTS_PER_CLUSTER "ECT_STRINGIFY(LIGHTS_PER_CLUSTER)"\n" \
"struct Cluster\n" \
"{\n" \
"   vec4 bounds_min;\n" \
"   vec4 bounds_max;\n" \
"   uint count;\n" \
"   uint indices[LIGHTS_PER_CLUSTER];\n" \
"};\n" \
"layout(std430, binding=1) restrict buffer ClusterSSBO\n" \
"{\n" \
"   Cluster clusters[];\n" \
"};\n"

#define GFX_LIGHT_GLSL \
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
"layout(std430, binding=2) restrict buffer LightSSBO\n" \
"{\n" \
"   Light lights[];\n" \
"};\n"

#endif
