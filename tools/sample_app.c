#include <util/types.h>
#include <util/math.h>
#include <core/keymap.h>
#include <core/renderer.h>
#include <core/engine.h>

int main(int argc, char* argv[])
{
   Engine* engine = Engine_Init(
      &(EngineDesc){ 0 },
      &(RendererDesc){
         .color_format = PIX_RGBA_8BPC,
         .depth_format = PIX_DEPTH_STENCIL,
         .samples = 1,
         .render_scale = { 0.5f, 0.5f }
      }
   );

   struct {
      mat4x4 viewproj;
      vec3 origin;
      vec3 euler;
      f32 fov;
      f32 aspect;
   } camera = { 0 };

   camera.origin = VEC3(0, 0.5f, 4);
   camera.euler = VEC3(0);
   camera.fov = 25.0f;
   camera.aspect = 1.0f;
   camera.viewproj = MAT4(0);

   f32 vrt[] = {
       0,-1, 0,   0,-1, 0,   0.5f, 0.5f, 1.0f,
      -1, 0, 0,  -1, 0, 0,   1.0f, 0.5f, 0.5f,
       0, 0, 1,   0, 0, 1,   0.5f, 1.0f, 0.5f,
       0, 1, 0,   0, 1, 0,   0.5f, 0.5f, 0.0f,
       1, 0, 0,   1, 0, 0,   0.0f, 0.5f, 0.5f,
       0, 0,-1,   0, 0,-1,   0.5f, 0.0f, 0.5f
   };

   u16 idx[] = {
      0, 1, 2,
      0, 2, 4,
      0, 4, 5,
      0, 5, 1,
      3, 2, 1,
      3, 4, 2,
      3, 5, 4,
      3, 1, 5
   };

   Geometry geom = Renderer_CreateGeometry(
      (Buffer){ .data = (void*)vrt, .count = 6, .size = sizeof(f32) * 9u },
      (Buffer){ .data = (void*)idx, .count = 24, .size = sizeof(u16) },
      (VertexDesc){
         .count = 3,
         .index_type = IDX_U16,
         .attribute = {
            [0] = { .format = ATR_F32_V3 },
            [1] = { .format = ATR_F32_V3, .offset = sizeof(f32) * 3u },
            [2] = { .format = ATR_F32_V3, .offset = sizeof(f32) * 6u },
         }
      }
   );

   const char* v_shdsrc =
      "#version 330 core\n"
      "layout(location=0) in vec3 vrt_position;\n"
      "layout(location=1) in vec3 vrt_normal;\n"
      "layout(location=2) in vec3 vrt_color;\n"
      "uniform mat4 u_mvp;\n"
      "out vec3 v2f_normal;\n"
      "out vec3 v2f_color;\n"
      "void main()\n"
      "{\n"
      "   v2f_color = vrt_color;\n"
      "   v2f_normal = vrt_normal;\n"
      "   gl_Position = u_mvp * vec4(vrt_position, 1.0);\n"
      "}\n";

   const char* f_shdsrc =
      "#version 330 core\n"
      "in vec3 v2f_normal;\n"
      "in vec3 v2f_color;\n"
      "out vec4 frg_color;\n"
      "void main()\n"
      "{\n"
      "   vec3 normal = normalize(v2f_normal);\n"
      "   float NdL = dot(normal, vec3(0.707)) * 0.5 + 0.5;\n"
      "   frg_color.rgb = v2f_color * NdL;\n"
      "   frg_color.a = 1.0;\n"
      "}\n";

   Shader shader = Renderer_CreateShader((ShaderDesc){
      .stage_src = {
         [SHD_VERTEX] = v_shdsrc,
         [SHD_FRAGMENT] = f_shdsrc,
      }
   });

   Engine_SetMouseMode(engine, MOUSE_DISABLE_CURSOR);

   mat4x4 xform = Util_MulMat4(
      Util_TranslationMatrix(VEC3(5, 0.5f, 1)),
      Util_ScalingMatrix(VEC3(2, 2, 2))
   );

   while(!Engine_ShouldQuit(engine))
   {
      if (Engine_CheckKey(engine, KEY_ESCAPE, KEY_IS_DOWN))
         Engine_Quit(engine);

      Renderer* renderer = Engine_GetRenderer(engine);

      size2i size = Engine_GetSize(engine);
      camera.aspect = (f32)size.height / (f32)size.width;

      vec2 mouse_delta = Engine_GetMouseDelta(engine);
      camera.euler.y -= mouse_delta.x * 0.02f;
      camera.euler.x -= mouse_delta.y * 0.02f;

      vec3 move_dir = { 0 };

      {
         f32 cos_yaw = M_COS(camera.euler.y);
         f32 sin_yaw = M_SIN(camera.euler.y);

         if (Engine_CheckKey(engine, KEY_W, KEY_IS_DOWN))
            move_dir = Util_SubVec3(move_dir, VEC3( sin_yaw, 0, cos_yaw));
         if (Engine_CheckKey(engine, KEY_S, KEY_IS_DOWN))
            move_dir = Util_AddVec3(move_dir, VEC3( sin_yaw, 0, cos_yaw));
         if (Engine_CheckKey(engine, KEY_A, KEY_IS_DOWN))
            move_dir = Util_AddVec3(move_dir, VEC3(-cos_yaw, 0, sin_yaw));
         if (Engine_CheckKey(engine, KEY_D, KEY_IS_DOWN))
            move_dir = Util_AddVec3(move_dir, VEC3( cos_yaw, 0,-sin_yaw));
      }

      move_dir = Util_NormalizeVec3(move_dir);
      camera.origin = Util_AddVec3(camera.origin, Util_ScaleVec3(move_dir, 0.08f));

      camera.viewproj = Util_MulMat4(
         Util_PerspectiveMatrix(camera.fov, camera.aspect, 0.25f, 100.0f),
         Util_ViewMatrix(camera.origin, camera.euler, 0)
      );

      Renderer_SetFrameSize(renderer, size);
      Renderer_SetViewProjMatrix(renderer, camera.viewproj);

      Renderer_SubmitDrawCall(renderer, (DrawCall){
         .uniform_count = 1,
         .texture_count = 0,
         .uniforms = {
            [0] = { .name = "u_mvp", .type = UNI_MATRIX, .datablob = DATABLOB(camera.viewproj.arr) }
         },
         .shader = shader,
         .geometry = geom
      });

      Renderer_SubmitDrawCall(renderer, (DrawCall){
         .uniform_count = 1,
         .texture_count = 0,
         .uniforms = {
            [0] = { .name = "u_mvp", .type = UNI_MATRIX, .datablob = DATABLOB(Util_MulMat4(camera.viewproj, xform).arr) }
         },
         .shader = shader,
         .geometry = geom
      });

      Engine_Render(engine, (vec4){ 0.1f, 0.15f, 0.2f, 1.0f }, (ClearTargets){ .color = true, .depth = true });
   }

   Renderer_FreeShader(&shader);
   Renderer_FreeGeometry(&geom);
   Engine_Free(engine);
   return 0;
}
