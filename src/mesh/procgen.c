#include "util/types.h"
#include "util/vec4.h"
#include "util/matrix.h"
#include "graphics.h"

#include "mesh.h"

void Mesh_AddQuad(u32 faces_x, u32 faces_y, mat4x4 transform, MeshInterface mesh_interface)
{
   u16 vertex_count = (u16)((faces_x + 1) * (faces_y + 1));
   u16 face_count = (u16)(faces_x * faces_y);

   u16 start = *mesh_interface.vertex_count;

   vec3* positions = realloc(*mesh_interface.atr.position, (uS)(start + vertex_count) * sizeof(vec3));
   if (positions != NULL)
   {
      for (u16 i=0; i<=faces_y; i++)
      {
         for (u16 j=0; j<=faces_x; j++)
         {
            vec4 point = VEC4((f32)j / (f32)faces_x - 0.5f, 0, (f32)i / (f32)faces_y - 0.5f, 1);
            positions[start + i * (faces_x + 1) + j] = Util_MulMat4Vec4(transform, point).xyz;
         }
      }
      
      *mesh_interface.atr.position = positions;
      *mesh_interface.vertex_count += vertex_count;
   }

   u16* indices = realloc(*mesh_interface.indices, (uS)(*mesh_interface.index_count + face_count * 6) * sizeof(u16));
   if (indices != NULL)
   {
      for (u16 i=0; i<faces_y; i++)
      {
         for (u16 j=0; j<faces_x; j++)
         {
            u16 base_y0 = start + i * (faces_x + 1) + j;
            u16 base_y1 = start + (i + 1) * (faces_x + 1) + j;

            u16 quad_ids[6] = {
               base_y0,
               base_y1 + 1,
               base_y0 + 1,
               base_y1,
               base_y1 + 1,
               base_y0
            };

            u16* ptr = indices + (uS)(*mesh_interface.index_count + (i * faces_x + j) * 6);
            memcpy(ptr, quad_ids, sizeof(quad_ids));
         }
      }

      *mesh_interface.indices = indices;
      *mesh_interface.index_count += face_count * 6;
   }
}

Mesh Mesh_CreatePlaneMesh(u32 faces_x, u32 faces_y, vec2 size)
{
   Mesh res = { 0 };
   res.primitive = GFX_PRIMITIVE_TRIANGLE;

   res.attribute_count = 1;
   res.attributes = malloc((uS)res.attribute_count * sizeof(u8));
   res.attributes[0] = GFX_ATTRIBUTE_F32_3X;
   
   MeshInterface mesh_interface = {
      &res.vertex_count,
      {
         .position = (vec3**)&res.vertex_buffer
      },
      &res.index_count,
      &res.index_buffer
   };

   mat4x4 t = Util_ScalingMatrix(VEC3(size.x, 1, size.y));
   Mesh_AddQuad(faces_x, faces_y, t, mesh_interface);

   return res;
}

Mesh Mesh_CreateBoxMesh(u32 faces_x, u32 faces_y, u32 faces_z, vec3 size)
{
   Mesh res = { 0 };
   res.primitive = GFX_PRIMITIVE_TRIANGLE;

   res.attribute_count = 1;
   res.attributes = malloc((uS)res.attribute_count * sizeof(u8));
   res.attributes[0] = GFX_ATTRIBUTE_F32_3X;
   
   MeshInterface mesh_interface = {
      &res.vertex_count,
      {
         .position = (vec3**)&res.vertex_buffer
      },
      &res.index_count,
      &res.index_buffer
   };
   mat4x4 s = Util_ScalingMatrix(size);
   mat4x4 t = Util_TranslationMatrix(VEC3(0, 0.5f, 0));
   mat4x4 r = Util_RotationMatrix(VEC3(1, 0, 0), 50);
   mat4x4 t0 = Util_MulMat4(s, t);
   mat4x4 t1 = Util_MulMat4(s, Util_MulMat4(r, t));
   mat4x4 t2 = Util_MulMat4(s, Util_MulMat4(Util_MulMat4(Util_RotationMatrix(VEC3(0, 1, 0), 50), r), t));
   mat4x4 t3 = Util_MulMat4(s, Util_MulMat4(Util_MulMat4(Util_RotationMatrix(VEC3(0, 1, 0),100), r), t));
   mat4x4 t4 = Util_MulMat4(s, Util_MulMat4(Util_MulMat4(Util_RotationMatrix(VEC3(0, 1, 0),150), r), t));
   mat4x4 t5 = Util_MulMat4(s, Util_MulMat4(Util_MulMat4(r, r), t));

   Mesh_AddQuad(faces_x, faces_z, t0, mesh_interface);
   Mesh_AddQuad(faces_x, faces_y, t1, mesh_interface);
   Mesh_AddQuad(faces_z, faces_y, t2, mesh_interface);
   Mesh_AddQuad(faces_x, faces_y, t3, mesh_interface);
   Mesh_AddQuad(faces_z, faces_y, t4, mesh_interface);
   Mesh_AddQuad(faces_x, faces_z, t5, mesh_interface);

   return res;
}
