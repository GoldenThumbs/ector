#ifndef LIGHTS_PER_CLUSTER
#define LIGHTS_PER_CLUSTER 200
#endif

const float M_EPSILON = 1e-4;
const float M_PI = 3.141592;
const float M_TAU = 6.283185;
const float M_INVPI = 1.0 / M_PI;

layout(local_size_x=128, local_size_y=1, local_size_z=1) in;

struct Cluster
{
   vec4 center;
   vec4 extents;
   uvec4 light_count;
   uint indices[LIGHTS_PER_CLUSTER];
   
};

struct PackedLight
{
   vec4 origin_radius;
   uint rgbe_color;
   uint cosang_softness;
   uint angles;
   uint shad_next;

};

struct LightData
{
   vec3 origin;
   float radius;
   vec3 color;
   float cos_half_angle;
   float spot_softness;
   float theta;
   float phi;
   int shadow_id;
   int next_light;

};

layout(std140, binding=1) uniform CameraUBO
{
   mat4 mat_view;
   mat4 mat_proj;
   mat4 mat_invview;
   mat4 mat_invproj;
   vec2 u_near_far;
   uvec2 u_screen_size;
   vec4 u_proj_info;

};

layout(std430, binding=1) restrict buffer ClusterSSBO
{
   uvec4 u_cluster_dimensions;
   Cluster clusters[];

};

layout(std430, binding=2) restrict buffer LightSSBO
{
   ivec4 u_light_list;
   PackedLight lights[];

};

vec3 DecodeColor(uint rgbe_color)
{
   if (rgbe_color == 0) return vec3(0.0);

   uvec4 v = uvec4(
      rgbe_color & 255u,
      (rgbe_color >>  8) & 255u,
      (rgbe_color >> 16) & 255u,
      rgbe_color >> 24
   );

   float f = pow(2.0, float(v.w) - 136.0);

   return vec3(v.xyz) * f;
}

ivec2 DecodeHalfInts(uint packed_floats)
{
   ivec2 v = (ivec2((int(packed_floats) << 16) >> 16, int(packed_floats) >> 16));

   return v;
}

LightData DecodeLightData(PackedLight packed_light)
{
   LightData light;

   vec2 cosang_softness = unpackUnorm2x16(packed_light.cosang_softness);
   vec2 angles = unpackUnorm2x16(packed_light.angles);
   ivec2 shad_next = DecodeHalfInts(packed_light.shad_next);

   light.origin = (mat_view * vec4(packed_light.origin_radius.xyz, 1.0)).xyz;
   light.radius = packed_light.origin_radius.w;
   light.color = DecodeColor(packed_light.rgbe_color);

   light.cos_half_angle = cosang_softness[0];
   light.spot_softness = cosang_softness[1];
   light.theta = angles[0] * M_TAU;
   light.phi = angles[1] * M_TAU;
   light.shadow_id = shad_next[0];
   light.next_light = shad_next[1];

   return light;
}

bool SphereTest(vec3 s_origin, float s_radius, vec3 center, vec3 extents)
{
   vec3 diff = max(vec3(0.0), abs(center - s_origin) - extents);
   return (dot(diff, diff) <= s_radius * s_radius);
}

bool LightTest(int light_idx, Cluster cluster, out int next_light_idx)
{
   PackedLight packed_light = lights[light_idx];
   LightData light = DecodeLightData(packed_light);

   vec3 s_origin = light.origin;
   float s_radius = light.radius;

   vec3 center = cluster.center.xyz;
   vec3 extents = cluster.extents.xyz;

   bool test = SphereTest(s_origin, s_radius, center, extents);
   next_light_idx = light_idx + light.next_light;

   return test;
}

void main()
{
   uint tile_id = gl_WorkGroupID.x * 128 + gl_LocalInvocationID.x;
   Cluster cluster = clusters[tile_id];
   cluster.light_count.w = 0;

   int light_idx = u_light_list.x;

   while (light_idx > -1)
   {  
      int next_light_idx = 0;
      if (LightTest(light_idx, cluster, next_light_idx))
         cluster.indices[cluster.light_count.w++] = uint(light_idx);
      
      if (cluster.light_count.w >= LIGHTS_PER_CLUSTER || next_light_idx == light_idx)
         break;
      
      light_idx = next_light_idx;
      
   }

   clusters[tile_id] = cluster;
}
