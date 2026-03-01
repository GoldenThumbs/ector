
layout(local_size_x=1, local_size_y=1, local_size_z=1) in;

struct Cluster
{
   vec4 center;
   vec4 extents;
   uvec4 light_count;
   uint indices[200];
   
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

vec3 ToViewSpace(vec2 point_ss)
{
   // conversion to normalized device coordinates
   vec4 ndc = vec4(point_ss / vec2(u_screen_size) * 2.0 - 1.0, -1.0, 1.0);
   vec4 point_vs = mat_invproj * ndc;
   point_vs /= point_vs.w;
   return point_vs.xyz;
}

vec3 LineIntersect(vec3 p_start, vec3 p_end, float d)
{
   const vec3 normal = vec3( 0, 0,-1);
   vec3 dir = p_end - p_start;
   float t = (d - dot(normal, p_start)) / dot(normal, dir);
   return p_start + t * dir;
}

void main()
{
   uint tile_id = (gl_WorkGroupID.y * u_cluster_dimensions.x) + (gl_WorkGroupID.z * u_cluster_dimensions.x * u_cluster_dimensions.y) + gl_WorkGroupID.x;
   vec2 tile_size = (vec2(u_screen_size) / vec2(u_cluster_dimensions.xy));

   vec3 tile_min = ToViewSpace(vec2(gl_WorkGroupID.xy) * tile_size);
   vec3 tile_max = ToViewSpace(vec2(gl_WorkGroupID.xy+1) * tile_size);

   float z_min = u_near_far.x * pow(u_near_far.y / u_near_far.x, float(gl_WorkGroupID.z) / float(u_cluster_dimensions.z));
   float z_max = u_near_far.x * pow(u_near_far.y / u_near_far.x, float(gl_WorkGroupID.z + 1) / float(u_cluster_dimensions.z));

   vec3 points[4] = vec3[4](
      LineIntersect(vec3(0.0), tile_min, z_min),
      LineIntersect(vec3(0.0), tile_min, z_max),
      LineIntersect(vec3(0.0), tile_max, z_min),
      LineIntersect(vec3(0.0), tile_max, z_max)
   );

   vec3 b_min = min(points[1], points[0]);
   vec3 b_max = max(points[3], points[2]);

   clusters[tile_id].center = vec4(0.5 * (b_min + b_max), 0);
   clusters[tile_id].extents = vec4(b_max - clusters[tile_id].center.xyz, 0);
}
