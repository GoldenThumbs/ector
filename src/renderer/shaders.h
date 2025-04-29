#ifndef RNDR_SHADERS_INTERNAL
#define RNDR_SHADERS_INTERNAL

#include "renderer.h"

const char* rndr_CLUSTER_SHADER_GLSL =
"#version 430 core\n"

"layout(local_size_x=1, local_size_y=1, local_size_z=1) in;\n"

RNDR_CAMERA_GLSL
RNDR_CLUSTER_GLSL

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
"   uint tile_id = (gl_WorkGroupID.y * u_cluster_count.x) + (gl_WorkGroupID.z * u_cluster_count.x * u_cluster_count.y) + gl_WorkGroupID.x;\n"
"   vec2 tile_size = (vec2(u_screen_size) / vec2(u_cluster_count.xy));\n"

"   vec3 tile_min = ToViewSpace(vec2(gl_WorkGroupID.xy) * tile_size);\n"
"   vec3 tile_max = ToViewSpace(vec2(gl_WorkGroupID.xy+1) * tile_size);\n"

"   float z_min = u_near_far.x * pow(u_near_far.y / u_near_far.x, float(gl_WorkGroupID.z) / float(u_cluster_count.z));\n"
"   float z_max = u_near_far.x * pow(u_near_far.y / u_near_far.x, float(gl_WorkGroupID.z + 1) / float(u_cluster_count.z));\n"

"   vec3 points[4] = vec3[4](\n"
"      LineIntersect(vec3(0.0), tile_min, z_min),\n"
"      LineIntersect(vec3(0.0), tile_min, z_max),\n"
"      LineIntersect(vec3(0.0), tile_max, z_min),\n"
"      LineIntersect(vec3(0.0), tile_max, z_max)\n"
"   );\n"

"   vec3 b_min = min(points[1], points[0]);\n"
"   vec3 b_max = max(points[3], points[2]);\n"

"   clusters[tile_id].center = vec4(0.5 * (b_min + b_max), 0);\n"
"   clusters[tile_id].extents = vec4(b_max - clusters[tile_id].center.xyz, 0);\n"
"}\n"
;

const char* rndr_CULLING_SHADER_GLSL =
"#version 430 core\n"
"#define CLUSTERS_X 128\n"
"layout(local_size_x=CLUSTERS_X, local_size_y=1, local_size_z=1) in;\n"

RNDR_CAMERA_GLSL
RNDR_CLUSTER_GLSL
RNDR_LIGHT_GLSL

"layout(std140, binding=3) uniform SettingsUBO\n"
"{\n"
"   float u_light_cutoff;\n"
"   float u_fade_speed;\n"
"   vec2 padding;\n"
"};\n"

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

"bool SphereTest(vec3 s_origin, float s_radius, vec3 center, vec3 extents)\n"
"{\n"
"   vec3 diff = max(vec3(0.0), abs(center - s_origin) - extents);\n"
"   return (dot(diff, diff) <= s_radius * s_radius);\n"
"}\n"

"bool LightTest(uint light_id, Cluster cluster)\n"
"{\n"
"   Light light = lights[light_id];\n"

"   vec4 light_props = DecodeParameters(light.params);\n"
"   float lum = dot( DecodeColor(light.color), vec3(0.299, 0.587, 0.114));\n"
"   uint light_type = uint(light_props.w);\n"

"   if (light_type == LIGHT_DIR)\n"
"   {\n"
"      SetFadeWeight(light_id, 1.0);\n"
"      return true;\n"
"   }\n"

"   if (light_type == NULL_LIGHT)\n"
"      return false;\n"

"   vec3 s_origin = (mat_view * vec4(light.origin.xyz, 1.0)).xyz;\n"
"   float s_radius = light.origin.w;\n"

"   vec3 center = cluster.center.xyz;\n"
"   vec3 extents = cluster.extents.xyz;\n"

"   float importance = max(1.0 + light.importance.x, lum * s_radius * light.importance.y) / abs(s_origin.z);\n"
"   float fade_weight = clamp((importance - u_light_cutoff) * u_fade_speed, 0.0, 1.0);\n"
"   SetFadeWeight(light_id, fade_weight);\n"

"   bool test = SphereTest(s_origin, s_radius, center, extents);\n"

"   return test && (importance > u_light_cutoff);\n"
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
RNDR_MODEL_GLSL

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
"   vec4 color;\n"
"   float metallic;\n"
"   float inv_m;\n"
"   float roughness;\n"
"   float r_sqr;\n"
"   vec3 albedo;\n"
"   vec3 f0;\n"
"};\n"

RNDR_CAMERA_GLSL
RNDR_CLUSTER_GLSL
RNDR_LIGHT_GLSL

"layout(std140, binding=3) uniform SurfaceUBO\n"
"{\n"
"   vec4 u_color;\n"
"   float u_metallic;\n"
"   float u_roughness;\n"
"   float u_ambient;\n"
"};\n"

"in vec3 v2f_position;\n"
"in vec3 v2f_normal;\n"
"out vec4 frg_out;\n"

"const float M_EPSILON = 1e-8;\n"
"const float M_PI = 3.141592;\n"
"const float M_INVPI = 1.0 / M_PI;\n"

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

"float PointAttenution(vec3 light_pos)\n"
"{\n"
"   return pow(clamp(1.0 - dot(light_pos, light_pos), 0.0, 1.0), 4.0);\n"
"}\n"

"float SpotAttenuation(vec3 light_vec, vec3 light_dir, float cos_half_angle, float softness)\n"
"{\n"
"   float spot_dist = dot(light_vec, light_dir) * 0.5 + 0.5;\n"
"   return smoothstep(cos_half_angle, cos_half_angle + (1.0 - cos_half_angle) * softness, spot_dist);\n"
"}\n"

"vec3 Fresnel(vec3 f0, float cos_theta)\n"
"{\n"
"   return f0 + (1.0 - f0) * pow(1.0 - cos_theta, 5.0);\n"
"}\n"

"float Visibility(float nDl, float nDv, float r)\n"
"{\n"
"   // float r_sqr = r * r;\n"
"   // float l = nDv * sqrt((nDl - nDl * r_sqr) * nDl + r_sqr);\n"
"   // float v = nDl * sqrt((nDv - nDv * r_sqr) * nDv + r_sqr);\n"
"   return max(0.5 / mix(2.0 * nDl * nDv, nDl + nDv, r), M_EPSILON);\n"
"}\n"

"float Distribution(float nDh, float r)\n"
"{\n"
"   float inv_sqrmag = 1.0 - nDh * nDh;\n"
"   float a = r * nDh;\n"
"   float k = r / (inv_sqrmag + a * a);\n"
"   return max(k * k * M_INVPI, M_EPSILON);\n"
"}\n"

"vec3 CalcLight(FragSurface surface, Light light)\n"
"{\n"
"   const vec3 forward = vec3(0.0, 0.0,-1.0);\n"
"   vec3 light_vec;\n"

"   vec4 light_props = DecodeParameters(light.params);\n"
"   vec3 light_color = DecodeColor(light.color);\n"
"   light_color.rgb *= light_props.z;\n"

"   uint light_type = uint(light_props.w);\n"
"   float atten = 1.0;\n"
"   if (light_type == LIGHT_DIR)\n"
"   {\n"
"      light_vec = (mat_view * vec4(RotPointByQuat(light.rotation, forward), 0)).xyz;\n"
"   } else {\n"
"      vec3 origin = (mat_view * vec4(light.origin.xyz, 1.0)).xyz;\n"
"      vec3 light_pos = (origin - surface.position) / light.origin.w;\n"
"      light_vec = normalize(light_pos);\n"
"      atten = PointAttenution(light_pos);\n"
"      if (light_type == LIGHT_SPOT)\n"
"      {\n"
"         vec3 light_dir = (mat_view * vec4(RotPointByQuat(light.rotation, forward), 0)).xyz;\n"
"         atten *= SpotAttenuation(light_vec, light_dir, light_props.x, light_props.y);\n"
"      }\n"
"   }\n"
"   vec3 halfway = normalize(light_vec + surface.view);\n"
"   float nDl = max(0.0, dot(surface.normal, light_vec));\n"
"   float nDh = max(0.0, dot(surface.normal, halfway));\n"
"   float nDv = abs(dot(surface.normal, surface.view)) + M_EPSILON;\n"
"   float lDh = max(0.0, dot(light_vec, halfway));\n"

"   vec3 f = Fresnel(surface.f0, lDh);\n"
"   float d = Distribution(nDh, surface.r_sqr);\n"
"   float v = Visibility(nDl, nDv, surface.r_sqr);\n"

"   return (surface.albedo.rgb * M_INVPI + f * d * v) * (nDl * atten) * light_color.rgb;\n"
"}\n"

"void main()\n"
"{\n"
"   FragSurface surface;\n"
"   surface.position = v2f_position;\n"
"   surface.normal = normalize(v2f_normal);\n"
"   surface.view = normalize(-v2f_position);\n"
"   surface.color = u_color;\n"
"   surface.metallic = u_metallic;\n"
"   surface.inv_m = 1.0 - surface.metallic;\n"
"   surface.roughness = u_roughness;\n"
"   surface.r_sqr = surface.roughness * surface.roughness;\n"
"   surface.albedo = surface.color.rgb * surface.inv_m;\n"
"   surface.f0 = 0.04 * surface.inv_m + surface.color.rgb * surface.metallic;\n"

"   vec2 tile_size = vec2(u_screen_size) / vec2(u_cluster_count.xy);\n"

"   uint z_id = uint((log(abs(surface.position.z) / u_near_far.x) * u_cluster_count.z) / log(u_near_far.y / u_near_far.x));\n"

"   uvec3 tile = uvec3(gl_FragCoord.xy / tile_size, z_id);\n"
"   uint tile_id = (tile.y * u_cluster_count.x) + (tile.z * u_cluster_count.x * u_cluster_count.y) + tile.x;\n"

"   uint light_count = clusters[tile_id].count;\n"

"   vec3 color = surface.albedo.rgb * u_ambient;\n"
"   for (uint i=0; i<light_count; i++)\n"
"   {\n"
"      uint idx = clusters[tile_id].indices[i];\n"
"      Light light = lights[idx];\n"
"      color += CalcLight(surface, light);\n"
"   }\n"

"   const float rcp_gamma = 1.0 / 2.2;\n"
"   frg_out.rgb = color;\n"
"   frg_out.a = 1.0;\n"

"   #define DEBUG_CLUSTERS\n"

"   #ifdef DEBUG_CLUSTERS\n"
"   float light_fac = 10*2.0 * float(light_count) / float(LIGHTS_PER_CLUSTER);\n"
"   frg_out.rgb = clamp(frg_out.rgb, 0.0, 1.0) * 0.25;\n"
"   frg_out.rgb += mix(vec3(0.0, 0.0, 0.5), mix(vec3(1.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), max(light_fac - 1, 0)), min(light_fac, 1));\n"
"   #endif\n"
"}\n"
;

#endif
