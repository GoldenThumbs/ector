#include "util/math.h"
#include "util/types.h"
#include "util/vec3.h"
#include "util/vec4.h"
#include "util/matrix.h"

#include "mesh.h"

//#include <stdio.h>
#include <stdlib.h>

Mesh Mesh_CreatePlane(u32 faces_x, u32 faces_y, vec2 size)
{
   Mesh mesh = Mesh_EmptyMesh(MESH_PRIMITIVE_TRIANGLE);
   mesh.attribute_count = 3;
   mesh.attributes[0] = MESH_ATTRIBUTE_3_CHANNEL;
   mesh.attributes[1] = MESH_ATTRIBUTE_3_CHANNEL;
   mesh.attributes[2] = MESH_ATTRIBUTE_2_CHANNEL;
   
   MeshInterface mesh_interface = Mesh_NewInterface(&mesh);

   mat4x4 t = Util_ScalingMatrix(VEC3(size.x, 1, size.y));
   mesh_interface = Mesh_AddQuad(faces_x, faces_y, t, mesh_interface);
   mesh_interface = Mesh_GenNormals(mesh_interface);
   mesh_interface = Mesh_GenTexcoords(mesh_interface);

   return mesh;
}

Mesh Mesh_CreateBox(u32 faces_x, u32 faces_y, u32 faces_z, vec3 size)
{
   Mesh mesh = Mesh_EmptyMesh(MESH_PRIMITIVE_TRIANGLE);
   mesh.attribute_count = 3;
   mesh.attributes[0] = MESH_ATTRIBUTE_3_CHANNEL;
   mesh.attributes[1] = MESH_ATTRIBUTE_3_CHANNEL;
   mesh.attributes[2] = MESH_ATTRIBUTE_2_CHANNEL;
   
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
   mesh_interface = Mesh_GenNormals(mesh_interface);
   mesh_interface = Mesh_GenTexcoords(mesh_interface);

   return mesh;
}

MeshInterface Mesh_AddQuad(u32 faces_x, u32 faces_y, mat4x4 transform, MeshInterface mesh_interface)
{
   u16 vertex_count = (u16)((faces_x + 1) * (faces_y + 1));
   u16 index_count = (u16)(faces_x * faces_y * 6);

   u16 last_vrt = mesh_interface.mesh->vertex_count;
   u16 last_idx = mesh_interface.mesh->index_count;
   uS last_byte = mesh_interface.total_bytes;
   uS total_bytes = (uS)vertex_count * sizeof(vec3);
   mesh_interface.atr.position_size += total_bytes;

   u8* vertex_buffer = realloc(mesh_interface.mesh->vertex_buffer, last_byte + total_bytes);

   if (vertex_buffer != NULL)
   {
      vec3* position = (vec3*)(vertex_buffer + last_byte);

      u16 vert_idx = 0;
      for (u16 i=0; i < (faces_y + 1); i++)
      {
         for (u16 j=0; j < (faces_x + 1); j++)
         {
            f32 x = (f32)j / (f32)faces_x - 0.5f;
            f32 y = (f32)i / (f32)faces_y - 0.5f;
            vec4 point = VEC4(x, 0, y, 1);
            position[vert_idx++] = Util_MulMat4Vec4(transform, point).xyz;
         }
      }
      
      mesh_interface.mesh->vertex_buffer = vertex_buffer;
      mesh_interface.mesh->vertex_count += vertex_count;
      mesh_interface.total_bytes += total_bytes;
   }
   
   u16* index_buffer = realloc(mesh_interface.mesh->index_buffer, (uS)(last_idx + index_count) * sizeof(u16));
   
   if (index_buffer != NULL)
   {
      u16 quad_idx = last_idx;
      for (u16 i=0; i < faces_y; i++)
      {
         for (u16 j=0; j < faces_x; j++)
         {
            u16 base_y0 = last_vrt + i * (u16)(faces_x + 1) + j;
            u16 base_y1 = last_vrt + (i + 1) * (u16)(faces_x + 1) + j;
            
            index_buffer[quad_idx++] = base_y0;
            index_buffer[quad_idx++] = base_y1 + 1;
            index_buffer[quad_idx++] = base_y0 + 1;
            index_buffer[quad_idx++] = base_y1;
            index_buffer[quad_idx++] = base_y1 + 1;
            index_buffer[quad_idx++] = base_y0;
         }
      }

      mesh_interface.mesh->index_buffer = index_buffer;
      mesh_interface.mesh->index_count += index_count;
   }

   return mesh_interface;
}

MeshInterface Mesh_GenNormals(MeshInterface mesh_interface)
{
   if (mesh_interface.atr.position_size == 0)
      return mesh_interface; // TODO: error tracking...

   u16 vertex_count = mesh_interface.mesh->vertex_count;
   u16 index_count = mesh_interface.mesh->index_count;
   uS last_byte = mesh_interface.total_bytes;
   uS total_bytes = (uS)vertex_count * sizeof(vec3);

   mesh_interface.atr.normal_ofs = last_byte;
   mesh_interface.atr.normal_size = total_bytes;

   u8* vertex_buffer = realloc(mesh_interface.mesh->vertex_buffer, last_byte + total_bytes);
   if (vertex_buffer != NULL)
   {
      vec3* position = (vec3*)vertex_buffer;
      vec3* normal = (vec3*)(vertex_buffer + last_byte);
      memset(normal, 0, total_bytes);
      
      for (u16 i=0; i < index_count; i+=3)
      {
         u16 idx_a = mesh_interface.mesh->index_buffer[i + 0];
         u16 idx_b = mesh_interface.mesh->index_buffer[i + 1];
         u16 idx_c = mesh_interface.mesh->index_buffer[i + 2];

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
         f32 ang_a = Util_DotVec3(b_a, c_a) + 0.001f;
         f32 ang_b = Util_DotVec3(c_b, a_b) + 0.001f;
         f32 ang_c = Util_DotVec3(a_c, b_c) + 0.001f;

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

      for (u16 i=0; i<vertex_count; i++)
      {
         normal[i] = Util_NormalizeVec3(normal[i]);
      }

      mesh_interface.mesh->vertex_buffer = vertex_buffer;
      mesh_interface.total_bytes += total_bytes;
   }

   return mesh_interface;
}

MeshInterface Mesh_GenTexcoords(MeshInterface mesh_interface)
{
   const f32 sqrt_third = M_SQRT(1.0f / 3.0f);
   if ((mesh_interface.atr.position_size == 0) || (mesh_interface.atr.normal_size == 0))
      return mesh_interface; // TODO: error tracking...

   u16 vertex_count = mesh_interface.mesh->vertex_count;
   u16 index_count = mesh_interface.mesh->index_count;
   uS last_byte = mesh_interface.total_bytes;
   uS total_bytes = (uS)vertex_count * sizeof(vec2);

   mesh_interface.atr.texcoord_ofs[0] = last_byte;
   mesh_interface.atr.texcoord_size[0] = total_bytes;

   u8* vertex_buffer = realloc(mesh_interface.mesh->vertex_buffer, last_byte + total_bytes);
   if (vertex_buffer != NULL)
   {
      vec3* position = (vec3*)vertex_buffer;
      vec3* normal = (vec3*)(vertex_buffer + mesh_interface.atr.normal_ofs);
      vec2* texcoord = (vec2*)(vertex_buffer + last_byte);
      memset(texcoord, 0, total_bytes);

      for (u16 i=0; i<vertex_count; i++)
      {
         mat3x3 basis = { 0 };
         basis.v[2] = normal[i];

         if (M_ABS(normal[i].x) >= sqrt_third)
            basis.v[0] = Util_ScaleVec3(VEC3( normal[i].y, normal[i].x, 0), M_SIGN(normal[i].z));
         else
            basis.v[0] = Util_ScaleVec3(VEC3( 0, normal[i].z,-normal[i].y),-M_SIGN(normal[i].x));

         basis.v[0] = Util_NormalizeVec3(basis.v[0]);
         basis.v[1] = Util_CrossVec3(normal[i], basis.v[0]);

         vec3 coord = Util_MulMat3Vec3(Util_TransposeMat3(basis), position[i]);

         texcoord[i] = VEC2(coord.y * 0.5 + 0.5, coord.x * 0.5 + 0.5);
      }

      mesh_interface.mesh->vertex_buffer = vertex_buffer;
      mesh_interface.total_bytes += total_bytes;
   }

   return mesh_interface;
}
