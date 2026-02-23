#include <util/types.h>
#include <util/extra_types.h>
#include <util/matrix.h>
#include <util/quaternion.h>
#include <util/keymap.h>
#include <util/files.h>
#include <image.h>
#include <engine.h>
#include <graphics.h>
#include <renderer.h>
#include <default_modules.h>

#include "mesh.h"
#include "particle_shader_code.h"
#include "util/math.h"
#include "util/vec3.h"

#include <stdlib.h>
#include <stdio.h>

typedef struct ParticleDrawable_t
{
   Shader shader;
   Shader particle_shader;
   Texture color_texture;
   Geometry geometry;
   Buffer particle_ssbo;
   color8 color;
   Transform3D transform;

   u32 particle_count;

} ParticleDrawable;

Geometry Plane(Graphics* graphics);
Texture LoadTexture(Graphics* graphics, const char* texture_name, const char* base_path, resolution2d slice_size, bool is_srgb);
void ParticleRenderFunc(Renderer* renderer, Drawable self, u32 pass_id);

int main(int argc, char* argv[])
{
   Engine* engine = Engine_Init(
      argc, argv,
      &(EngineDesc){ .app_name = "Game", .window.title = "Game Window" }
   );

   Engine_SetRawMouseInput(engine, true);
   Engine_SetMouseMode(engine, MOUSE_DISABLE_CURSOR);

   Module_Defaults(engine, 0, NULL);

   Graphics* graphics = Engine_FetchModule(engine, "graphics");
   Graphics_SetClearColor(graphics, (color8){ 25, 25, 25, 255 });

   Renderer* renderer = Engine_FetchModule(engine, "renderer");

   Renderer_RegisterDrawableType(renderer, "ParticleDrawable", &(DrawableTypeDesc){
      .data_size = sizeof(ParticleDrawable),
      .render_func = ParticleRenderFunc
   });

   Surface basic_surf = Renderer_AddSurface(renderer, "Basic", &(SurfaceDesc){
      .pass_count = 1,
      .passes[0] = {
         .shader = Renderer_BasicShader(renderer),
         .uniform_block_count = 0
      },
      .texture_defaults[1] = RNDR_SURF_TEXTURE_NORMAL,
      .texture_defaults[2] = RNDR_SURF_TEXTURE_GRAY,
      .texture_defaults[3] = RNDR_SURF_TEXTURE_BLACK
   });

   char* level_model_path = Util_MakeFilePath(argv[0], "assets/models/rocks.ebmf");
   memblob level_model_memory = Util_LoadFileIntoMemory(level_model_path, true);
   Model level_model = Mesh_LoadEctorModel(level_model_memory);
   free(level_model_path);
   free(level_model_memory.data);

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
         "assets/textures/wood.png",
         NULL,
         "assets/textures/wood_r.png"
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

         u32 index = mesh_data->material.texture_count;
         mesh_data->material.texture_count++;

         Texture model_texture = LoadTexture(graphics, level_model_textures[mesh_i][tex_i], argv[0], (resolution2d){ 0 }, (tex_i == 0));
         Graphics_SetTextureInterpolation(graphics, model_texture, (TextureInterpolation){ 1, GFX_TEXTUREFILTER_BILINEAR_LINEAR_MIPMAPS, GFX_TEXTUREWRAP_REPEAT });

         mesh_data->material.textures[index].texture = model_texture;
         mesh_data->material.textures[index].bind_slot = tex_i;

      }

   }

   Model_Free(&level_model);

   Drawable block_object = Renderer_CreateDrawable(renderer, GEOMETRY_DRAWABLE_TYPE);
   GeometryDrawable* block_data = Renderer_DrawableData(renderer, block_object);
   block_data->geometry = Renderer_BoxGeometry(renderer);
   block_data->color.hex = 0xFF10E0FF;
   block_data->transform.scale = VEC3(0.5f, 0.5, 0.5f);
   block_data->transform.rotation = Util_IdentityQuat();
   block_data->transform.origin = VEC3(2.0f, 0.0f, -1.0f);
   block_data->material.surface = basic_surf;
   block_data->material.texture_count = 2;
   block_data->material.textures[0].texture = LoadTexture(graphics, "assets/textures/toybox_n.png", argv[0], (resolution2d){ 0 }, false);
   block_data->material.textures[0].bind_slot = 1;
   block_data->material.textures[1].texture = Renderer_WhiteTexture(renderer);
   block_data->material.textures[1].bind_slot = 3;
   Graphics_SetTextureInterpolation(graphics, block_data->material.textures[0].texture, (TextureInterpolation){ 1, GFX_TEXTUREFILTER_BILINEAR_LINEAR_MIPMAPS, GFX_TEXTUREWRAP_REPEAT });
   Graphics_SetTextureInterpolation(graphics, block_data->material.textures[1].texture, (TextureInterpolation){ 1, GFX_TEXTUREFILTER_BILINEAR_LINEAR_MIPMAPS, GFX_TEXTUREWRAP_REPEAT });

   char* barrel_model_path = Util_MakeFilePath(argv[0], "assets/models/barrel.ebmf");
   memblob barrel_model_memory = Util_LoadFileIntoMemory(barrel_model_path, true);
   Model barrel_model = Mesh_LoadEctorModel(barrel_model_memory);
   free(barrel_model_path);
   free(barrel_model_memory.data);

   Drawable barrel_object = Renderer_CreateDrawable(renderer, GEOMETRY_DRAWABLE_TYPE);
   GeometryDrawable* barrel_data = Renderer_DrawableData(renderer, barrel_object);
   barrel_data->geometry = Graphics_CreateGeometry(graphics, barrel_model.meshes[0], GFX_DRAWMODE_STATIC);
   barrel_data->color.hex = 0xFFFFFFFF;
   barrel_data->transform = Util_IdentityTransform();
   barrel_data->transform.origin.y -= 1.0f;
   barrel_data->transform.scale = Util_FillVec3(0.5f);
   barrel_data->material.surface = basic_surf;
   barrel_data->material.texture_count = 4;
   barrel_data->material.textures[0].texture = LoadTexture(graphics, "assets/textures/barrel.png", argv[0], (resolution2d){ 0 }, true);
   barrel_data->material.textures[1].texture = LoadTexture(graphics, "assets/textures/barrel_n.png", argv[0], (resolution2d){ 0 }, false);
   barrel_data->material.textures[2].texture = LoadTexture(graphics, "assets/textures/barrel_r.png", argv[0], (resolution2d){ 0 }, false);
   barrel_data->material.textures[3].texture = LoadTexture(graphics, "assets/textures/barrel_m.png", argv[0], (resolution2d){ 0 }, false);
   barrel_data->material.textures[1].bind_slot = 1;
   barrel_data->material.textures[2].bind_slot = 2;
   barrel_data->material.textures[3].bind_slot = 3;
   Graphics_SetTextureInterpolation(graphics, barrel_data->material.textures[0].texture, (TextureInterpolation){ 1, GFX_TEXTUREFILTER_BILINEAR_LINEAR_MIPMAPS, GFX_TEXTUREWRAP_REPEAT });
   Graphics_SetTextureInterpolation(graphics, barrel_data->material.textures[1].texture, (TextureInterpolation){ 1, GFX_TEXTUREFILTER_BILINEAR_LINEAR_MIPMAPS, GFX_TEXTUREWRAP_REPEAT });
   Graphics_SetTextureInterpolation(graphics, barrel_data->material.textures[2].texture, (TextureInterpolation){ 1, GFX_TEXTUREFILTER_BILINEAR_LINEAR_MIPMAPS, GFX_TEXTUREWRAP_REPEAT });
   Graphics_SetTextureInterpolation(graphics, barrel_data->material.textures[3].texture, (TextureInterpolation){ 1, GFX_TEXTUREFILTER_BILINEAR_LINEAR_MIPMAPS, GFX_TEXTUREWRAP_REPEAT });

   Model_Free(&barrel_model);

   const u32 spotlight_count = 30;
   const f32 inv_spotlight_count = 1.0f / (f32)(spotlight_count - 1);

   for (u32 j = 0; j < 5; j++)
      for (u32 i = 0; i < spotlight_count; i++)
   {
      f32 x = (i * inv_spotlight_count - 0.5f) * 20.0f;
      f32 z = ((f32)j - 2.0f) * 3.0f;
      Renderer_AddLight(renderer, (LightDesc){
         .origin = VEC3(x, 3, 1),
         .radius = 8.0f,
         .spotlight_angle = 30.0f,
         .color = { .hex = 0xFFFFFFFF },
         .brightness = 1.0f,
         .theta = 0.0f,
         .phi = 0.0f
      });
   }

   u32 light_idx = Renderer_AddLight(renderer, (LightDesc){
      .origin = VEC3(0),
      .radius = 15.0f,
      .spotlight_angle = 200.0f,
      .color = { .hex = 0xFFFFFFFF },
      .brightness = 1.0f,
      .theta = 0.0f,
      .phi = 0.0f
   });

   Buffer global_ubo = Graphics_CreateBuffer(graphics, NULL, 1, sizeof(f32), GFX_DRAWMODE_DYNAMIC, GFX_BUFFERTYPE_UNIFORM);
   
   f32 global_timer = 0.0f;
   f64 fps_timer = 0.0f;
   u32 frames_rendered = 0;

   f32 cam_dist = 2.0f;

   while(!Engine_CheckExitConditions(engine))
   {
      if (Engine_CheckKey(engine, KEY_ESCAPE, KEY_IS_DOWN))
         Engine_RequestExit(engine);

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
      }

      Renderer_UpdateLight(renderer, light_idx, (LightDesc){
         .origin = player.origin,
         .radius = 5.0f,
         .spotlight_angle = 200.0f,
         .color = { .hex = 0xFFFFFFFF },
         .brightness = 2.5f,
         .theta = 0.0f,
         .phi = 0.0f
      });

      resolution2d size = Engine_GetFrameSize(engine);
      Graphics_Viewport(graphics, size);

      Graphics_UseBuffer(graphics, global_ubo, 0);

      Renderer_SetViewAndProjectionMatrix(
         renderer,
         Util_ViewMatrix(player.origin, player.euler, cam_dist),
         Util_PerspectiveMatrix(50.0f, (f32)size.height / (f32)size.width, 0.05f, 100.0f)
      );

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

      for (u32 j = 0; j < 5; j++)
         for (u32 i = 0; i < spotlight_count; i++)
      {
         f32 x = (i * inv_spotlight_count - 0.5f) * 20.0f;
         f32 z = ((f32)j - 2.0f) * 3.0f;
         Renderer_UpdateLight(renderer, j * spotlight_count + i, (LightDesc){
            .origin = VEC3(x, 2.0f + 0.5f * M_SIN((x + z) * 2.0f + global_timer * 15.0f), z),
            .radius = 8.0f,
            .spotlight_angle = M_ABS(M_COS(global_timer * 3.0f + (x + z * 6.0f) * 6.0f)) * 30.0f + 20.0f,
            .color = { .hex = 0xFFFFFFFF },
            .brightness =  M_SIN(global_timer * 60.0f + (x + z * 10.0f) * 10.0f) * 0.25f + 0.25f,
            .theta = 0.0f,
            .phi = -50.0f + M_SIN(global_timer * 10.0f + (x + z * 2.0f) * 10.0f) * -20.0f
         });
      }

   }

   Engine_Free(engine);
   return 0;
}

Geometry Plane(Graphics* graphics)
{
   Mesh mesh = Mesh_EmptyMesh(MESH_PRIMITIVE_TRIANGLE);
   mesh.attribute_count = 3;
   mesh.attributes[0] = MESH_ATTRIBUTE_3_CHANNEL;
   mesh.attributes[1] = MESH_ATTRIBUTE_3_CHANNEL;
   mesh.attributes[2] = MESH_ATTRIBUTE_2_CHANNEL;
   
   MeshInterface mesh_interface = Mesh_NewInterface(&mesh);

   mat4x4 t = Util_MulMat4(Util_RotationMatrix(VEC3(1, 0, 0), 50), Util_ScalingMatrix(VEC3(2, 1, 2)));
   mesh_interface = Mesh_AddQuad(1, 1, t, mesh_interface);
   mesh_interface = Mesh_GenNormals(mesh_interface);
   mesh_interface = Mesh_GenTexcoords(mesh_interface);

   Geometry plane = Graphics_CreateGeometry(graphics, mesh, GFX_DRAWMODE_STATIC);
   Mesh_Free(&mesh);

   return plane;
}

Texture LoadTexture(Graphics* graphics, const char* texture_name, const char* base_path, resolution2d slice_size, bool is_srgb)
{
   if (graphics == NULL || texture_name == NULL || base_path == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   char* texture_path = Util_MakeFilePath(base_path, texture_name);
   if (texture_path == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   memblob memory = Util_LoadFileIntoMemory(texture_path, true);
   Image image = Image_CreateImage(memory, IMG_TYPE_2D, slice_size, is_srgb);
   Image_GenerateMipmaps(&image);
   free(memory.data);

   TextureDesc desc = { 0 };
   desc.size = image.size;
   desc.depth = image.depth;
   desc.mipmap_count = image.mipmap_count;
   desc.texture_type = GFX_TEXTURETYPE_2D;
   if (image.image_format == IMG_FORMAT_U8_SRGB)
      desc.texture_format = (image.channel_count == 4) ? GFX_TEXTUREFORMAT_SRGB_ALPHA : GFX_TEXTUREFORMAT_SRGB;
   else
      desc.texture_format = ((image.image_format == IMG_FORMAT_F32) ? GFX_TEXTUREFORMAT_R_F32 : GFX_TEXTUREFORMAT_R_U8_NORM) + image.channel_count - 1;

   free(texture_path);
   if (image.data == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   Texture texture = Graphics_CreateTexture(graphics, image.data, desc);

   Image_Free(&image);

   return texture;
}

void ParticleRenderFunc(Renderer* renderer, Drawable self, u32 pass_id)
{
   Graphics* graphics = Renderer_Graphics(renderer);
   ParticleDrawable* drawable_data = Renderer_DrawableData(renderer, self);

   Graphics_SetBlending(graphics, GFX_BLENDMODE_MIX);

   Buffer camera_buffer = Renderer_CameraBuffer(renderer);
   Buffer model_buffer = Renderer_ModelBuffer(renderer);
   mat4x4 view_projection = Renderer_GetViewAndProjectionMatrix(renderer);

   Renderer_UpdateModelData(renderer, drawable_data->transform, drawable_data->color);

   Graphics_UseBuffer(graphics, drawable_data->particle_ssbo, 3);
   Graphics_Dispatch(graphics, drawable_data->particle_shader, drawable_data->particle_count, 1, 1, (UniformBlockList){ .count = 0 });
   Graphics_DispatchBarrier();

   Renderer_SetTexture(renderer, drawable_data->color_texture, 0);
   Graphics_DrawInstanced(graphics, drawable_data->shader, drawable_data->geometry, drawable_data->particle_count, (UniformBlockList){ .count = 0 });

   Graphics_SetBlending(graphics, GFX_BLENDMODE_NONE);

}
