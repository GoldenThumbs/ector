#ifndef RNDR_SHADERS_INTERNAL
#define RNDR_SHADERS_INTERNAL

#include "renderer.h"

const char* rndr_CLUSTER_SHADER_GLSL =
"#version 430 core\n"

"layout(local_size_x=1, local_size_y=1, local_size_z=1) in;\n"

RNDR_CAMERA_GLSL
RNDR_CLUSTER_GLSL

"uniform uvec3 u_clusters;\n"

"vec3 ToViewSpace(vec2 point_ss)\n"
"{\n"
"   // conversion to normalized device coordinates\n"
"   vec4 ndc = vec4(point_ss / vec2(u_screen_size) * 2.0 - 1.0, -1.0, 1.0);\n"
"   vec4 point_vs = mat_invproj * ndc;\n"
"   point_vs /= point_vs.w;\n"
"   return point_vs.xyz;\n"
"}\n"

"vec3 LineIntersect(vec3 p_start, vec3 p_end, float distance)\n"
"{\n"
"   const vec3 normal = vec3( 0, 0,-1);\n"
"   vec3 dir = p_end - p_start;\n"
"   float t = (distance - dot(normal, p_start)) / dot(normal, dir);\n"
"   return p_start + t * dir;\n"
"}\n"

"void main()\n"
"{\n"
"   uint tile_id = (gl_WorkGroupID.y * u_clusters.x) + (gl_WorkGroupID.z * u_clusters.x * u_clusters.y) + gl_WorkGroupID.x;\n"
"   vec2 tile_size = (vec2(u_screen_size) / vec2(u_clusters.xy));\n"

"   vec3 tile_min = ToViewSpace(vec2(gl_WorkGroupID.xy) * tile_size);\n"
"   vec3 tile_max = ToViewSpace(vec2(gl_WorkGroupID.xy+1) * tile_size);\n"

"   float z_min = u_near_far.x * pow(u_near_far.y / u_near_far.x, float(gl_WorkGroupID.z) / float(u_clusters.z));\n"
"   float z_max = u_near_far.x * pow(u_near_far.y / u_near_far.x, float(gl_WorkGroupID.z + 1) / float(u_clusters.z));\n"

"   vec3 points[4] = vec3[4](\n"
"      LineIntersect(vec3(0.0), tile_min, z_min),\n"
"      LineIntersect(vec3(0.0), tile_min, z_max),\n"
"      LineIntersect(vec3(0.0), tile_max, z_min),\n"
"      LineIntersect(vec3(0.0), tile_max, z_max)\n"
"   );\n"

"   clusters[tile_id].bounds_min = vec4(min(points[1], points[0]), 0);\n"
"   clusters[tile_id].bounds_max = vec4(max(points[3], points[2]), 0);\n"
"}\n"
;

const char* rndr_CULL_SHADER_GLSL =
"#version 430 core\n"
"#define CLUSTERS_X 128\n"
"layout(local_size_x=CLUSTERS_X, local_size_y=1, local_size_z=1) in;\n"

RNDR_CAMERA_GLSL
RNDR_CLUSTER_GLSL
RNDR_LIGHT_GLSL

"uniform float u_light_cutoff;\n"
"uniform float u_fade_speed;"

"bool SphereTest(vec3 s_origin, float s_radius, vec3 bounds_min, vec3 bounds_max)\n"
"{\n"
"   vec3 diff = clamp(s_origin, bounds_min, bounds_max) - s_origin;\n"
"   return (dot(diff, diff) <= s_radius * s_radius);\n"
"}\n"

"bool LightTest(uint light_id, Cluster cluster)\n"
"{\n"
"   Light light = lights[light_id];\n"
"   if (light.light_type == LIGHT_DIR)\n"
"   {\n"
"      lights[light_id].fade_weight = 1.0;\n"
"      return true;\n"
"   }\n"

"   vec3 s_origin = (mat_view * vec4(light.origin.xyz, 1.0)).xyz;\n"
"   float s_radius = light.origin.w;\n"

"   vec3 bounds_min = cluster.bounds_min.xyz;\n"
"   vec3 bounds_max = cluster.bounds_max.xyz;\n"

"   vec3 extent = abs(bounds_min - bounds_max);\n"
"   float bounds_sqr = dot(extent, extent) * abs(s_origin.z);\n"

"   float importance = max(1.0 + light.importance_bias, s_radius * light.intensity * light.importance_scale) / abs(s_origin.z);\n"
"   lights[light_id].fade_weight = clamp((importance - u_light_cutoff) * u_fade_speed, 0.0, 1.0);\n"

"   return SphereTest(s_origin, s_radius, bounds_min, bounds_max) && (importance >= u_light_cutoff);\n"
"}\n"

"void main()\n"
"{\n"
"   uint tile_id = gl_WorkGroupID.x * CLUSTERS_X + gl_LocalInvocationID.x;\n"
"   Cluster cluster = clusters[tile_id];\n"
"   cluster.count = 0;\n"

"   uint light_count = lights.length();\n"
"   for (uint i=0; i<light_count; i++)\n"
"   {\n"
"      if (LightTest(i, cluster))\n"
"      {\n"
"         cluster.indices[cluster.count++] = i;\n"
"      }\n"
"      if (cluster.count >= LIGHTS_PER_CLUSTER)\n"
"         break;\n"
"   }\n"

"   clusters[tile_id] = cluster;"
"}\n"
;

const char* rndr_LIT_VRTSHADER_GLSL =
"#version 430 core\n"

"layout(location=0) in vec3 vrt_position;\n"
"layout(location=1) in vec3 vrt_normal;\n"

RNDR_CAMERA_GLSL

"uniform mat4 mat_model;\n"
"uniform mat4 mat_normal_model;\n"

"out vec3 v2f_position;\n"
"out vec3 v2f_normal;\n"

"void main()\n"
"{\n"
"   v2f_position = (mat_view * mat_model * vec4(vrt_position, 1.0)).xyz;\n"
"   v2f_normal = mat3(mat_view) * mat3(mat_normal_model) * vrt_normal;\n"
"   gl_Position = mat_proj * vec4(v2f_position, 1.0);\n"
"}\n"
;

const char* rndr_LIT_FRGSHADER_GLSL =
"#version 430 core\n"

"struct FragSurface\n"
"{\n"
"   vec3 position;\n"
"   vec3 normal;\n"
"   vec3 view;\n"
"   vec4 albedo;\n"
"};\n"

RNDR_CAMERA_GLSL
RNDR_CLUSTER_GLSL
RNDR_LIGHT_GLSL

"uniform uvec3 u_clusters;\n"
"uniform vec3 u_color;\n"

"in vec3 v2f_position;\n"
"in vec3 v2f_normal;\n"
"out vec4 frg_out;\n"

"vec4 MulQuat(vec4 a, vec4 b)\n"
"{\n"
"   vec4 res_q = vec4(0.0);\n"
"   res_q.xyz = b.xyz * a.w + a.xyz * b.w + cross(a.xyz, b.xyz);\n"
"   res_q.w = a.w * b.w - dot(a.xyz, b.xyz);\n"
"   return res_q;\n"
"}\n"

"vec3 RotPointByQuat(vec4 rotation, vec3 point)\n"
"{\n"
"   return MulQuat(MulQuat(vec4(-rotation.xyz, rotation.w), vec4(point, 0.0)), rotation).xyz;\n"
"}\n"

"vec4 DecodeColor(uint packed_color)\n"
"{\n"
"   const uint byte_mask = (1u << 8) - 1;\n"
"   const float rcp_maxbyte = 1.0 / float(byte_mask);\n"
"   return vec4(\n"
"      (packed_color >>  0) & byte_mask,\n"
"      (packed_color >>  8) & byte_mask,\n"
"      (packed_color >> 16) & byte_mask,\n"
"      (packed_color >> 24) & byte_mask\n"
"   ) * rcp_maxbyte;\n"
"}\n"

"uint EncodeColor(vec4 color)\n"
"{\n"
"   const uint byte_mask = (1u << 8) - 1;\n"
"   return\n"
"      (uint(color.r * byte_mask) <<  0) |\n"
"      (uint(color.g * byte_mask) <<  8) |\n"
"      (uint(color.b * byte_mask) << 16) |\n"
"      (uint(color.a * byte_mask) << 24);\n"
"}\n"

"float PointAttenution(vec3 light_pos)\n"
"{\n"
"   return pow(clamp(1.0 - dot(light_pos, light_pos), 0.0, 1.0), 4.0);\n"
"}\n"

"float SpotAttenuation(vec3 light_vec, vec3 light_dir, float cos_half_angle, float softness)\n"
"{\n"
"   float spot_dist = dot(light_vec, light_dir) * 0.5 + 0.5;\n"
"   return smoothstep(cos_half_angle, cos_half_angle + (1.0 - cos_half_angle) * softness, spot_dist);\n"
"}\n"

"vec3 CalcLight(FragSurface surface, Light light)\n"
"{\n"
"   const vec3 forward = vec3(0.0, 0.0,-1.0);\n"
"   vec3 light_vec;\n"

"   vec4 light_color = DecodeColor(light.color);\n"
"   light_color.rgb *= light.intensity * light.fade_weight;\n"

"   float atten = 1.0;\n"
"   if (light.light_type == LIGHT_DIR)\n"
"   {\n"
"      light_vec = (mat_view * vec4(RotPointByQuat(light.rotation, forward), 0)).xyz;\n"
"   } else {\n"
"      vec3 origin = (mat_view * vec4(light.origin.xyz, 1.0)).xyz;\n"
"      vec3 light_pos = (origin - surface.position) / light.origin.w;\n"
"      light_vec = normalize(light_pos);\n"
"      atten = PointAttenution(light_pos);\n"
"      if (light.light_type == LIGHT_SPOT)\n"
"      {\n"
"         vec3 light_dir = (mat_view * vec4(RotPointByQuat(light.rotation, forward), 0)).xyz;\n"
"         atten *= SpotAttenuation(light_vec, light_dir, light.cos_half_angle, light.softness);\n"
"      }\n"
"   }\n"
"   vec3 halfway = normalize(light_vec + surface.view);\n"
"   float nDl = max(0.0, dot(surface.normal, light_vec));\n"
"   float nDh = max(0.0, dot(surface.normal, halfway));\n"
"   return (surface.albedo.rgb + pow(nDh, 42.0)) * (nDl * atten) * light_color.rgb;\n"
"}\n"

"void main()\n"
"{\n"
"   FragSurface surface;\n"
"   surface.position = v2f_position;\n"
"   surface.normal = normalize(v2f_normal);\n"
"   surface.view = normalize(-v2f_position);\n"
"   surface.albedo = vec4(u_color, 1.0);\n"

"   vec2 tile_size = (vec2(u_screen_size) / vec2(u_clusters.xy));\n"

"   uint z_id = uint((log(abs(v2f_position.z) / u_near_far.x) * u_clusters.z) / log(u_near_far.y / u_near_far.x));\n"

"   uvec3 tile = uvec3(gl_FragCoord.xy / tile_size, z_id);\n"
"   uint tile_id = (tile.y * u_clusters.x) + (tile.z * u_clusters.x * u_clusters.y) + tile.x;\n"

"   uint light_count = clusters[tile_id].count;\n"

"   vec3 color = surface.albedo.rgb * 0.25;\n"
"   for (uint i=0; i<light_count; i++)\n"
"   {\n"
"      uint idx = clusters[tile_id].indices[i];\n"
"      Light light = lights[idx];\n"
"      color += CalcLight(surface, light);\n"
"   }\n"

"   const float rcp_gamma = 1.0 / 2.2;\n"
"   frg_out.rgb = pow(color, vec3(rcp_gamma));\n"
"   frg_out.a = 1.0;\n"

"   // #define DEBUG_CLUSTERS\n"

"   #ifdef DEBUG_CLUSTERS\n"
"   float light_fac = 10*2.0 * float(light_count) / float(LIGHTS_PER_CLUSTER);\n"
"   frg_out.rgb = clamp(frg_out.rgb, 0.0, 1.0) * 0.25;\n"
"   frg_out.rgb += mix(vec3(0.0, 0.0, 0.5), mix(vec3(1.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), max(light_fac - 1, 0)), min(light_fac, 1));\n"
"   #endif\n"
"}\n"
;

#endif
