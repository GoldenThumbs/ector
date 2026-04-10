
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

layout(std140, binding=2) uniform ModelUBO
{
   mat4 mat_model;
   mat4 mat_invmodel;
   mat4 mat_mvp;
   mat4 mat_normal_model_u_color;

};

#ifdef VERT

layout(location=0) in vec3 vrt_position;

#ifdef USE_LIGHTING
layout(location=1) in vec3 vrt_normal;
layout(location=3) in vec4 vrt_tangent;
#endif // USE_LIGHTING

layout(location=2) in vec2 vrt_texcoord;
out vec2 v2f_texcoord;

#ifdef USE_LIGHTING
out vec3 v2f_normal;
out vec3 v2f_tangent;
out vec3 v2f_bitangent;
out vec3 v2f_position;
#endif // USE_LIGHTING

#ifdef SHADOW_CASTER
out float v2f_depth;
#endif

void main()
{
   v2f_texcoord = vrt_texcoord;
   gl_Position = mat_mvp * vec4(vrt_position, 1.0);

#if defined(USE_LIGHTING) || defined(SHADOW_CASTER)
   vec3 vs_position = (mat_view * mat_model * vec4(vrt_position, 1.0)).xyz;
#endif

#ifdef USE_LIGHTING
   vec3 vs_normal = mat3(mat_view) * mat3(mat_normal_model_u_color) * vrt_normal;
   vec3 vs_tangent = mat3(mat_view) * mat3(mat_normal_model_u_color) * vrt_tangent.xyz;
   v2f_normal = normalize(vs_normal);
   v2f_tangent = normalize(vs_tangent);
   v2f_bitangent = normalize(cross(v2f_normal, v2f_tangent)) * vrt_tangent.w;
   v2f_position = vs_position;
#endif // USE_LIGHTING

#ifdef SHADOW_CASTER
   vs_position *= 1.0 / u_near_far.y;
   v2f_depth = dot(vs_position, vs_position);
#endif

}

#endif

#ifdef FRAG

layout(binding=0) uniform sampler2D tex_color;

in vec2 v2f_texcoord;

#ifdef USE_LIGHTING

#ifndef LIGHTS_PER_CLUSTER
#define LIGHTS_PER_CLUSTER 200
#endif

const float M_EPSILON = 1e-4;
const float M_PI = 3.141592;
const float M_TAU = 6.283185;
const float M_INVPI = 1.0 / M_PI;

layout(binding=1) uniform sampler2D tex_normal;
layout(binding=2) uniform sampler2D tex_roughness;
layout(binding=3) uniform sampler2D tex_metallic;

layout(binding=5) uniform samplerCubeArray tex_pointlight_shadows;

in vec3 v2f_normal;
in vec3 v2f_tangent;
in vec3 v2f_bitangent;
in vec3 v2f_position;

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
   bool pointlight_shadow;

};

struct SurfaceData
{
   vec3 surf_color;
   vec3 surf_normal;
   float surf_roughness;
   float surf_metallic;

   vec3 position_vs;
   vec3 view;
   float nDv;

   float roughness_sqr;
   float inv_metallic;
   vec3 albedo;
   vec3 f0;

   vec2 screen_coord;

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

float InterleavedGradientNoise(vec2 coords)
{
   const vec3 ign_magic = vec3(0.06711056, 0.00583715, 52.9829189);
   return fract(ign_magic.z * fract(dot(ign_magic.xy, coords)));
}

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
   ivec2 v = ivec2(packed_floats, packed_floats >> 16) & int(0xFFFFu);

   return ivec2(v);
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
   light.shadow_id = abs(shad_next[0]) - 1;
   light.next_light = shad_next[1];

   return light;
}

SurfaceData FillSurfaceData(vec2 screen_coord, vec3 surf_color, vec3 surf_normal, float surf_roughness, float surf_metallic, vec3 position_vs)
{
   SurfaceData surf_data;

   surf_data.surf_color = surf_color;
   surf_data.surf_normal = surf_normal;
   surf_data.surf_roughness = surf_roughness;
   surf_data.surf_metallic = surf_metallic;

   surf_data.position_vs = position_vs;
   surf_data.view = normalize(-position_vs);
   surf_data.nDv = min(abs(dot(surf_normal, surf_data.view)) + M_EPSILON, 1.0);

   surf_data.roughness_sqr = surf_roughness * surf_roughness;
   surf_data.inv_metallic = 1.0 - surf_metallic;
   surf_data.albedo = surf_color * surf_data.inv_metallic;
   surf_data.f0 = surf_color * surf_metallic + surf_data.inv_metallic * 0.04;

   surf_data.screen_coord = screen_coord;

   return surf_data;
}

float PointAttenution(vec3 light_position)
{
   return pow(clamp(1.0 - dot(light_position, light_position), 0.0, 1.0), 4.0);
}

float SpotAttenuation(vec3 light_dir, LightData light)
{
   float cos_half_angle = light.cos_half_angle;
   vec4 cs_polar = vec4(cos(light.theta), sin(light.theta), cos(light.phi), sin(light.phi));

   vec3 spot_dir = vec3(cs_polar.y * cs_polar.z, -cs_polar.w, cs_polar.x * cs_polar.z);
   spot_dir = mat3(mat_view) * spot_dir;

   float spot_dist = dot(light_dir, spot_dir) * 0.5 + 0.5;
   float softness = (1.0 - cos_half_angle) * light.spot_softness;
   
   return smoothstep(cos_half_angle, cos_half_angle + softness, spot_dist);
}

vec3 NormalMapped(sampler2D normalmap_texture, vec2 coords, vec3 t, vec3 b, vec3 n)
{
   mat3 tbn = mat3(t, b, normalize(n));
   vec3 normalmap = texture(normalmap_texture, coords).xyz * 2.0 - 1.0;

   return normalize(tbn * normalmap);
}

vec3 Fresnel(vec3 f0, float x)
{
   return f0 + (1.0 - f0) * pow(1.0 - x, 5.0);
}

float Visibility(float nDl, float nDv, float r)
{
   return max(0.5 / mix(2.0 * nDl * nDv, nDl + nDv, r), M_EPSILON);
}

float Distribution(float nDh, float r)
{
   float inv_sqrmag = 1.0 - nDh * nDh;
   float a = r * nDh;
   float k = r / (inv_sqrmag + a * a);
   return max(k * k * M_INVPI, M_EPSILON);
}

float SampleShadow(float depth, vec3 shadowmap_coords, int shadow_id)
{
   vec4 sm = vec4(shadowmap_coords, float(shadow_id));
   vec4 shadowmap_depth = textureGather(tex_pointlight_shadows, sm, 0);
   vec4 comp = step(depth, shadowmap_depth);

   return (comp.r + comp.g + comp.b + comp.a) * 0.25;
}

vec3 LightContribution(SurfaceData surf_data, PackedLight packed_light)
{
   LightData light = DecodeLightData(packed_light);

   vec3 light_position = (light.origin - surf_data.position_vs) / light.radius;
   vec3 light_dir = normalize(light_position);
   vec3 halfway = normalize(light_dir + surf_data.view);

   float nDl = max(0.0, dot(surf_data.surf_normal, light_dir));
   float nDh = max(0.0, dot(surf_data.surf_normal, halfway));
   float lDh = clamp(dot(light_dir, halfway), 0.0, 1.0);

   float attenuation = PointAttenution(light_position);
   attenuation *= SpotAttenuation(light_dir, light);

   float shadow_spread = 0.2 / float(textureSize(tex_pointlight_shadows, 0).x);
   float noise_spread = 1.5 * shadow_spread;
   if (light.shadow_id > -1)
   {
      vec3 surf_to_light = -(mat3(mat_invview) * light_position);

      const float g_3d = 1.32471795724474602596;
      const vec3 p_3d = 1.0 / (vec3(g_3d, g_3d * g_3d, g_3d * g_3d * g_3d));

      float ign = InterleavedGradientNoise(surf_data.screen_coord);
      vec3 ign_p = 0.5 + ign * 15.0 * p_3d;
      
      float bias = mix(0.16, 0.1, nDl * nDl);
      float surf_depth = dot(surf_to_light, surf_to_light) - bias * bias;

      const vec3 shadow_offsets[8] = vec3[](
         vec3( -1.0, -1.0, -1.0 ),
         vec3( -1.0,  1.0, -1.0 ),
         vec3(  1.0,  1.0, -1.0 ),
         vec3(  1.0, -1.0, -1.0 ),
         vec3( -1.0, -1.0,  1.0 ),
         vec3( -1.0,  1.0,  1.0 ),
         vec3(  1.0,  1.0,  1.0 ),
         vec3(  1.0, -1.0,  1.0 )
      );

      float shadow = 0.0;
      for (int s = 0; s < 8; s++)
      {
         vec3 shadow_noise = (fract(ign_p + float(s) * p_3d) - 0.5) * noise_spread;
         vec3 shadowmap_coords = surf_to_light + shadow_noise + shadow_offsets[s] * shadow_spread;

         shadow += SampleShadow(surf_depth, shadowmap_coords, light.shadow_id);
      }

      attenuation *= shadow * 0.125;

   }

   vec3 f = Fresnel(surf_data.f0, lDh);
   float d = Distribution(nDh, surf_data.roughness_sqr);
   float v = Visibility(nDl, surf_data.nDv, surf_data.roughness_sqr);

   vec3 diffuse = light.color * attenuation * nDl;
   vec3 specular = f * d * v;

   return (surf_data.albedo + specular) * diffuse;
}
#endif // USE_LIGHTING

#ifdef SHADOW_CASTER
in float v2f_depth;
#endif

out vec4 frg_color;
void main()
{
   vec4 color = texture(tex_color, v2f_texcoord) * mat_normal_model_u_color[3];

#ifdef USE_LIGHTING
   SurfaceData surf_data = FillSurfaceData(
      gl_FragCoord.xy,
      color.rgb,
      NormalMapped(tex_normal, v2f_texcoord, v2f_tangent, v2f_bitangent, v2f_normal),
      texture(tex_roughness, v2f_texcoord).r,
      texture(tex_metallic, v2f_texcoord).r,
      v2f_position
   );

   vec3 final_color = surf_data.surf_color * 0.0001;

   vec2 tile_size = vec2(u_screen_size) / vec2(u_cluster_dimensions.xy); 
   uint z_id = uint((log(abs(surf_data.position_vs.z) / u_near_far.x) * u_cluster_dimensions.z) / log(u_near_far.y / u_near_far.x));   
   uvec3 tile = uvec3(gl_FragCoord.xy / tile_size, z_id);
   uint tile_id = (tile.y * u_cluster_dimensions.x) + (tile.z * u_cluster_dimensions.x * u_cluster_dimensions.y) + tile.x;

   for (uint light_i = 0; light_i < clusters[tile_id].light_count.w; light_i++)
   {
      PackedLight packed_light = lights[clusters[tile_id].indices[light_i]];
      final_color += LightContribution(surf_data, packed_light);

   }

// #define CLUSTER_DEBUG_VIZ

#ifdef CLUSTER_DEBUG_VIZ

   const vec3 cool = vec3(0.0, 0.0, 0.5);
   const vec3 warm = vec3(1.0, 1.0, 0.0);
   const vec3 hot = vec3(1.0, 0.0, 0.0);

   float light_fac = (float(clusters[tile_id].light_count.w) / float(100)) * 2.0;
   float a = max(light_fac - 1, 0);
   float b = min(light_fac, 1);

   // final_color = clamp(final_color, 0.0, 1.0) * 0.25;
   final_color = mix(cool, mix(warm, hot, a), b);

#endif

#else
   vec3 final_color = color.rgb;
#endif // USE_LIGHTING

   frg_color.rgb = pow(final_color * color.a, vec3(1.0 / 2.2));
   frg_color.a = color.a;

#ifdef SHADOW_CASTER
   gl_FragDepth = v2f_depth;
#endif
   
}

#endif
