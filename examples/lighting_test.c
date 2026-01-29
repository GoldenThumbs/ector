#include "util/extra_types.h"
#include "util/matrix.h"
#include "util/quaternion.h"
#include <string.h>
#include <util/types.h>
#include <util/keymap.h>
#include <engine.h>
#include <graphics.h>
#include <renderer.h>
#include <default_modules.h>

#include "particle_shader_code.h"

#include <stb_image.h>

#include <stdio.h>

void ParticleRenderFunc(Renderer* renderer, Drawable self);
Geometry Plane(Graphics* graphics);

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
void ParticleRenderFunc(Renderer* renderer, Drawable self);

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

   i32 directory_length = strnlen(argv[0], 2048);
   while (directory_length > 0)
   {
      char directory_char = argv[0][--directory_length];
      if (directory_char == '/' || directory_char == '\\')
         break;
   }

   const char* image_name = "assets/textures/bubble.png";
   i32 image_path_length = snprintf(NULL, 0, "%.*s/%s", directory_length, argv[0], image_name) + 1;
   char* image_path = malloc(image_path_length * sizeof(char));
   snprintf(image_path, image_path_length, "%.*s/%s", directory_length, argv[0], image_name);
   printf("%s\n", image_path);

   resolution2d image_size = { 0 };
   int image_channel_count = 0;

   u8* image = stbi_load(image_path, &image_size.width, &image_size.height, &image_channel_count, 4);

   Drawable floor_object = Renderer_CreateDrawable(renderer, GEOMETRY_DRAWABLE_TYPE);
   GeometryDrawable* floor_data = Renderer_DrawableData(renderer, floor_object);
   floor_data->geometry = Renderer_PlaneGeometry(renderer);
   floor_data->color.hex = 0xFF888888;
   floor_data->transform.scale = VEC3(10, 1, 10);
   floor_data->transform.rotation = Util_IdentityQuat();
   floor_data->transform.origin.y -= 1.0f;
   floor_data->material.shader = Renderer_BasicShader(renderer);

   Drawable box_object = Renderer_CreateDrawable(renderer, GEOMETRY_DRAWABLE_TYPE);
   GeometryDrawable* box_data = Renderer_DrawableData(renderer, box_object);
   box_data->geometry = Renderer_BoxGeometry(renderer);
   box_data->color.hex = 0xFF000088;
   box_data->transform.scale = VEC3(0.6f, 0.6f, 0.6f);
   box_data->transform.rotation = Util_IdentityQuat();
   box_data->transform.origin.y -= 0.6f;
   box_data->material.shader = Renderer_BasicShader(renderer);

   Drawable particle_object = Renderer_CreateDrawable(renderer, "ParticleDrawable");
   ParticleDrawable* particle_data = Renderer_DrawableData(renderer, particle_object);
   particle_data->particle_count = 16000;
   particle_data->geometry = Plane(graphics);
   particle_data->color.hex = 0xFFFFFFFF;
   particle_data->transform.scale = VEC3(0.08f, 0.08f, 1.0f);
   particle_data->transform.rotation = Util_MakeQuat(VEC3(0, 1, 0), 25.0f);
   particle_data->transform.origin.y += 1.8f;
   particle_data->shader = Graphics_CreateShader(graphics, particle_vertex_code, particle_fragment_code);
   particle_data->color_texture = Graphics_CreateTexture(graphics, image, (TextureDesc){ image_size, 1, 1, GFX_TEXTURETYPE_2D, GFX_TEXTUREFORMAT_RGBA_U8_NORM });
   particle_data->particle_shader = Graphics_CreateComputeShader(graphics, particle_compute_code);
   particle_data->particle_ssbo = Graphics_CreateBuffer(graphics, NULL, particle_data->particle_count, sizeof(mat4x4), GFX_DRAWMODE_STATIC_COPY, GFX_BUFFERTYPE_STORAGE);

   Graphics_GenerateTextureMipmaps(graphics, particle_data->color_texture);

   Buffer global_ubo = Graphics_CreateBuffer(graphics, NULL, 1, sizeof(f32), GFX_DRAWMODE_DYNAMIC, GFX_BUFFERTYPE_UNIFORM);
   
   f32 global_timer = 0.0f;
   f64 fps_timer = 0.0f;
   u32 frames_rendered = 0;

   stbi_image_free(image);

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

void ParticleRenderFunc(Renderer* renderer, Drawable self)
{
   ParticleDrawable* drawable_data = Renderer_DrawableData(renderer, self);

   Graphics* graphics = Renderer_Graphics(renderer);
   Buffer camera_buffer = Renderer_CameraBuffer(renderer);
   Buffer model_buffer = Renderer_ModelBuffer(renderer);
   mat4x4 view_projection = Renderer_GetViewAndProjectionMatrix(renderer);

   ModelData model_data = { 0 };
   model_data.mat_model = Util_TransformationMatrix(drawable_data->transform);
   model_data.mat_invmodel = Util_InverseMat4(model_data.mat_model);
   model_data.mat_mvp = Util_MulMat4(view_projection, model_data.mat_model);
   model_data.u_color = Util_Vec4FromColor(drawable_data->color);

   mat4x4 mat_normal_model = Util_TransposeMat4(model_data.mat_invmodel);
   model_data.mat_normal_model[0].xyz = mat_normal_model.v[0].xyz;
   model_data.mat_normal_model[1].xyz = mat_normal_model.v[1].xyz;
   model_data.mat_normal_model[2].xyz = mat_normal_model.v[2].xyz;

   Graphics_SetBlending(graphics, GFX_BLENDMODE_ADD);

   Graphics_UpdateBuffer(graphics, model_buffer, &model_data, 1, sizeof(ModelData));
   Graphics_UseBuffer(graphics, model_buffer, 2);

   Graphics_UseBuffer(graphics, camera_buffer, 1);

   Graphics_UseBuffer(graphics, drawable_data->particle_ssbo, 3);

   Graphics_Dispatch(graphics, drawable_data->particle_shader, drawable_data->particle_count, 1, 1, (UniformBlockList){ .count = 0 });
   Graphics_DispatchBarrier();

   Graphics_SetTextureInterpolation(graphics, drawable_data->color_texture, (TextureInterpolation){ 1, GFX_TEXTUREFILTER_BILINEAR_LINEAR_MIPMAPS, GFX_TEXTUREWRAP_CLAMP });
   Graphics_BindTexture(graphics, drawable_data->color_texture, 0);

   Graphics_DrawInstanced(graphics, drawable_data->shader, drawable_data->geometry, drawable_data->particle_count, (UniformBlockList){ .count = 0 });

   Graphics_SetBlending(graphics, GFX_BLENDMODE_NONE);
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
