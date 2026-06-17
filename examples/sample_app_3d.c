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
      &(EngineDesc){ .app_name = "Game", .window.title = "Game Window" }
   );

   Engine_RegisterModule(engine, Module_Graphics());
   Engine_RegisterModule(engine, Module_Renderer());

   Graphics* graphics = Engine_FetchModule(engine, GRAPHICS_MODULE);
   Graphics_SetClearColor(graphics, (color8){ 15, 15, 15, 255 });

   Renderer* renderer = Engine_FetchModule(engine, RENDERER_MODULE);
   Renderer_UpdateCamera(renderer, VEC3(0, 1, 0), VEC3(0), 0);

   Texture texture = Renderer_LoadTexture(renderer, "assets/textures/grass.png", (res2D){ 0 }, true, true);
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

         if (strncmp(node.name, "Floor", 64) != 0)
            drawable_data->color = Util_IntToColor(0x323232FF);
         else
            Renderer_SetSurfaceMaterialTexture(&drawable_data->material, 0, 0, texture);

      }

   }

   f32 camera_yaw = 0;
   f32 camera_pitch = 0;

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
         camera_pitch -= mouse_delta.y * 0.05f;
         camera_yaw -= mouse_delta.x * 0.05f;

         Renderer_UpdateCamera(renderer, VEC3(0, 1, 0), VEC3(camera_pitch, camera_yaw, 0), 0);

      } else {
         if (mouse_is_locked)
         {
            Engine_SetMouseMode(engine, MOUSE_DEFAULT);
            mouse_is_locked = false;

         }

      }

      res2D size = Engine_GetFrameSize(engine);
      Graphics_Viewport(graphics, size);
      Graphics_Clear(graphics);

      Renderer_RenderPass(renderer, size, Engine_GetFrameDelta(engine), 0);

      Engine_Present(engine);
   }

   Model_Free(&model);

   Engine_Free(engine);
   return 0;
}
