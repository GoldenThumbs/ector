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

#include <stdio.h>

#define ANISTROPIC_FILTERING_LEVEL 8

typedef struct DemoPlayer_t
{
   vec3 origin;
   vec3 euler;
   vec3 velocity;

   f32 move_speed;
   f32 look_speed;

   Key key_forward;
   Key key_backward;
   Key key_left;
   Key key_right;

} DemoPlayer;

typedef struct DemoCamera_t
{
   f32 fov;
   f32 near_clip;
   f32 far_clip;

   vec3 origin;
   f32 distance;
   vec3 origin_delta;
   f32 distance_delta;

   f32 zoom_speed;
   f32 zoom_fac;
   f32 move_fac;

   f32 zoom_min;
   f32 zoom_max;
   f32 move_max;

} DemoCamera;

void MovePlayer(Engine* engine, DemoPlayer* player, Transform3D* transform);
void CreateScene(Renderer* renderer, Surface scene_surface);
void CreateLampGrid(Renderer* renderer);

int main(int argc, char* argv[])
{
   Engine* engine = Engine_Init(
      argc, argv,
      &(EngineDesc){
         .app_name = "Lighting Test",
         .window = {
            .title = "Ector Sample - Lighting Test"
         }
   });

   Engine_SetRawMouseInput(engine, true);
   Engine_SetMouseMode(engine, MOUSE_DISABLE_CURSOR);

   Module_Defaults(engine, 0, NULL);

   Graphics* graphics = Engine_FetchModule(engine, GRAPHICS_MODULE);
   Renderer* renderer = Engine_FetchModule(engine, RENDERER_MODULE);

   Graphics_SetClearColor(graphics, (color8){ 25, 25, 25, 255 });
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

   Model barrel_model = Renderer_LoadModel(renderer, "assets/models/barrel.ebmf");
   Model ball_model = Renderer_LoadModel(renderer, "assets/models/pball_11.ebmf");

   TextureInterpolation texture_interp = {
      .texture_anisotropy = ANISTROPIC_FILTERING_LEVEL,
      .texture_wrap = GFX_TEXTUREWRAP_REPEAT,
      .texture_filter = GFX_TEXTUREFILTER_BILINEAR_LINEAR_MIPMAPS
   };

   Texture toybox_normal = Renderer_LoadTexture(renderer, "assets/textures/toybox_n.png", (res2D){ 0 }, true, false);
   Texture barrel_albedo = Renderer_LoadTexture(renderer, "assets/textures/barrel.png", (res2D){ 0 }, true, true);
   Texture barrel_normal = Renderer_LoadTexture(renderer, "assets/textures/barrel_n.png", (res2D){ 0 }, true, false);
   Texture barrel_roughness = Renderer_LoadTexture(renderer, "assets/textures/barrel_r.png", (res2D){ 0 }, true, false);
   Texture barrel_metalness = Renderer_LoadTexture(renderer, "assets/textures/barrel_m.png", (res2D){ 0 }, true, false);
   Texture ball_albedo = Renderer_LoadTexture(renderer, "assets/textures/pball_11.png", (res2D){ 0 }, true, true);
   Texture ball_roughness = Renderer_LoadTexture(renderer, "assets/textures/pball_11_r.png", (res2D){ 0 }, true, false);

   // NOTE: all angles are in half-turns (50 half-turns == 90 degrees, 100 half-turns == 180, etc...)
   DemoPlayer player = { 0 };
   player.origin = VEC3(0, 0, 3);
   player.euler = VEC3(0, 0, 0);
   player.move_speed = 8.0f;
   player.look_speed = 0.1f;
   player.key_forward = KEY_W;
   player.key_backward = KEY_S;
   player.key_left = KEY_A;
   player.key_right = KEY_D;

   DemoCamera camera = { 0 };
   camera.fov = 40.0f;
   camera.near_clip = 0.05f;
   camera.far_clip = 100.0f;
   camera.distance = 2.0f;
   camera.zoom_speed = 0.25f;
   camera.zoom_min = 1.0f;
   camera.zoom_max = 6.0f;
   camera.move_max = 0.75f;
   camera.zoom_fac = 2.0f;
   camera.move_fac = 5.0f;

   Drawable block_object = Renderer_CreateDrawable(renderer, GEOMETRY_DRAWABLE_TYPE);
   GeometryDrawable* block_data = Renderer_DrawableData(renderer, block_object);
   
   block_data->geometry = Renderer_BoxGeometry(renderer);
   block_data->color = Util_IntToColor(0xFFE010FF);
   block_data->transform.scale = Util_FillVec3(0.5f);
   block_data->transform.rotation = Util_IdentityQuat();

   block_data->material.surface = basic_surf;
   Renderer_SetSurfaceMaterialTextureAdvanced(&block_data->material,-1, 1, toybox_normal, texture_interp);
   Renderer_SetSurfaceMaterialTexture(&block_data->material,-1, 3, Renderer_WhiteTexture(renderer));

   Drawable barrel_object = Renderer_CreateDrawable(renderer, GEOMETRY_DRAWABLE_TYPE);
   GeometryDrawable* barrel_data = Renderer_DrawableData(renderer, barrel_object);

   barrel_data->geometry = Graphics_CreateGeometry(graphics, barrel_model.meshes[0], GFX_DRAWMODE_STATIC);
   barrel_data->transform.origin.z -= 7.0f;
   barrel_data->transform.origin.y -= 0.5f;
   barrel_data->transform.scale = Util_FillVec3(0.5f);

   barrel_data->material.surface = basic_surf;
   Renderer_SetSurfaceMaterialTextureAdvanced(&barrel_data->material,-1, 0, barrel_albedo, texture_interp);
   Renderer_SetSurfaceMaterialTextureAdvanced(&barrel_data->material,-1, 1, barrel_normal, texture_interp);
   Renderer_SetSurfaceMaterialTextureAdvanced(&barrel_data->material,-1, 2, barrel_roughness, texture_interp);
   Renderer_SetSurfaceMaterialTextureAdvanced(&barrel_data->material,-1, 3, barrel_metalness, texture_interp);

   Drawable ball_object = Renderer_CreateDrawable(renderer, GEOMETRY_DRAWABLE_TYPE);
   GeometryDrawable* ball_data = Renderer_DrawableData(renderer, ball_object);

   ball_data->geometry = Graphics_CreateGeometry(graphics, ball_model.meshes[0], GFX_DRAWMODE_STATIC);;
   ball_data->transform.scale = Util_FillVec3(0.5f);
   ball_data->transform.rotation = Util_IdentityQuat();

   ball_data->material.surface = basic_surf;
   Renderer_SetSurfaceMaterialTextureAdvanced(&ball_data->material,-1, 0, ball_albedo, texture_interp);
   Renderer_SetSurfaceMaterialTextureAdvanced(&ball_data->material,-1, 2, ball_roughness, texture_interp);

   f64 fps_timer = 0.0f;
   u32 frames_rendered = 0;
   f32 global_timer = 0.0f;

   Buffer global_ubo = Graphics_CreateBuffer(graphics, NULL, 1, sizeof(f32), GFX_DRAWMODE_DYNAMIC, GFX_BUFFERTYPE_UNIFORM);

   CreateLampGrid(renderer);
   CreateScene(renderer, basic_surf);

   camera.origin = player.origin;
   camera.origin.y += 0.25f;

   Renderer_SetFieldOfView(renderer, camera.fov);
   Renderer_SetClippingPlanes(renderer, camera.near_clip, camera.far_clip);
   while(!Engine_CheckExitConditions(engine))
   {
      if (Engine_CheckKey(engine, KEY_ESCAPE, KEY_IS_DOWN))
         Engine_RequestExit(engine);

      res2D size = Engine_GetFrameSize(engine);
      f64 frame_delta = Engine_GetFrameDelta(engine);
      global_timer += (f32)frame_delta;

      quat block_frame_rotation = Util_MakeQuatEuler(VEC3(0, (f32)frame_delta * 10.0f, 0));
      block_data->transform.rotation = Util_MulQuat(block_data->transform.rotation, block_frame_rotation);

      vec2 scroll_delta = Engine_GetMouseScroll(engine);
      f32 desired_cam_dist_delta = -scroll_delta.y * camera.zoom_speed;

      f32 zoom_lerp_fac = (f32)frame_delta * camera.zoom_fac;
      f32 inv_zoom_lerp_fac = 1.0f - zoom_lerp_fac;
      camera.distance_delta = camera.distance_delta * inv_zoom_lerp_fac + desired_cam_dist_delta * zoom_lerp_fac;
      camera.distance_delta = M_CLAMP(camera.distance_delta, -camera.zoom_speed, camera.zoom_speed);

      camera.distance = M_CLAMP( camera.distance + camera.distance_delta, camera.zoom_min, camera.zoom_max);

      MovePlayer(engine, &player, &ball_data->transform);

      f32 move_lerp_fac = (f32)frame_delta * camera.move_fac;
      f32 inv_move_lerp_fac = 1.0f - move_lerp_fac;

      vec3 desired_cam_origin = player.origin;
      desired_cam_origin.y += 0.25f;

      vec3 camera_velocity = Util_SubVec3(camera.origin, desired_cam_origin);
      vec3 camera_move_dir = Util_NormalizeVec3(camera_velocity);
      f32 camera_speed = Util_DotVec3(camera_velocity, camera_move_dir);
      camera_speed = M_CLAMP(camera_speed, 0.0f, camera.move_max);
      camera.origin = Util_AddVec3(desired_cam_origin, Util_ScaleVec3(camera_move_dir, camera_speed * inv_move_lerp_fac));

      Graphics_Viewport(graphics, size);
      Graphics_Clear(graphics);
      Graphics_UpdateBuffer(graphics, global_ubo, &global_timer, 1, sizeof(f32));
      Graphics_BindBuffer(graphics, global_ubo, 0);

      Renderer_UpdateCamera(renderer, camera.origin, player.euler, camera.distance);
      Renderer_RenderPass(renderer, size, frame_delta, 0);
      
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

   Model_Free(&barrel_model);
   Model_Free(&ball_model);

   Engine_Free(engine);

   return 0;
}

void MovePlayer(Engine* engine, DemoPlayer* player, Transform3D* transform)
{
   f32 frame_delta = (f32)Engine_GetFrameDelta(engine);
   vec2 mouse_delta = Engine_GetMouseDelta(engine);

   player->euler.y -= mouse_delta.x * player->look_speed;
   player->euler.y += (player->euler.y > 200) ?-200 : ((player->euler.y < 0) ? 200 : 0);

   player->euler.x -= mouse_delta.y * player->look_speed;
   player->euler.x = M_MAX(-50, M_MIN( 50, player->euler.x));

   f32 yaw_sin = M_SIN(player->euler.y);
   f32 yaw_cos = M_COS(player->euler.y);

   vec3 move_vec = VEC3(0, 0, 0);

   if (Engine_CheckKey(engine, player->key_forward, KEY_IS_DOWN))
   {
      move_vec = Util_AddVec3(
         move_vec,
         VEC3(-yaw_sin, 0,-yaw_cos)
      );
      
   }

   if (Engine_CheckKey(engine, player->key_backward, KEY_IS_DOWN))
   {
      move_vec = Util_AddVec3(
         move_vec,
         VEC3( yaw_sin, 0, yaw_cos)
      );
      
   }

   if (Engine_CheckKey(engine, player->key_left, KEY_IS_DOWN))
   {
      move_vec = Util_AddVec3(
         move_vec,
         VEC3(-yaw_cos, 0, yaw_sin)
      );
      
   }

   if (Engine_CheckKey(engine, player->key_right, KEY_IS_DOWN))
   {
      move_vec = Util_AddVec3(
         move_vec,
         VEC3( yaw_cos, 0,-yaw_sin)
      );
      
   }

   move_vec = Util_NormalizeVec3(move_vec);
   vec3 desired_velocity = Util_ScaleVec3(move_vec, player->move_speed * frame_delta);

   f32 friction = frame_delta * 2.0f;
   f32 inv_friction = 1.0f - friction;
   player->velocity = Util_AddVec3(Util_ScaleVec3(player->velocity, inv_friction), Util_ScaleVec3(desired_velocity, friction));
   player->origin = Util_AddVec3(player->origin, player->velocity);
   
   vec3 axis_vec = VEC3(-player->velocity.z, 0, player->velocity.x);
   quat frame_rotation = Util_MakeQuat(Util_NormalizeVec3(axis_vec), Util_MagVec3(axis_vec) * 50.0f);
   transform->rotation = Util_MulQuat(transform->rotation, frame_rotation);
   transform->origin = player->origin;

}

void CreateScene(Renderer* renderer, Surface scene_surface)
{
   const char* scene_textures[4][3] = {
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

   TextureInterpolation texture_interp = {
      .texture_anisotropy = ANISTROPIC_FILTERING_LEVEL,
      .texture_wrap = GFX_TEXTUREWRAP_REPEAT,
      .texture_filter = GFX_TEXTUREFILTER_BILINEAR_LINEAR_MIPMAPS
   };

   Model scene_model = Renderer_LoadModel(renderer, "assets/models/rocks.ebmf");
   Graphics* graphics = Renderer_Graphics(renderer);

   for (u32 mesh_i = 0; mesh_i < scene_model.mesh_count; mesh_i++)
   {
      Drawable mesh_object = Renderer_CreateDrawable(renderer, GEOMETRY_DRAWABLE_TYPE);
      GeometryDrawable* mesh_data = Renderer_DrawableData(renderer, mesh_object);

      Mesh mesh = scene_model.meshes[mesh_i];
      mesh_data->geometry = Graphics_CreateGeometry(graphics, mesh, GFX_DRAWMODE_STATIC);
      mesh_data->material.surface = scene_surface;

      for (u32 tex_i = 0; tex_i < 3; tex_i++)
      {
         if (scene_textures[mesh_i][tex_i] == NULL)
            continue;

         Texture model_texture = Renderer_LoadTexture(renderer, scene_textures[mesh_i][tex_i], (res2D){ 0 }, true, (tex_i == 0));
         Renderer_SetSurfaceMaterialTextureAdvanced(&mesh_data->material,-1, (i32)tex_i, model_texture, texture_interp);

      }

      if (mesh.node_id >= 0)
         mesh_data->transform = scene_model.nodes[mesh.node_id].transform;
      else
         mesh_data->transform = Util_IdentityTransform();
      
      mesh_data->transform.origin.y -= 0.5f;

   }

   Model_Free(&scene_model);

}

void AddLamp(Renderer* renderer, vec3 origin, f32 scale, color8 color, f32 brightness)
{
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

void CreateLampGrid(Renderer* renderer)
{
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

}
