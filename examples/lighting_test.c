#include <util/types.h>
#include <util/array.h>
#include <util/extra_types.h>
#include <util/math.h>
#include <util/vec3.h>
#include <util/quaternion.h>
#include <util/matrix.h>
#include <util/keymap.h>
#include <util/files.h>
#include <mesh.h>
#include <image.h>
#include <engine.h>
#include <graphics.h>
#include <renderer.h>
#include <default_modules.h>
#include <default_lightmanager.h>

#include <stdlib.h>
#include <stdio.h>

struct LampInfo_t
{
   Geometry* geometries;
   SurfaceMaterial* materials;

} LampInfo;

Model LoadModel(const char* model_name, const char* base_path);
void CreateModelGeometry(Graphics* graphics, Geometry** geometries, const char* model_name, const char* base_path);

void AddLamp(Renderer* renderer, vec3 origin, f32 scale, color8 color, f32 brightness);

int main(int argc, char* argv[])
{
   Engine* engine = Engine_Init(
      argc, argv,
      &(EngineDesc){ .app_name = "Game", .window.title = "Game Window" }
   );

   Engine_SetRawMouseInput(engine, true);
   Engine_SetMouseMode(engine, MOUSE_DISABLE_CURSOR);

   Module_Defaults(engine, 0, NULL);

   Graphics* graphics = Engine_FetchModule(engine, GRAPHICS_MODULE);
   Graphics_SetClearColor(graphics, (color8){ 25, 25, 25, 255 });

   Mesh sphere_mesh = Mesh_CreateSphere(12, 1.0f);
   Geometry sphere = Graphics_CreateGeometry(graphics, sphere_mesh, GFX_DRAWMODE_STATIC);
   Mesh_Free(&sphere_mesh);

   Renderer* renderer = Engine_FetchModule(engine, RENDERER_MODULE);
   Renderer_SetLightManager(renderer, DefaultLightManager_Info(renderer));

   Surface unlit_surf = Renderer_AddSurface(renderer, "Unlit", &(SurfaceDesc){
      .pass_count = 1,
      .passes[0] = {
         .shader = Renderer_UnlitShader(renderer),
         .uniform_block_count = 0
      },
      .texture_defaults[0] = RNDR_SURF_TEXTURE_WHITE
   });

   Surface basic_surf = Renderer_AddSurface(renderer, "Basic", &(SurfaceDesc){
      .pass_count = 1,
      .passes[0] = {
         .shader = Renderer_BasicShader(renderer),
         .uniform_block_count = 0
      },
      .texture_defaults[0] = RNDR_SURF_TEXTURE_WHITE,
      .texture_defaults[1] = RNDR_SURF_TEXTURE_NORMAL,
      .texture_defaults[2] = RNDR_SURF_TEXTURE_GRAY,
      .texture_defaults[3] = RNDR_SURF_TEXTURE_BLACK
   });

   Model level_model = LoadModel("assets/models/rocks.ebmf", argv[0]);

   // NOTE: all angles are in half-turns (50 half-turns == 90 degrees, 100 half-turns == 180, etc...)
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
      8,
      0.1f,
      KEY_W,
      KEY_S,
      KEY_A,
      KEY_D
   };

   const char* level_model_textures[4][3] = {
      {
         "assets/textures/rock0_albedo.png",
         "assets/textures/rock0_normal.png",
         "assets/textures/rock0_roughness.png"
      },
      {
         "assets/textures/rock1_albedo.png",
         "assets/textures/rock1_normal.png",
         "assets/textures/rock1_roughness.png"
      },
      {
         "assets/textures/rock2_albedo.png",
         "assets/textures/rock2_normal.png",
         "assets/textures/rock2_roughness.png"
      },
      {
         "assets/textures/grass.png",
         NULL,
         "assets/textures/grass_r.png"
      }
   };

   for (u32 mesh_i = 0; mesh_i < level_model.mesh_count; mesh_i++)
   {
      Drawable mesh_object = Renderer_CreateDrawable(renderer, GEOMETRY_DRAWABLE_TYPE);
      GeometryDrawable* mesh_data = Renderer_DrawableData(renderer, mesh_object);
      mesh_data->geometry = Graphics_CreateGeometry(graphics, level_model.meshes[mesh_i], GFX_DRAWMODE_STATIC);
      mesh_data->color.hex = 0xFF808080;
      if (level_model.meshes[mesh_i].node_id >= 0)
         mesh_data->transform = level_model.nodes[level_model.meshes[mesh_i].node_id].transform;
      else
         mesh_data->transform = Util_IdentityTransform();
      
      mesh_data->transform.origin.y -= 0.5f;

      mesh_data->material.surface = basic_surf;
      
      for (u32 tex_i = 0; tex_i < 3; tex_i++)
      {
         if (level_model_textures[mesh_i][tex_i] == NULL)
            continue;

         Texture model_texture = Renderer_LoadTexture(renderer, level_model_textures[mesh_i][tex_i], (resolution2d){ 0 }, true, (tex_i == 0));
         Renderer_SetSurfaceMaterialTexture(&mesh_data->material,-1, (i32)tex_i, model_texture);

      }

   }

   Model_Free(&level_model);

   LampInfo.geometries = NEW_ARRAY(Geometry);
   CreateModelGeometry(graphics, &LampInfo.geometries, "assets/models/floor_torch.ebmf", argv[0]);
   LampInfo.materials = NEW_ARRAY_N(SurfaceMaterial, Util_ArrayLength(LampInfo.geometries));
   LampInfo.materials[0].surface = basic_surf;
   LampInfo.materials[1].surface = unlit_surf;

   for (i32 y =-1; y <= 1; y++)
      for (i32 x =-1; x <= 1; x++)
   {
      f32 x_f = (f32)x * 10.0f;
      f32 y_f = (f32)y * 10.0f;

      AddLamp(renderer, VEC3(-3 + x_f,-1,-3 + y_f), 1.0f, Util_IntToColor(0xFF8080FF), 1.0f);
      AddLamp(renderer, VEC3(-1 + x_f,-1,-3 + y_f), 0.6f, Util_IntToColor(0xFF8080FF), 0.6f);
      AddLamp(renderer, VEC3( 1 + x_f,-1,-3 + y_f), 0.3f, Util_IntToColor(0xFF8080FF), 0.3f);
      AddLamp(renderer, VEC3( 3 + x_f,-1,-3 + y_f), 1.2f, Util_IntToColor(0xFF8080FF), 1.2f);

      AddLamp(renderer, VEC3(-3 + x_f,-1,-1 + y_f), 0.3f, Util_IntToColor(0xFFFF00FF), 0.3f);
      AddLamp(renderer, VEC3(-1 + x_f,-1,-1 + y_f), 1.2f, Util_IntToColor(0xFFFF00FF), 1.2f);
      AddLamp(renderer, VEC3( 1 + x_f,-1,-1 + y_f), 1.0f, Util_IntToColor(0xFFFF00FF), 1.0f);
      AddLamp(renderer, VEC3( 3 + x_f,-1,-1 + y_f), 0.6f, Util_IntToColor(0xFFFF00FF), 0.6f);

      AddLamp(renderer, VEC3(-3 + x_f,-1, 1 + y_f), 0.5f, Util_IntToColor(0xFFFFFFFF), 0.5f);
      AddLamp(renderer, VEC3(-1 + x_f,-1, 1 + y_f), 1.5f, Util_IntToColor(0xFFFFFFFF), 0.5f);
      AddLamp(renderer, VEC3( 1 + x_f,-1, 1 + y_f), 2.5f, Util_IntToColor(0xFFFFFFFF), 0.5f);
      AddLamp(renderer, VEC3( 3 + x_f,-1, 1 + y_f), 3.5f, Util_IntToColor(0xFFFFFFFF), 0.5f);

      AddLamp(renderer, VEC3(-3 + x_f,-1, 3 + y_f), 1.0f, Util_IntToColor(0x1616FFFF), 0.5f);
      AddLamp(renderer, VEC3(-1 + x_f,-1, 3 + y_f), 0.5f, Util_IntToColor(0x1616FFFF), 0.5f);
      AddLamp(renderer, VEC3( 1 + x_f,-1, 3 + y_f), 0.25f, Util_IntToColor(0x1616FFFF), 0.5f);
      AddLamp(renderer, VEC3( 3 + x_f,-1, 3 + y_f), 0.125f, Util_IntToColor(0x1616FFFF), 0.5f);
   }

   Texture toybox_normal = Renderer_LoadTexture(renderer, "assets/textures/toybox_n.png", (resolution2d){ 0 }, true, false);

   Drawable block_object = Renderer_CreateDrawable(renderer, GEOMETRY_DRAWABLE_TYPE);
   GeometryDrawable* block_data = Renderer_DrawableData(renderer, block_object);
   block_data->geometry = Renderer_BoxGeometry(renderer);
   block_data->color.hex = 0xFF10E0FF;
   block_data->transform.scale = Util_FillVec3(0.5f);
   block_data->transform.rotation = Util_IdentityQuat();
   block_data->transform.origin = VEC3(0.0f);
   block_data->material.surface = basic_surf;
   Renderer_SetSurfaceMaterialTexture(&block_data->material,-1, 1, toybox_normal);
   Renderer_SetSurfaceMaterialTexture(&block_data->material,-1, 3, Renderer_WhiteTexture(renderer));

   Model barrel_model = LoadModel("assets/models/barrel.ebmf", argv[0]);

   Drawable barrel_object = Renderer_CreateDrawable(renderer, GEOMETRY_DRAWABLE_TYPE);
   GeometryDrawable* barrel_data = Renderer_DrawableData(renderer, barrel_object);
   barrel_data->geometry = Graphics_CreateGeometry(graphics, barrel_model.meshes[0], GFX_DRAWMODE_STATIC);
   barrel_data->color.hex = 0xFFFFFFFF;
   barrel_data->transform = Util_IdentityTransform();
   barrel_data->transform.origin.z -= 7.0f;
   barrel_data->transform.origin.y -= 0.5f;
   barrel_data->transform.scale = Util_FillVec3(0.5f);
   barrel_data->material.surface = basic_surf;
   Renderer_SetSurfaceMaterialTexture(&barrel_data->material,-1,-1,Renderer_LoadTexture(renderer, "assets/textures/barrel.png", (resolution2d){ 0 }, true, true));
   Renderer_SetSurfaceMaterialTexture(&barrel_data->material,-1,-1,Renderer_LoadTexture(renderer, "assets/textures/barrel_n.png", (resolution2d){ 0 }, true, false));
   Renderer_SetSurfaceMaterialTexture(&barrel_data->material,-1,-1,Renderer_LoadTexture(renderer, "assets/textures/barrel_r.png", (resolution2d){ 0 }, true, false));
   Renderer_SetSurfaceMaterialTexture(&barrel_data->material,-1,-1,Renderer_LoadTexture(renderer, "assets/textures/barrel_m.png", (resolution2d){ 0 }, true, false));

   Model_Free(&barrel_model);

   Drawable player_object = Renderer_CreateDrawable(renderer, GEOMETRY_DRAWABLE_TYPE);
   GeometryDrawable* player_data = Renderer_DrawableData(renderer, player_object);
   player_data->geometry = sphere;
   player_data->color.hex = 0xFFFFFFFF;
   player_data->transform.scale = Util_FillVec3(0.5f);
   player_data->transform.rotation = Util_IdentityQuat();
   player_data->material.surface = basic_surf;
   Renderer_SetSurfaceMaterialTexture(&player_data->material,-1, 2, Renderer_LoadTexture(renderer, "assets/textures/smooth.png", (resolution2d){ 0 }, true, false));

   Buffer global_ubo = Graphics_CreateBuffer(graphics, NULL, 1, sizeof(f32), GFX_DRAWMODE_DYNAMIC, GFX_BUFFERTYPE_UNIFORM);
   
   f32 global_timer = 0.0f;
   f64 fps_timer = 0.0f;
   u32 frames_rendered = 0;

   f32 cam_dist = 2.0f;

   bool slot_is_reserved = false;

   while(!Engine_CheckExitConditions(engine))
   {
      if (Engine_CheckKey(engine, KEY_ESCAPE, KEY_IS_DOWN))
         Engine_RequestExit(engine);

      if (Engine_CheckKey(engine, KEY_3, KEY_JUST_PRESSED))
      {
         slot_is_reserved = !slot_is_reserved;

         if (slot_is_reserved)
         {
            Renderer_ReserveTexture(renderer, 0);
            Graphics_BindTexture(graphics, Renderer_BlackTexture(renderer), 0);

         }
         else
            Renderer_UnreserveTexture(renderer, 0);

      }

      f64 frame_delta = Engine_GetFrameDelta(engine);
      vec2 mouse_delta = Engine_GetMouseDelta(engine);

      vec2 scroll_delta = Engine_GetMouseScroll(engine);
      cam_dist = cam_dist - (scroll_delta.y * 0.25f);
      cam_dist = M_CLAMP(cam_dist, 0.0f, 5.0f);

      block_data->transform.rotation = Util_MulQuat(Util_MakeQuatEuler(VEC3(0, (f32)frame_delta * 10.0f, 0)), block_data->transform.rotation);

      global_timer += (f32)frame_delta;
      Graphics_UpdateBuffer(graphics, global_ubo, &global_timer, 1, sizeof(f32));

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
            Util_ScaleVec3(move_vec, player.move_speed * (f32)frame_delta)
         );

         player_data->transform.origin = player.origin;

      }

      resolution2d size = Engine_GetFrameSize(engine);
      Graphics_Viewport(graphics, size);

      Graphics_UseBuffer(graphics, global_ubo, 0);

      Renderer_UpdateCamera(renderer, player.origin, player.euler, cam_dist);
      Renderer_Render(renderer, size);
      
      Engine_Present(engine);

      fps_timer += frame_delta;

      frames_rendered++;
      if (fps_timer >= 1.0f)
      {
         printf("%u fps\n", frames_rendered);
         frames_rendered = 0;
         fps_timer -= 1.0f;
      }

   }

   FREE_ARRAY(LampInfo.materials);
   FREE_ARRAY(LampInfo.geometries);

   Engine_Free(engine);
   return 0;
}

Model LoadModel(const char* model_name, const char* base_path)
{
   char* model_path = Util_MakeFilePath(base_path, model_name);
   memblob model_memory = Util_LoadFileIntoMemory(model_path, true);
   Model model = Mesh_LoadEctorModel(model_memory);
   free(model_path);
   free(model_memory.data);

   return model;
}

void CreateModelGeometry(Graphics* graphics, Geometry** geometries, const char* model_name, const char* base_path)
{
   Model model = LoadModel(model_name, base_path);

   for (u32 mesh_i = 0; mesh_i < model.mesh_count; mesh_i++)
   {
      u32 index = Util_ArrayLength(*geometries);

      Geometry geometry = Graphics_CreateGeometry(graphics, model.meshes[mesh_i], GFX_DRAWMODE_STATIC);
      Util_InsertArrayIndex((void**)geometries, index);
      (*geometries)[index] = geometry;
      
   }

   Model_Free(&model);
}

void AddLamp(Renderer* renderer, vec3 origin, f32 scale, color8 color, f32 brightness)
{
   // for (u32 object_i = 0; object_i < Util_ArrayLength(LampInfo.geometries); object_i++)
   // {
   //    Drawable lamp_object = Renderer_CreateDrawable(renderer, GEOMETRY_DRAWABLE_TYPE);
   //    GeometryDrawable* lamp_data = Renderer_DrawableData(renderer, lamp_object);
   //    lamp_data->geometry = LampInfo.geometries[object_i];
   //    lamp_data->color.hex = (object_i != 1) ? 0xFFFFFFFF : color.hex;
   //    lamp_data->transform = Util_IdentityTransform();
   //    lamp_data->transform.origin = origin;
   //    lamp_data->transform.scale = Util_FillVec3(3.0f * scale);
   //    lamp_data->material = LampInfo.materials[object_i];
   // }

   Drawable light1 = DefaultLightManager_CreateLight(renderer);

   DefaultLightManager_SetLightOrigin(renderer, light1, Util_AddVec3(origin, VEC3(0, 2.0f * scale, 0)));
   DefaultLightManager_SetLightRadius(renderer, light1, 2.5f * scale);
   DefaultLightManager_SetLightSpotlightAngle(renderer, light1, 60.0f);
   DefaultLightManager_SetLightSpotlightSoftness(renderer, light1, 0.4f);
   DefaultLightManager_SetLightColor(renderer, light1, color);
   DefaultLightManager_SetLightBrightness(renderer, light1, brightness);
   DefaultLightManager_SetLightAngles(renderer, light1, 0.0f, 0.0f);

   Drawable light2 = DefaultLightManager_CreateLight(renderer);

   DefaultLightManager_SetLightOrigin(renderer, light2, Util_AddVec3(origin, VEC3(0, 1.8f * scale, 0)));
   DefaultLightManager_SetLightRadius(renderer, light2, 4.0f * scale);
   DefaultLightManager_SetLightSpotlightAngle(renderer, light2, 200.0f);
   DefaultLightManager_SetLightColor(renderer, light2, color);
   DefaultLightManager_SetLightBrightness(renderer, light2, brightness * 0.6f);
   DefaultLightManager_SetLightAngles(renderer, light2, 100.0f, 0.0f);

}
