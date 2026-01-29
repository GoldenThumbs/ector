#include <util/types.h>
#include <util/extra_types.h>
#include <util/matrix.h>
#include <util/quaternion.h>
#include <util/keymap.h>
#include <util/files.h>
#include <engine.h>
#include <graphics.h>
#include <renderer.h>
#include <default_modules.h>

#include "particle_shader_code.h"

#include <stb_image.h>

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
Texture LoadTexture(Graphics* graphics, const char* texture_name, const char* base_path);
void ParticleRenderFunc(Renderer* renderer, Drawable self, u32 pass_id);

int main(int argc, char* argv[])
{
   Engine* engine = Engine_Init(
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
      }
   });

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

   Drawable floor_object = Renderer_CreateDrawable(renderer, GEOMETRY_DRAWABLE_TYPE);
   GeometryDrawable* floor_data = Renderer_DrawableData(renderer, floor_object);
   floor_data->geometry = Renderer_PlaneGeometry(renderer);
   floor_data->color.hex = 0xFFFFFFFF;
   floor_data->transform.scale = VEC3(10, 1, 10);
   floor_data->transform.rotation = Util_IdentityQuat();
   floor_data->transform.origin.y -= 1.0f;
   floor_data->material.surface = basic_surf;
   floor_data->material.texture_count = 1;
   floor_data->material.textures[0].texture = LoadTexture(graphics, "assets/textures/dirt.png", argv[0]);
   Graphics_SetTextureInterpolation(graphics, floor_data->material.textures[0].texture, (TextureInterpolation){ 1, GFX_TEXTUREFILTER_BILINEAR_LINEAR_MIPMAPS, GFX_TEXTUREWRAP_REPEAT });

   Drawable box_object = Renderer_CreateDrawable(renderer, GEOMETRY_DRAWABLE_TYPE);
   GeometryDrawable* box_data = Renderer_DrawableData(renderer, box_object);
   box_data->geometry = Renderer_BoxGeometry(renderer);
   box_data->color.hex = 0xFF000088;
   box_data->transform.scale = VEC3(0.6f, 0.6f, 0.6f);
   box_data->transform.rotation = Util_IdentityQuat();
   box_data->transform.origin.y += 0.6f;
   box_data->material.surface = basic_surf;
   box_data->material.texture_count = 1;
   box_data->material.textures[0].texture = LoadTexture(graphics, "assets/textures/leaves.png", argv[0]);
   Graphics_SetTextureInterpolation(graphics, box_data->material.textures[0].texture, (TextureInterpolation){ 1, GFX_TEXTUREFILTER_BILINEAR_LINEAR_MIPMAPS, GFX_TEXTUREWRAP_REPEAT });

   Drawable particle_object = Renderer_CreateDrawable(renderer, "ParticleDrawable");
   ParticleDrawable* particle_data = Renderer_DrawableData(renderer, particle_object);
   particle_data->particle_count = 16000;
   particle_data->geometry = Plane(graphics);
   particle_data->color.hex = 0xFFFFFFFF;
   particle_data->transform.scale = VEC3(0.08f, 0.08f, 1.0f);
   particle_data->transform.rotation = Util_MakeQuat(VEC3(0, 1, 0), 25.0f);
   particle_data->transform.origin.y += 1.8f;
   particle_data->shader = Graphics_CreateShader(graphics, particle_vertex_code, particle_fragment_code);
   particle_data->color_texture = LoadTexture(graphics, "assets/textures/bubble.png", argv[0]);
   particle_data->particle_shader = Graphics_CreateComputeShader(graphics, particle_compute_code);
   particle_data->particle_ssbo = Graphics_CreateBuffer(graphics, NULL, particle_data->particle_count, sizeof(mat4x4), GFX_DRAWMODE_STATIC_COPY, GFX_BUFFERTYPE_STORAGE);
   Graphics_SetTextureInterpolation(graphics, particle_data->color_texture, (TextureInterpolation){ 1, GFX_TEXTUREFILTER_BILINEAR_LINEAR_MIPMAPS, GFX_TEXTUREWRAP_CLAMP });

   Buffer global_ubo = Graphics_CreateBuffer(graphics, NULL, 1, sizeof(f32), GFX_DRAWMODE_DYNAMIC, GFX_BUFFERTYPE_UNIFORM);
   
   f32 global_timer = 0.0f;
   f64 fps_timer = 0.0f;
   u32 frames_rendered = 0;

   while(!Engine_CheckExitConditions(engine))
   {
      if (Engine_CheckKey(engine, KEY_ESCAPE, KEY_IS_DOWN))
         Engine_RequestExit(engine);

      f64 frame_delta = Engine_GetFrameDelta(engine);
      vec2 mouse_delta = Engine_GetMouseDelta(engine);

      particle_data->transform.rotation = Util_MulQuat(Util_MakeQuat(VEC3(0, 1, 0), 1.0f * (f32)frame_delta), particle_data->transform.rotation);

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

      resolution2d size = Engine_GetFrameSize(engine);
      Graphics_Viewport(graphics, size);

      Graphics_UseBuffer(graphics, global_ubo, 0);

      Renderer_SetViewAndProjectionMatrix(
         renderer,
         Util_ViewMatrix(player.origin, player.euler, 0.0f),
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

Texture LoadTexture(Graphics* graphics, const char* texture_name, const char* base_path)
{
   if (graphics == NULL || texture_name == NULL || base_path == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   char* texture_path = Util_MakeFilePath(base_path, texture_name);
   if (texture_path == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   TextureDesc desc = { 0 };
   desc.depth = 1;
   desc.mipmap_count = 1;
   desc.texture_type = GFX_TEXTURETYPE_2D;

   resolution2d image_size = { 0 };
   i32 channel_count = 0;

   u8* image = stbi_load(texture_path, &desc.size.width, &desc.size.height, &channel_count, 0);
   desc.texture_format = GFX_TEXTUREFORMAT_R_U8_NORM + (channel_count - 1);

   free(texture_path);
   if (image == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   Texture texture = Graphics_CreateTexture(graphics, image, desc);
   Graphics_GenerateTextureMipmaps(graphics, texture);

   stbi_image_free(image);

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
