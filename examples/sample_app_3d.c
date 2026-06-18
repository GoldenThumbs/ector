#include "util/quaternion.h"
#include "util/vec3.h"
#include <util/types.h>
#include <util/keymap.h>
#include <engine.h>
#include <graphics.h>
#include <renderer.h>
#include <mesh.h>
#include <default_modules.h>

#include <string.h>

int main(int argc, char* argv[])
{
   Engine* engine = Engine_Init(
      argc, argv,
      &(EngineDesc){ .app_name = "Sample App 3D", .window.title = "Ector Sample - Sample App 3D" }
   );

   Engine_RegisterModule(engine, Module_Graphics());
   Engine_RegisterModule(engine, Module_Renderer());

   Graphics* graphics = Engine_FetchModule(engine, GRAPHICS_MODULE);
   Graphics_SetClearColor(graphics, (color8){ 15, 15, 15, 255 });

   Renderer* renderer = Engine_FetchModule(engine, RENDERER_MODULE);

   Renderer_RegisterDrawableType(renderer, "TestEmpty", NULL);

   Texture rock0_texture = Renderer_LoadTexture(renderer, "assets/textures/rock0_albedo.png", (res2D){ 0 }, true, true);
   Texture rock1_texture = Renderer_LoadTexture(renderer, "assets/textures/rock1_albedo.png", (res2D){ 0 }, true, true);
   Texture rock2_texture = Renderer_LoadTexture(renderer, "assets/textures/rock2_albedo.png", (res2D){ 0 }, true, true);
   Texture floor_texture = Renderer_LoadTexture(renderer, "assets/textures/grass.png", (res2D){ 0 }, true, true);
   Shader shader = Renderer_LoadShader(renderer, "assets/core/shaders/builtin.glsl", NULL, 0, false);
   Surface surface = Renderer_AddSurface(renderer, "Basic", &(SurfaceDesc){
      .pass_count = 1,
      .passes[0] = {
         .shader = shader
      }
   });

   Model model = Renderer_LoadModel(renderer, "assets/models/rocks.ebmf");
   for (u32 mesh_i = 0; mesh_i < model.mesh_count; mesh_i++)
   {
      Mesh mesh = model.meshes[mesh_i];
      Geometry geometry = Graphics_CreateGeometry(graphics, mesh, GFX_DRAWMODE_STATIC);

      Drawable drawable = Renderer_CreateDrawable(renderer, GEOMETRY_DRAWABLE_TYPE);
      GeometryDrawable* drawable_data = Renderer_GetDrawableData(renderer, drawable);
      drawable_data->geometry = geometry;
      drawable_data->material.surface = surface;

      if (mesh.node_id > -1)
      {
         Node node = model.nodes[mesh.node_id];
         drawable_data->transform = node.transform;

         if (strncmp(node.name, "Floor", 64) != 0) {
            if (strncmp(node.name, "Rock0", 64) == 0)
               Renderer_SetSurfaceMaterialTexture(&drawable_data->material, 0, 0, rock0_texture);

            if (strncmp(node.name, "Rock1", 64) == 0)
               Renderer_SetSurfaceMaterialTexture(&drawable_data->material, 0, 0, rock1_texture);

            if (strncmp(node.name, "Rock2", 64) == 0)
               Renderer_SetSurfaceMaterialTexture(&drawable_data->material, 0, 0, rock2_texture);

         } else
            Renderer_SetSurfaceMaterialTexture(&drawable_data->material, 0, 0, floor_texture);

      }

   }
   Model_Free(&model);

   vec3 camera_euler = { 0, 0, 0 };
   quat camera_rot = Util_IdentityQuat();
   vec3 camera_origin = VEC3(0, 1, 0);
   Renderer_UpdateCamera(renderer, camera_origin, camera_euler, 0);

   bool mouse_is_locked = false;
   while(!Engine_CheckExitConditions(engine))
   {
      if (Engine_CheckKey(engine, KEY_ESCAPE, KEY_IS_DOWN))
         Engine_RequestExit(engine);

      if (Engine_CheckMouseButton(engine, MOUSE_BUTTON_LEFT_CLICK, KEY_IS_DOWN))
      {
         if (!mouse_is_locked)
         {
            Engine_SetMouseMode(engine, MOUSE_DISABLE_CURSOR);
            mouse_is_locked = true;

         }

         vec2 mouse_delta = Engine_GetMouseDelta(engine);
         camera_euler.x -= mouse_delta.y * 0.05f;
         camera_euler.y -= mouse_delta.x * 0.05f;

         camera_rot = Util_MakeQuatEuler(camera_euler);

      } else {
         if (mouse_is_locked)
         {
            Engine_SetMouseMode(engine, MOUSE_DEFAULT);
            mouse_is_locked = false;

         }

      }

      f32 speed = 2.5f * (f32)Engine_GetFrameDelta(engine);
      vec3 move_vec = { 0 };

      if (Engine_CheckKey(engine, KEY_W, KEY_IS_DOWN))
         move_vec = Util_AddVec3(move_vec, Util_RotatePoint(camera_rot, VEC3( 0, 0, speed)));

      if (Engine_CheckKey(engine, KEY_S, KEY_IS_DOWN))
         move_vec = Util_AddVec3(move_vec, Util_RotatePoint(camera_rot, VEC3( 0, 0,-speed)));

      if (Engine_CheckKey(engine, KEY_A, KEY_IS_DOWN))
         move_vec = Util_AddVec3(move_vec, Util_RotatePoint(camera_rot, VEC3( speed, 0, 0)));

      if (Engine_CheckKey(engine, KEY_D, KEY_IS_DOWN))
         move_vec = Util_AddVec3(move_vec, Util_RotatePoint(camera_rot, VEC3(-speed, 0, 0)));

      camera_origin = Util_AddVec3(camera_origin, move_vec);

      Renderer_UpdateCamera(renderer, camera_origin, camera_euler, 0);

      res2D size = Engine_GetFrameSize(engine);
      Graphics_Viewport(graphics, size);
      Graphics_Clear(graphics);

      Renderer_RenderPass(renderer, size, Engine_GetFrameDelta(engine), 0);

      Engine_Present(engine);
   }

   Engine_Free(engine);
   return 0;
}
