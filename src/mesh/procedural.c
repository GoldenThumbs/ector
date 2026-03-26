#include "util/math.h"
#include "util/types.h"
#include "util/vec2.h"
#include "util/vec3.h"
#include "util/vec4.h"
#include "util/matrix.h"

#include "mesh.h"

//#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Mesh Mesh_CreatePlane(u32 faces_x, u32 faces_y, vec2 size)
{
   Mesh mesh = Mesh_EmptyMeshWithIndexType(MESH_PRIMITIVE_TRIANGLE, MESH_INDEXTYPE_32BIT);
   mesh.attribute_count = 4;
   mesh.attributes[0] = MESH_ATTRIBUTE_3_CHANNEL;
   mesh.attributes[1] = MESH_ATTRIBUTE_3_CHANNEL;
   mesh.attributes[2] = MESH_ATTRIBUTE_2_CHANNEL;
   mesh.attributes[3] = MESH_ATTRIBUTE_4_CHANNEL;
   
   MeshInterface mesh_interface = Mesh_NewInterface(&mesh);

   mat4x4 t = Util_ScalingMatrix(VEC3(size.x, 1, size.y));
   mesh_interface = Mesh_AddQuad(faces_x, faces_y, t, mesh_interface);

   return mesh;
}

Mesh Mesh_CreateBox(u32 faces_x, u32 faces_y, u32 faces_z, vec3 size)
{
   return Mesh_CreateBoxAdvanced(faces_x, faces_y, faces_z, size, false);
}

Mesh Mesh_CreateBoxAdvanced(u32 faces_x, u32 faces_y, u32 faces_z, vec3 size, bool smooth_seams)
{
   Mesh mesh = Mesh_EmptyMeshWithIndexType(MESH_PRIMITIVE_TRIANGLE, MESH_INDEXTYPE_32BIT);
   mesh.attribute_count = 4;
   mesh.attributes[0] = MESH_ATTRIBUTE_3_CHANNEL;
   mesh.attributes[1] = MESH_ATTRIBUTE_3_CHANNEL;
   mesh.attributes[2] = MESH_ATTRIBUTE_2_CHANNEL;
   mesh.attributes[3] = MESH_ATTRIBUTE_4_CHANNEL;
   
   MeshInterface mesh_interface = Mesh_NewInterface(&mesh);

   mat4x4 s = Util_ScalingMatrix(size);
   mat4x4 t = Util_TranslationMatrix(VEC3(0, 0.5f, 0));
   mat4x4 r = Util_RotationMatrix(VEC3(1, 0, 0), 50);
   mat4x4 t0 = Util_MulMat4(s, t);
   mat4x4 t1 = Util_MulMat4(s, Util_MulMat4(r, t));
   mat4x4 t2 = Util_MulMat4(s, Util_MulMat4(Util_MulMat4(Util_RotationMatrix(VEC3(0, 1, 0), 50), r), t));
   mat4x4 t3 = Util_MulMat4(s, Util_MulMat4(Util_MulMat4(Util_RotationMatrix(VEC3(0, 1, 0),100), r), t));
   mat4x4 t4 = Util_MulMat4(s, Util_MulMat4(Util_MulMat4(Util_RotationMatrix(VEC3(0, 1, 0),150), r), t));
   mat4x4 t5 = Util_MulMat4(s, Util_MulMat4(Util_MulMat4(r, r), t));

   mesh_interface = Mesh_AddQuad(faces_x, faces_z, t0, mesh_interface);
   mesh_interface = Mesh_AddQuad(faces_x, faces_y, t1, mesh_interface);
   mesh_interface = Mesh_AddQuad(faces_z, faces_y, t2, mesh_interface);
   mesh_interface = Mesh_AddQuad(faces_x, faces_y, t3, mesh_interface);
   mesh_interface = Mesh_AddQuad(faces_z, faces_y, t4, mesh_interface);
   mesh_interface = Mesh_AddQuad(faces_x, faces_z, t5, mesh_interface);

   if (smooth_seams)
      mesh_interface = Mesh_AverageNormalsOverSeams(mesh_interface);

   return mesh;
}

Mesh Mesh_CreateSphere(u32 faces, f32 size)
{
   Mesh mesh = Mesh_EmptyMeshWithIndexType(MESH_PRIMITIVE_TRIANGLE, MESH_INDEXTYPE_32BIT);
   mesh.attribute_count = 4;
   mesh.attributes[0] = MESH_ATTRIBUTE_3_CHANNEL;
   mesh.attributes[1] = MESH_ATTRIBUTE_3_CHANNEL;
   mesh.attributes[2] = MESH_ATTRIBUTE_2_CHANNEL;
   mesh.attributes[3] = MESH_ATTRIBUTE_4_CHANNEL;
   
   MeshInterface mesh_interface = Mesh_NewInterface(&mesh);

   mat4x4 s = Util_ScalingMatrix(Util_FillVec3(size));
   mat4x4 t = Util_TranslationMatrix(VEC3(0, 0.5f, 0));
   mat4x4 r = Util_RotationMatrix(VEC3(1, 0, 0), 50);
   mat4x4 t0 = Util_MulMat4(s, t);
   mat4x4 t1 = Util_MulMat4(s, Util_MulMat4(r, t));
   mat4x4 t2 = Util_MulMat4(s, Util_MulMat4(Util_MulMat4(Util_RotationMatrix(VEC3(0, 1, 0), 50), r), t));
   mat4x4 t3 = Util_MulMat4(s, Util_MulMat4(Util_MulMat4(Util_RotationMatrix(VEC3(0, 1, 0),100), r), t));
   mat4x4 t4 = Util_MulMat4(s, Util_MulMat4(Util_MulMat4(Util_RotationMatrix(VEC3(0, 1, 0),150), r), t));
   mat4x4 t5 = Util_MulMat4(s, Util_MulMat4(Util_MulMat4(r, r), t));

   mesh_interface = Mesh_AddQuad(faces, faces, t0, mesh_interface);
   mesh_interface = Mesh_AddQuad(faces, faces, t1, mesh_interface);
   mesh_interface = Mesh_AddQuad(faces, faces, t2, mesh_interface);
   mesh_interface = Mesh_AddQuad(faces, faces, t3, mesh_interface);
   mesh_interface = Mesh_AddQuad(faces, faces, t4, mesh_interface);
   mesh_interface = Mesh_AddQuad(faces, faces, t5, mesh_interface);

   vec3* position = (vec3*)mesh.vertex_buffer;
   vec3* normal = (vec3*)(mesh.vertex_buffer + mesh_interface.atr.normal_ofs);
   vec4* tangent = (vec4*)(mesh.vertex_buffer + mesh_interface.atr.tangent_ofs);

   for (u32 vert_i = 0; vert_i < mesh.vertex_count; vert_i++)
   {
      vec3 v_normal = Util_NormalizeVec3(position[vert_i]);
      position[vert_i] = Util_ScaleVec3(v_normal, size);
      normal[vert_i] = v_normal;

   }

   Mesh_GenTangents(mesh_interface);

   return mesh;
}

MeshInterface Mesh_ReallocVertices(u32 new_vertex_count, bool use_normal, bool use_texcoord0, bool use_texcoord1, bool use_tangent, MeshInterface mesh_interface)
{
   if (mesh_interface.mesh == NULL)
      return mesh_interface;

   uS position_bytes = (uS)new_vertex_count * sizeof(vec3);
   uS normal_bytes = (uS)new_vertex_count * sizeof(vec3);
   uS texcoord_bytes = (uS)new_vertex_count * sizeof(vec2);
   uS tangent_bytes = (uS)new_vertex_count * sizeof(vec4);

   uS total_bytes =
      position_bytes +
      normal_bytes * (uS)use_normal +
      texcoord_bytes * ((uS)use_texcoord0 + (uS)use_texcoord1) +
      tangent_bytes * (uS)use_tangent;
   
   MeshInterface new_mesh_interface = mesh_interface;

   u8* old_vertex_buffer = mesh_interface.mesh->vertex_buffer;
   u8* vertex_buffer = malloc(total_bytes);
   if (vertex_buffer != NULL)
   {
      vec3* old_position = (vec3*)old_vertex_buffer;
      uS old_position_size = mesh_interface.atr.position_size;

      new_mesh_interface.atr.position_size = position_bytes;
      vec3* position = (vec3*)vertex_buffer;

      if (old_position != NULL)
         memcpy(position, old_position, old_position_size);

      new_mesh_interface.total_bytes = position_bytes;

      if (use_normal)
      {
         vec3* old_normal = (vec3*)(old_vertex_buffer + mesh_interface.atr.normal_ofs);
         uS old_normal_size = mesh_interface.atr.normal_size;

         new_mesh_interface.atr.normal_ofs = new_mesh_interface.total_bytes;
         new_mesh_interface.atr.normal_size = normal_bytes;
         vec3* normal = (vec3*)(vertex_buffer + new_mesh_interface.total_bytes);

         if (old_normal != NULL)
            memcpy(normal, old_normal, old_normal_size);

         new_mesh_interface.total_bytes += normal_bytes;

      }

      if (use_texcoord0)
      {
         vec2* old_texcoord0 = (vec2*)(old_vertex_buffer + mesh_interface.atr.texcoord_ofs[0]);
         uS old_texcoord0_size = mesh_interface.atr.texcoord_size[0];

         new_mesh_interface.atr.texcoord_ofs[0] = new_mesh_interface.total_bytes;
         new_mesh_interface.atr.texcoord_size[0] = texcoord_bytes;
         vec2* texcoord0 = (vec2*)(vertex_buffer + new_mesh_interface.total_bytes);
         
         if (old_texcoord0 != NULL)
            memcpy(texcoord0, old_texcoord0, old_texcoord0_size);

         new_mesh_interface.total_bytes += texcoord_bytes;

      }

      if (use_texcoord1)
      {
         vec2* old_texcoord1 = (vec2*)(old_vertex_buffer + mesh_interface.atr.texcoord_ofs[1]);
         uS old_texcoord1_size = mesh_interface.atr.texcoord_size[1];

         new_mesh_interface.atr.texcoord_ofs[1] = new_mesh_interface.total_bytes;
         new_mesh_interface.atr.texcoord_size[1] = texcoord_bytes;
         vec2* texcoord1 = (vec2*)(vertex_buffer + new_mesh_interface.total_bytes);
         
         if (old_texcoord1 != NULL)
            memcpy(texcoord1, old_texcoord1, old_texcoord1_size);

         new_mesh_interface.total_bytes += texcoord_bytes;

      }

      if (use_tangent)
      {
         vec4* old_tangent = (vec4*)(old_vertex_buffer + mesh_interface.atr.tangent_ofs);
         uS old_tangent_size = mesh_interface.atr.tangent_size;

         new_mesh_interface.atr.tangent_ofs = new_mesh_interface.total_bytes;
         new_mesh_interface.atr.tangent_size = tangent_bytes;
         vec4* tangent = (vec4*)(vertex_buffer + new_mesh_interface.total_bytes);
         
         if (old_tangent != NULL)
            memcpy(tangent, old_tangent, old_tangent_size);

         new_mesh_interface.total_bytes += tangent_bytes;

      }

      if (old_vertex_buffer != NULL)
         free(old_vertex_buffer);

      new_mesh_interface.mesh->vertex_buffer = vertex_buffer;
      new_mesh_interface.mesh->vertex_count = new_vertex_count;
      
      mesh_interface = new_mesh_interface;

   }

   return mesh_interface;
}

MeshInterface Mesh_AddQuad(u32 faces_x, u32 faces_y, mat4x4 transform, MeshInterface mesh_interface)
{
   return Mesh_AddQuadAdvanced(faces_x, faces_y, transform, VEC4(0, 0, 1, 1), mesh_interface);
}

MeshInterface Mesh_AddQuadAdvanced(u32 faces_x, u32 faces_y, mat4x4 transform, vec4 texture_coords, MeshInterface mesh_interface)
{
   if (mesh_interface.mesh == NULL || mesh_interface.mesh->index_type != MESH_INDEXTYPE_32BIT)
      return mesh_interface;

   u32 vertex_count = (u32)((faces_x + 1) * (faces_y + 1));
   u32 index_count = (u32)(faces_x * faces_y * 6);

   u32 last_vrt = mesh_interface.mesh->vertex_count;
   u32 last_idx = mesh_interface.mesh->index_count;

   u32 total_verts = vertex_count + mesh_interface.mesh->vertex_count;

   bool has_texcoord1 = (mesh_interface.atr.texcoord_size[1] != 0);

   MeshInterface new_mesh_interface = Mesh_ReallocVertices(total_verts, true, true, false, true, mesh_interface);
   u8* vertex_buffer = new_mesh_interface.mesh->vertex_buffer;

   vec3* position = (vec3*)(vertex_buffer + mesh_interface.atr.position_size);

   uS normal_ofs = new_mesh_interface.atr.normal_ofs + mesh_interface.atr.normal_size;
   uS texcoord0_ofs = new_mesh_interface.atr.texcoord_ofs[0] + mesh_interface.atr.texcoord_size[0];
   uS tangent_ofs = new_mesh_interface.atr.tangent_ofs + mesh_interface.atr.tangent_size;

   mesh_interface = new_mesh_interface;

   vec3* normal = (vec3*)(vertex_buffer + normal_ofs);
   vec2* texcoord0 = (vec2*)(vertex_buffer + texcoord0_ofs);
   vec4* tangent = (vec4*)(vertex_buffer + tangent_ofs);

   mat4x4 normal_transform = Util_InverseMat4(Util_TransposeMat4(transform));

   vec3 plane_normal = Util_MulMat4Vec4(normal_transform, VEC4(0, 1, 0, 0)).xyz;
   plane_normal = Util_NormalizeVec3(plane_normal);

   vec4 plane_tangent = Util_MulMat4Vec4(normal_transform, VEC4(1, 0, 0, 0));
   plane_tangent.xyz = Util_NormalizeVec3(plane_tangent.xyz);
   plane_tangent.w = -1.0f;

   u32 vert_idx = 0;
   for (u32 i = 0; i < (faces_y + 1); i++)
   {
      for (u32 j = 0; j < (faces_x + 1); j++)
      {
         f32 x = (f32)j / (f32)faces_x;
         f32 y = (f32)i / (f32)faces_y;

         vec2 uv_fac = VEC2(x, 1.0f - y);
         vec2 inv_uv_fac = VEC2(1.0f - x, y);

         vec4 point = VEC4(x - 0.5f, 0, y - 0.5f, 1);
         position[vert_idx] = Util_MulMat4Vec4(transform, point).xyz;
         normal[vert_idx] = plane_normal;
         texcoord0[vert_idx] = Util_AddVec2(Util_MulVec2(texture_coords.xy, inv_uv_fac), Util_MulVec2(texture_coords.zw, uv_fac));
         tangent[vert_idx] = plane_tangent;

         vert_idx++;

      }
   }
   
   uS index_size = (mesh_interface.mesh->index_type == MESH_INDEXTYPE_16BIT) ? sizeof(u16) : sizeof(u32);
   u32* index_buffer = realloc(mesh_interface.mesh->index_buffer, (uS)(last_idx + index_count) * index_size);
   
   if (index_buffer != NULL)
   {
      u32 quad_idx = last_idx;
      for (u32 i=0; i < faces_y; i++)
      {
         for (u32 j=0; j < faces_x; j++)
         {
            u32 base_y0 = last_vrt + i * (faces_x + 1) + j;
            u32 base_y1 = last_vrt + (i + 1) * (faces_x + 1) + j;
            
            index_buffer[quad_idx++] = base_y0;
            index_buffer[quad_idx++] = base_y1 + 1;
            index_buffer[quad_idx++] = base_y0 + 1;
            index_buffer[quad_idx++] = base_y1;
            index_buffer[quad_idx++] = base_y1 + 1;
            index_buffer[quad_idx++] = base_y0;

         }

      }

      mesh_interface.mesh->index_buffer_32bit = index_buffer;
      mesh_interface.mesh->index_count += index_count;

   }

   return mesh_interface;
}

MeshInterface Mesh_GenNormals(MeshInterface mesh_interface)
{
   if (!Mesh_InterfaceIsValid(mesh_interface, true, false, false, false, false))
      return mesh_interface; // TODO: error tracking...

   u32 vertex_count = mesh_interface.mesh->vertex_count;
   u32 index_count = mesh_interface.mesh->index_count;

   bool has_texcoord0 = (mesh_interface.atr.texcoord_size[0] != 0);
   bool has_texcoord1 = (mesh_interface.atr.texcoord_size[1] != 0);

   MeshInterface new_mesh_interface = Mesh_ReallocVertices(vertex_count, true, has_texcoord0, has_texcoord1, false, mesh_interface);
   u8* vertex_buffer = new_mesh_interface.mesh->vertex_buffer;

   vec3* position = (vec3*)vertex_buffer;
   vec3* normal = (vec3*)(vertex_buffer + new_mesh_interface.atr.normal_ofs);
   memset(normal, 0, new_mesh_interface.atr.normal_size);

   mesh_interface = new_mesh_interface;
   
   for (u32 i = 0; i < index_count; i += 3)
   {
      u32 idx_a = Mesh_GetIndexFromBuffer(*mesh_interface.mesh, i + 0);
      u32 idx_b = Mesh_GetIndexFromBuffer(*mesh_interface.mesh, i + 1);
      u32 idx_c = Mesh_GetIndexFromBuffer(*mesh_interface.mesh, i + 2);

      vec3 vrt_a = position[idx_a];
      vec3 vrt_b = position[idx_b];
      vec3 vrt_c = position[idx_c];

      vec3 b_a = Util_SubVec3(vrt_b, vrt_a);
      vec3 c_a = Util_SubVec3(vrt_c, vrt_a);
      vec3 c_b = Util_SubVec3(vrt_c, vrt_b);
      vec3 a_b = Util_SubVec3(vrt_a, vrt_b);
      vec3 a_c = Util_SubVec3(vrt_a, vrt_c);
      vec3 b_c = Util_SubVec3(vrt_b, vrt_c);

      vec3 face_normal = Util_CrossVec3(b_a, c_a);
      f32 ang_a = Util_DotVec3(b_a, c_a) + 0.0001f;
      f32 ang_b = Util_DotVec3(c_b, a_b) + 0.0001f;
      f32 ang_c = Util_DotVec3(a_c, b_c) + 0.0001f;

      normal[idx_a] = Util_AddVec3(
         normal[idx_a],
         Util_ScaleVec3(face_normal, ang_a)
      );

      normal[idx_b] = Util_AddVec3(
         normal[idx_b],
         Util_ScaleVec3(face_normal, ang_b)
      );

      normal[idx_c] = Util_AddVec3(
         normal[idx_c],
         Util_ScaleVec3(face_normal, ang_c)
      );

   }

   for (u32 i = 0; i < vertex_count; i++)
      normal[i] = Util_NormalizeVec3(normal[i]);

   return mesh_interface;
}

MeshInterface Mesh_AverageNormalsOverSeams(MeshInterface mesh_interface)
{
   if (!Mesh_InterfaceIsValid(mesh_interface, true, true, false, false, false))
      return mesh_interface; // TODO: error tracking...

   u32 vertex_count = mesh_interface.mesh->vertex_count;

   vec3* position = (vec3*)mesh_interface.mesh->vertex_buffer;
   vec3* normal = (vec3*)(mesh_interface.mesh->vertex_buffer + mesh_interface.atr.normal_ofs);
   vec3* tmp_normal = malloc(mesh_interface.atr.normal_size);

   if (tmp_normal != NULL)
   {
      memcpy(tmp_normal, normal, mesh_interface.atr.normal_size);

      for (u32 i = 0; i < vertex_count; i++)
      {
         for (u32 j = 0; j < vertex_count; j++)
         {
            vec3 diff = Util_SubVec3(position[i], position[j]);
            if (Util_DotVec3(diff, diff) > 1e-7 || j == i)
               continue;

            tmp_normal[i] = Util_AddVec3(tmp_normal[i], normal[j]);
         }
      }

      for (u32 i = 0; i < vertex_count; i++)
         normal[i] = Util_NormalizeVec3(tmp_normal[i]);

      free(tmp_normal);
      
   }

   return mesh_interface;
}

MeshInterface Mesh_GenTexcoords(MeshInterface mesh_interface, vec3 triplanar_scale)
{
   if (!Mesh_InterfaceIsValid(mesh_interface, true, true, false, false, false))
      return mesh_interface; // TODO: error tracking...
   
   const f32 sqrt_third = M_SQRT(1.0f / 3.0f);

   u32 vertex_count = mesh_interface.mesh->vertex_count;
   u32 index_count = mesh_interface.mesh->index_count;

   bool has_texcoord1 = (mesh_interface.atr.texcoord_size[1] != 0);
   bool has_tangent = (mesh_interface.atr.tangent_size != 0);

   MeshInterface new_mesh_interface = Mesh_ReallocVertices(vertex_count, true, true, has_texcoord1, has_tangent, mesh_interface);
   u8* vertex_buffer = new_mesh_interface.mesh->vertex_buffer;

   u32 face_count = new_mesh_interface.mesh->index_count / 3;
   uS tmp_size = sizeof(vec3) * (uS)(face_count + vertex_count);

   vec3* position = (vec3*)vertex_buffer;
   vec3* normal = (vec3*)(vertex_buffer + new_mesh_interface.atr.normal_ofs);
   vec2* texcoord = (vec2*)(vertex_buffer + new_mesh_interface.atr.texcoord_ofs[0]);

   if (texcoord != NULL)
   {
      mesh_interface = new_mesh_interface;

      for (u32 i = 0; i < vertex_count; i++)
      {
         vec2 uv = { 0 };

         if (M_ABS(normal[i].y) >= M_ABS(normal[i].x) && M_ABS(normal[i].y) > M_ABS(normal[i].z))
         {
            uv.x = M_SIGN(normal[i].y) * position[i].x * M_RCP(triplanar_scale.x, 0.0001f);
            uv.y = -position[i].z * M_RCP(triplanar_scale.z, 0.0001f);

         } else if (M_ABS(normal[i].z) >= M_ABS(normal[i].x))
         {
            uv.x = M_SIGN(normal[i].z) * position[i].x * M_RCP(triplanar_scale.x, 0.0001f);
            uv.y = -position[i].y * M_RCP(triplanar_scale.y, 0.0001f);

         } else {
            uv.x = M_SIGN(normal[i].x) * position[i].z * M_RCP(triplanar_scale.z, 0.0001f);
            uv.y = -position[i].y * M_RCP(triplanar_scale.y, 0.0001f);

         }

         texcoord[i] = Util_AddVec2(Util_ScaleVec2(uv, 0.5f), Util_FillVec2(0.5f));

      }

   }

   return mesh_interface;
}

MeshInterface Mesh_GenTangents(MeshInterface mesh_interface)
{
   if (!Mesh_InterfaceIsValid(mesh_interface, true, true, true, false, false))
      return mesh_interface; // TODO: error tracking...

   u32 vertex_count = mesh_interface.mesh->vertex_count;
   u32 index_count = mesh_interface.mesh->index_count;

   bool has_texcoord1 = (mesh_interface.atr.texcoord_size[1] != 0);

   MeshInterface new_mesh_interface = Mesh_ReallocVertices(vertex_count, true, true, has_texcoord1, true, mesh_interface);
   u8* vertex_buffer = new_mesh_interface.mesh->vertex_buffer;

   vec3* position = (vec3*)vertex_buffer;
   vec3* normal = (vec3*)(vertex_buffer + new_mesh_interface.atr.normal_ofs);
   vec2* texcoord = (vec2*)(vertex_buffer + new_mesh_interface.atr.texcoord_ofs[0]);
   vec4* tangent = (vec4*)(vertex_buffer + new_mesh_interface.atr.tangent_ofs);
   vec3* tmp_tangent = malloc(new_mesh_interface.atr.normal_size * 2);

   if (tmp_tangent != NULL)
   {
      mesh_interface = new_mesh_interface;

      memset(tmp_tangent, 0, new_mesh_interface.atr.normal_size * 2);
      vec3* tmp_bitangent = tmp_tangent + vertex_count;

      for (u32 i = 0; i < index_count; i += 3)
      {
         u32 idx_a = Mesh_GetIndexFromBuffer(*mesh_interface.mesh, i + 0);
         u32 idx_b = Mesh_GetIndexFromBuffer(*mesh_interface.mesh, i + 1);
         u32 idx_c = Mesh_GetIndexFromBuffer(*mesh_interface.mesh, i + 2);

         vec3 vrt_a = position[idx_a];
         vec3 vrt_b = position[idx_b];
         vec3 vrt_c = position[idx_c];

         vec2 tex_a = texcoord[idx_a];
         vec2 tex_b = texcoord[idx_b];
         vec2 tex_c = texcoord[idx_c];

         f32 x1 = vrt_b.x - vrt_a.x;
         f32 x2 = vrt_c.x - vrt_a.x;
         f32 y1 = vrt_b.y - vrt_a.y;
         f32 y2 = vrt_c.y - vrt_a.y;
         f32 z1 = vrt_b.z - vrt_a.z;
         f32 z2 = vrt_c.z - vrt_a.z;
         
         f32 s1 = tex_b.x - tex_a.x;
         f32 s2 = tex_c.x - tex_a.x;
         f32 t1 = tex_b.y - tex_a.y;
         f32 t2 = tex_c.y - tex_a.y;

         f32 r = 1.0f / (s1 * t2 - s2 * t1);
         vec3 s_dir = VEC3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
         vec3 t_dir = VEC3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

         tmp_tangent[idx_a] = Util_AddVec3(tmp_tangent[idx_a], s_dir);
         tmp_tangent[idx_b] = Util_AddVec3(tmp_tangent[idx_b], s_dir);
         tmp_tangent[idx_c] = Util_AddVec3(tmp_tangent[idx_c], s_dir);

         tmp_bitangent[idx_a] = Util_AddVec3(tmp_bitangent[idx_a], t_dir);
         tmp_bitangent[idx_b] = Util_AddVec3(tmp_bitangent[idx_b], t_dir);
         tmp_bitangent[idx_c] = Util_AddVec3(tmp_bitangent[idx_c], t_dir);

      }

      for (u32 i = 0; i < vertex_count; i++)
      {
         vec3 n = normal[i];
         vec3 t = tmp_tangent[i];

         tangent[i].xyz = Util_NormalizeVec3(Util_SubVec3(t, Util_ScaleVec3(n, Util_DotVec3(n, t))));
         tangent[i].w = (Util_DotVec3(Util_CrossVec3(n, t), tmp_bitangent[i]) < 0.0f) ? 1.0f : -1.0f;

      }

      free(tmp_tangent);

   }

   return mesh_interface;
}
