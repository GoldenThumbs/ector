#include "util/array.h"
#include "util/math.h"
#include "util/vec3.h"
#include <util/quaternion.h>
#include <util/matrix.h>
#include <util/types.h>
#include <util/keymap.h>
#include <engine.h>
#include <graphics.h>
#include <mesh.h>
#include <renderer.h>

#include <stdio.h>

typedef struct CameraData_t
{
   mat4x4 view;
   mat4x4 proj;
   mat4x4 inv_view;
   mat4x4 inv_proj;
   f32 near_clip;
   f32 far_clip;
   u32 screen_width;
   u32 screen_height;
} CameraData;

int main(int argc, char* argv[])
{
   Engine* engine = Engine_Init(
      &(EngineDesc){ .app_name = "Game", .window.title = "Game Window" }
   );

   Engine_SetMouseMode(engine, MOUSE_DISABLE_CURSOR);
   
   GraphicsContext* gfx = Engine_GraphicsContext(engine);
   Graphics_SetClearColor(gfx, (color8){ 100, 100, 100, 255 });
   
   CameraData cam_data = {
      .view = Util_IdentityMat4(),
      .proj = Util_IdentityMat4(),
      .inv_view = Util_IdentityMat4(),
      .inv_proj = Util_IdentityMat4(),
      .near_clip = 0.25f,
      .far_clip = 100.0f,
      .screen_width = (u32)Engine_GetSize(engine).width,
      .screen_height = (u32)Engine_GetSize(engine).height
   };

   // NOTE: all angles are in half-turns (50 == 90 degrees, 100 == 180, etc...)
   struct {
      vec3 origin;
      vec3 euler;
      const f32 fov;
      const f32 move_speed;
      const f32 look_speed;
      const Key key_forward;
      const Key key_backward;
      const Key key_left;
      const Key key_right;
   } player = {
      VEC3(0.0f, 0.7f, 3.0f),
      VEC3(0.0f, 0.0f, 0.0f),
      50.0f,
      15.0f,
      0.15f,
      KEY_W,
      KEY_S,
      KEY_A,
      KEY_D
   };

   const u32 clusters_x = 16;
   const u32 clusters_y = 24;
   const u32 clusters_z = 64;
   const u32 cluster_count = clusters_x * clusters_y * clusters_z;

   Light* light_array = NEW_ARRAY_N(Light, 1);
   
   // Our sun is the first light in the array
   light_array[0].rotation = Util_MakeQuat(VEC3(-1, 0, 0), 28);
   light_array[0].light_type = LIGHT_DIR;
   light_array[0].color = (color8){ 255, 230, 205, 255 };
   light_array[0].intensity = 1.5f;
   light_array[0].softness = 0;

   Mesh floor_mesh = Mesh_CreatePlane(1, 1, VEC2(32, 32));
   Mesh box_mesh = Mesh_CreateBox(1, 1, 1, VEC3(2, 2, 2));

   Buffer cam_buf = Graphics_CreateBuffer(gfx, NULL, 1, sizeof(CameraData), GFX_DRAWMODE_DYNAMIC, GFX_BUFFERTYPE_UNIFORM);
   Graphics_UseBuffer(gfx, cam_buf, 1);

   Buffer cluster_buf = Graphics_CreateBuffer(gfx, NULL, cluster_count, sizeof(Cluster), GFX_DRAWMODE_STATIC_COPY, GFX_BUFFERTYPE_STORAGE);
   Graphics_UseBuffer(gfx, cluster_buf, 2);

   Buffer light_buf = Graphics_CreateBuffer(gfx, light_array, Util_ArrayLength(light_array), sizeof(Light), GFX_DRAWMODE_DYNAMIC, GFX_BUFFERTYPE_STORAGE);
   Graphics_UseBuffer(gfx, light_buf, 3);

   Geometry floor_geo = Graphics_CreateGeometry(gfx, floor_mesh, GFX_DRAWMODE_STATIC);
   Geometry box_geo = Graphics_CreateGeometry(gfx, box_mesh, GFX_DRAWMODE_STATIC);

   Shader lit_shader = Renderer_LitShader(gfx);
   Shader cluster_shader = Renderer_ClusterShader(gfx);
   Shader cull_shader = Renderer_CullShader(gfx);

   while(!Engine_ShouldQuit(engine))
   {
      if (Engine_CheckKey(engine, KEY_ESCAPE, KEY_IS_DOWN))
         Engine_Quit(engine);

      f64 frame_delta = Engine_GetFrameDelta(engine);
      vec2 mouse_delta = Engine_GetMouseDelta(engine);

      {
         player.euler.y -= mouse_delta.x * player.look_speed;
         player.euler.y += (player.euler.y > 200) ?-200 : ((player.euler.y < 0) ? 200 : 0);
         player.euler.x -= mouse_delta.y * player.look_speed;
         player.euler.x = M_MAX(-50, M_MIN( 50, player.euler.x));

         f32 yaw_sin = M_SIN(player.euler.y);
         f32 yaw_cos = M_COS(player.euler.y);

         vec3 move_vec = VEC3(0, 0, 0);
         if (Engine_CheckKey(engine, player.key_forward, KEY_IS_DOWN))
         {
            move_vec = Util_AddVec3(
               move_vec,
               VEC3(-yaw_sin, 0,-yaw_cos)
            );
         }

         if (Engine_CheckKey(engine, player.key_backward, KEY_IS_DOWN))
         {
            move_vec = Util_AddVec3(
               move_vec,
               VEC3( yaw_sin, 0, yaw_cos)
            );
         }

         if (Engine_CheckKey(engine, player.key_left, KEY_IS_DOWN))
         {
            move_vec = Util_AddVec3(
               move_vec,
               VEC3(-yaw_cos, 0, yaw_sin)
            );
         }

         if (Engine_CheckKey(engine, player.key_right, KEY_IS_DOWN))
         {
            move_vec = Util_AddVec3(
               move_vec,
               VEC3( yaw_cos, 0,-yaw_sin)
            );
         }

         move_vec = Util_NormalizeVec3(move_vec);
         player.origin = Util_AddVec3(
            player.origin,
            Util_ScaleVec3(move_vec, player.move_speed * frame_delta)
         );
      }
      
      size2i size = Engine_GetSize(engine);
      Graphics_Viewport(gfx, size);

      f32 aspect_ratio = (f32)size.height / (f32)size.width;

      cam_data.view = Util_ViewMatrix(player.origin, player.euler, 0.0f);
      cam_data.proj = Util_PerspectiveMatrix(50.0f, aspect_ratio, cam_data.near_clip, cam_data.far_clip);
      cam_data.inv_view = Util_InverseViewMatrix(cam_data.view);
      cam_data.inv_proj = Util_InversePerspectiveMatrix(cam_data.proj);
      cam_data.screen_width = size.width;
      cam_data.screen_height = size.height;

      mat4x4 floor_mat = Util_TranslationMatrix(VEC3(0,-1, 0));
      mat4x4 box_mat = Util_IdentityMat4();

      Graphics_UpdateBuffer(gfx, cam_buf, &cam_data, 1, sizeof(CameraData));
      // Graphics_UpdateBuffer(gfx, light_buf, light_array, Util_ArrayLength(light_array), sizeof(CameraData));
      // Uncomment line about to update lights on the gpu

      Graphics_Dispatch(gfx, cluster_shader, clusters_x, clusters_y, clusters_z, 1, (Uniform[]){
         [0].uniform_type = GFX_UNIFORMTYPE_U32_3X,
         [0].location = Graphics_GetUniformLocation(gfx, cluster_shader, "u_clusters"),
         [0].as_uint[0] = clusters_x,
         [0].as_uint[1] = clusters_y,
         [0].as_uint[2] = clusters_z
      });
      Graphics_DispatchBarrier();

      Graphics_Dispatch(gfx, cull_shader, cluster_count / 128, 1, 1, 2, (Uniform[]){
         [0].uniform_type = GFX_UNIFORMTYPE_F32_1X,
         [0].location = Graphics_GetUniformLocation(gfx, cull_shader, "u_light_cutoff"),
         [0].as_float[0] = 0.05f,

         [1].uniform_type = GFX_UNIFORMTYPE_F32_1X,
         [1].location = Graphics_GetUniformLocation(gfx, cull_shader, "u_fade_speed"),
         [1].as_float[0] = 5.0f
      });
      Graphics_DispatchBarrier();

      Graphics_Draw(gfx, lit_shader, floor_geo, 4, (Uniform[]){
         [0].uniform_type = GFX_UNIFORMTYPE_MAT4,
         [0].location = Graphics_GetUniformLocation(gfx, lit_shader, "mat_model"),
         [0].as_mat4 = floor_mat,

         [1].uniform_type = GFX_UNIFORMTYPE_MAT4,
         [1].location = Graphics_GetUniformLocation(gfx, lit_shader, "mat_normal_model"),
         [1].as_mat4 = Util_TransposeMat4(Util_InverseMat4(floor_mat)),

         [2].uniform_type = GFX_UNIFORMTYPE_U32_3X,
         [2].location = Graphics_GetUniformLocation(gfx, lit_shader, "u_clusters"),
         [2].as_uint[0] = clusters_x,
         [2].as_uint[1] = clusters_y,
         [2].as_uint[2] = clusters_z,

         [3].uniform_type = GFX_UNIFORMTYPE_F32_3X,
         [3].location = Graphics_GetUniformLocation(gfx, lit_shader, "u_color"),
         [3].as_vec3 = VEC3(0.3f, 0.3f, 0.3f)
      });

      Graphics_Draw(gfx, lit_shader, box_geo, 4, (Uniform[]){
         [0].uniform_type = GFX_UNIFORMTYPE_MAT4,
         [0].location = Graphics_GetUniformLocation(gfx, lit_shader, "mat_model"),
         [0].as_mat4 = box_mat,

         [1].uniform_type = GFX_UNIFORMTYPE_MAT4,
         [1].location = Graphics_GetUniformLocation(gfx, lit_shader, "mat_normal_model"),
         [1].as_mat4 = Util_TransposeMat4(Util_InverseMat4(box_mat)),

         [2].uniform_type = GFX_UNIFORMTYPE_U32_3X,
         [2].location = Graphics_GetUniformLocation(gfx, lit_shader, "u_clusters"),
         [2].as_uint[0] = clusters_x,
         [2].as_uint[1] = clusters_y,
         [2].as_uint[2] = clusters_z,

         [3].uniform_type = GFX_UNIFORMTYPE_F32_3X,
         [3].location = Graphics_GetUniformLocation(gfx, lit_shader, "u_color"),
         [3].as_vec3 = VEC3(1.0f, 0.2f, 0.2f)
      });
      
      Engine_Present(engine);
   }

   Mesh_Free(&floor_mesh);
   Mesh_Free(&box_mesh);
   FREE_ARRAY(light_array);

   Graphics_FreeBuffer(gfx, cam_buf);
   Graphics_FreeBuffer(gfx, cluster_buf);
   Graphics_FreeBuffer(gfx, light_buf);
   Graphics_FreeGeometry(gfx, floor_geo);
   Graphics_FreeGeometry(gfx, box_geo);
   Graphics_FreeShader(gfx, lit_shader);
   Graphics_FreeShader(gfx, cluster_shader);
   Graphics_FreeShader(gfx, cull_shader);

   Engine_Free(engine);
   return 0;
}
