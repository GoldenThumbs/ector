// #include "util/array.h"
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

// #include <stdio.h>

struct SurfaceDef_s
{
   vec4 color;
   f32 metallic;
   f32 roughness;
   f32 ambient;
};

void CreateRandomLights(Renderer* renderer, u32 count_x, u32 count_y);

int main(int argc, char* argv[])
{
   Engine* engine = Engine_Init(
      &(EngineDesc){ .app_name = "Game", .window.title = "Game Window", .renderer.enabled = true }
   );

   Engine_SetMouseMode(engine, MOUSE_DISABLE_CURSOR);
   
   GraphicsContext* gfx = Engine_FetchModule(engine, "gfx");
   Graphics_SetClearColor(gfx, (color8){ 100, 160, 220, 255 });

   Renderer* rndr = Engine_FetchModule(engine, "rndr");

   // NOTE: all angles are in half-turns (50 == 90 degrees, 100 == 180, etc...)
   struct {
      vec3 origin;
      vec3 euler;
      const f32 move_speed;
      const f32 look_speed;
      const Key key_forward;
      const Key key_backward;
      const Key key_left;
      const Key key_right;
   } player = {
      VEC3(0, 1, 3),
      VEC3(0, 0, 0),
      15,
      0.15f,
      KEY_W,
      KEY_S,
      KEY_A,
      KEY_D
   };

   Renderer_AddLight(rndr, &(LightDesc){
      .light_type = RNDR_LIGHT_DIR,
      .rotation = Util_MakeQuatEuler(VEC3(-32, 20, 0)),
      .color = VEC3(1.0f, 0.9f, 0.6f),
      .strength = 0.5f
   });

   Renderer_AddLight(rndr, &(LightDesc){
      .light_type = RNDR_LIGHT_SPOT,
      .radius = 10.0f,
      .origin = VEC3(-2, 1.5f,-2),
      .cone_angle = 30,
      .softness_fac = 0.2f,
      .importance.bias = 0.25f,
      .importance.scale = 2.0f,
      .rotation = Util_MakeQuatEuler(VEC3(0, 0, 0)),
      .color = VEC3(0.9f, 0.9f, 1.0f),
      .strength = 2.0f
   });

   // Everything else is random
   CreateRandomLights(rndr, 16, 16);

   Mesh floor_mesh = Mesh_CreatePlane(8, 8, VEC2(32, 32));
   Mesh box_mesh = Mesh_CreateBox(1, 1, 1, VEC3(2, 2, 2));

   Geometry floor_geo = Graphics_CreateGeometry(gfx, floor_mesh, GFX_DRAWMODE_STATIC);
   Geometry box_geo = Graphics_CreateGeometry(gfx, box_mesh, GFX_DRAWMODE_STATIC);

   Shader lit_shader = Renderer_LitShader(gfx);

   struct SurfaceDef_s floor_surf = {
      .color = VEC4(0.5f, 0.5f, 0.5f, 1),
      .metallic = 0,
      .roughness = 0.3f,
      .ambient = 0.6f
   };

   struct SurfaceDef_s box_surf = {
      .color = VEC4(1.0f, 0.3f, 0.3f, 1),
      .metallic = 0,
      .roughness = 0.1f,
      .ambient = 0.6f
   };

   Object floor_obj = Renderer_AddObject(rndr, &(ObjectDesc){
         .shader = lit_shader,
         .geometry = floor_geo,
         .bounds = { .extents = VEC3(16, 1, 16) },
         .uniforms = (Uniforms){
            .count = 1,
            .blocks[0] = {
               .binding = 3,
               .ubo = Graphics_CreateBuffer(gfx, (void*)&floor_surf, 1, sizeof(struct SurfaceDef_s), GFX_DRAWMODE_DYNAMIC, GFX_BUFFERTYPE_UNIFORM)
            }
         }
      },
      (Transform3D){
         .origin = VEC3(0, 0, 0),
         .rotation = Util_IdentityQuat(),
         .scale = VEC3(1, 1, 1)
      }
   );

   Object box_obj = Renderer_AddObject(rndr, &(ObjectDesc){
         .shader = lit_shader,
         .geometry = box_geo,
         .bounds = { .extents = VEC3(1, 1, 1) },
         .uniforms = (Uniforms){
            .count = 1,
            .blocks[0] = {
               .binding = 3,
               .ubo = Graphics_CreateBuffer(gfx, (void*)&box_surf, 1, sizeof(struct SurfaceDef_s), GFX_DRAWMODE_DYNAMIC, GFX_BUFFERTYPE_UNIFORM)
            }
         }
      },
      (Transform3D){
         .origin = VEC3(0, 1.5f, 0),
         .rotation = Util_MakeQuatEuler(VEC3(0, 20, 0)),
         .scale = VEC3(1, 1, 1)
      }
   );

   while(!Engine_CheckExitConditions(engine))
   {
      if (Engine_CheckKey(engine, KEY_ESCAPE, KEY_IS_DOWN))
         Engine_RequestExit(engine);

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
      
      resolution2d size = Engine_GetFrameSize(engine);

      Renderer_SetView(rndr, Util_ViewMatrix(player.origin, player.euler, 0));
      Renderer_RenderLit(rndr, size);
      
      Engine_Present(engine);
   }

   Mesh_Free(&floor_mesh);
   Mesh_Free(&box_mesh);

   Engine_Free(engine);
   return 0;
}

u32 XorShift(u32 x)
{
   x ^= (x << 13);
   x ^= (x >> 17);
   x ^= (x <<  5);
   return x;
}

void CreateRandomLights(Renderer* renderer, u32 count_x, u32 count_y)
{
   u32 seed = 12;
   for (u32 i=0; i<count_y; i++)
   {
      f32 y = 2 * ((f32)i - (f32)count_y * 0.5f + 0.5f);
      for (u32 j=0; j<count_x; j++)
      {
         Light light = { 0 };

         f32 x = 2 * ((f32)j - (f32)count_x * 0.5f + 0.5);

         seed = XorShift(seed);

         u32 offset_seed = XorShift(seed + 1);

         const u32 two_bytes = (1u << 16) - 1;
         x += (f32)((offset_seed >> 0) & 255) / 255.0f;
         y += (f32)((offset_seed >> 1) & 255) / 255.0f;
         f32 h = (f32)((offset_seed >> 2) & two_bytes) / (f32)two_bytes * 3.0f + 0.25f;

         LightDesc desc = { 0 };
         desc.origin = VEC3(x, h, y);
         desc.radius = (f32)(XorShift(seed + 2) % 128) / 32 + 2;
         desc.light_type = RNDR_LIGHT_POINT;
         desc.color.r = (f32)(XorShift(seed + 23) % 512) / 512.0f;
         desc.color.g = (f32)(XorShift(seed + 22) % 512) / 512.0f;
         desc.color.b = (f32)(XorShift(seed + 21) % 512) / 512.0f;
         desc.strength = (f32)(XorShift(seed + 4) % 128) / 150.0f + 0.5f;
         Renderer_AddLight(renderer, &desc);
      }
   }
}
