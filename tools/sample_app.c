#include <ect_types.h>
#include <ect_math.h>
#include <core/keymap.h>
#include <core/renderer.h>
#include <core/engine.h>

#include <glad/gl.h>

#include <stdio.h>

int main(int argc, char* argv[])
{
   EctEngine* engine = EctInit(&(EctEngineDesc){ 0 });

   glEnable(GL_DEPTH_TEST);

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

   f32 vrt[][6] = {
      { 0,-1, 0,  0.5f, 0.5f, 1.0f },
      {-1, 0, 0,  1.0f, 0.5f, 0.5f },
      { 0, 0, 1,  0.5f, 1.0f, 0.5f },
      { 0, 1, 0,  0.5f, 0.5f, 0.0f },
      { 1, 0, 0,  0.0f, 0.5f, 0.5f },
      { 0, 0,-1,  0.5f, 0.0f, 0.5f }
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

   EctGeometry geom = EctRendererCreateGeometry(
      (EctBuffer){ .data = (void*)vrt, .count = 8, .size = sizeof(f32) * 6u },
      (EctBuffer){ .data = (void*)idx, .count = 24, .size = sizeof(u16) },
      (EctVertexDesc){
         .count = 2,
         .index_type = ECT_IDX_U16,
         .attribute = {
            [0] = { .type = ECT_VRT_F32_V3 },
            [1] = { .type = ECT_VRT_F32_V3, .offset = sizeof(f32) * 3u },
         }
      }
   );

   const char* v_shdcode =
      "#version 330 core\n"
      "layout(location=0) in vec3 a_position;\n"
      "layout(location=1) in vec3 a_color;\n"
      "uniform mat4 u_mvp;\n"
      "out vec3 v_color;\n"
      "out vec3 v_pos;\n"
      "void main()\n"
      "{\n"
      "   v_pos = a_position;\n"
      "   v_color = a_color;\n"
      "   gl_Position = u_mvp * vec4(a_position, 1.0);\n"
      "}\n";

   u32 v_shd = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(v_shd, 1, &v_shdcode, NULL);
   glCompileShader(v_shd);

   const char* f_shdcode =
      "#version 330 core\n"
      "in vec3 v_color;\n"
      "in vec3 v_pos;\n"
      "out vec4 f_color;\n"
      "void main()\n"
      "{\n"
      "   vec3 n = normalize(cross(dFdx(v_pos), dFdy(v_pos)));\n"
      "   f_color.rgb = v_color * (0.5 * dot(n, vec3(0.707)) + 0.5);\n"
      "   f_color.a = 1.0;\n"
      "}\n";

   u32 f_shd = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(f_shd, 1, &f_shdcode, NULL);
   glCompileShader(f_shd);

   u32 shader = glCreateProgram();
   glAttachShader(shader, v_shd);
   glAttachShader(shader, f_shd);
   glLinkProgram(shader);

   i32 shd_success = 0;
   glGetProgramiv(shader, GL_LINK_STATUS, &shd_success);
   if (!shd_success)
   {
      char shd_log[1024];
      glGetProgramInfoLog(shader, 1024, NULL, shd_log);
      fprintf(stderr, "%s\n", shd_log);
   }

   glDeleteShader(v_shd);
   glDeleteShader(f_shd);

   // EctSetMouseMode(ECT_MOUSE_DISABLE_CURSOR);

   frame size = EctGetSize();
   glViewport(0, 0, size.width, size.height);
   while(!EctShouldQuit(engine))
   {
      if (EctCheckKey(ECT_KEY_ESCAPE, ECT_KEY_IS_DOWN, false))
         EctQuit(engine);

      size = EctGetSize();
      camera.aspect = (f32)size.height / (f32)size.width;

      vec2 mouse_delta = EctGetMouseDelta();
      camera.euler.y -= mouse_delta.x * 0.02f;
      camera.euler.x -= mouse_delta.y * 0.02f;

      vec3 move_dir = { 0 };

      {
         f32 cos_yaw = ECT_COS(camera.euler.y);
         f32 sin_yaw = ECT_SIN(camera.euler.y);

         if (EctCheckKey(ECT_KEY_W, ECT_KEY_IS_DOWN, false))
            move_dir = EctSubVec3(move_dir, VEC3( sin_yaw, 0, cos_yaw));
         if (EctCheckKey(ECT_KEY_S, ECT_KEY_IS_DOWN, false))
            move_dir = EctAddVec3(move_dir, VEC3( sin_yaw, 0, cos_yaw));
         if (EctCheckKey(ECT_KEY_A, ECT_KEY_IS_DOWN, false))
            move_dir = EctAddVec3(move_dir, VEC3(-cos_yaw, 0, sin_yaw));
         if (EctCheckKey(ECT_KEY_D, ECT_KEY_IS_DOWN, false))
            move_dir = EctAddVec3(move_dir, VEC3( cos_yaw, 0,-sin_yaw));
      }

      move_dir = EctNormalizeVec3(move_dir);
      camera.origin = EctAddVec3(camera.origin, EctScaleVec3(move_dir, 0.08f));

      camera.viewproj = EctMulMat4(
         EctPerspectiveMatrix(camera.fov, camera.aspect, 0.25f, 100.0f),
         EctViewMatrix(camera.origin, camera.euler, 0)
      );

      glClearColor(0.1f, 0.15f, 0.2f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glUseProgram(shader);

      u32 mvp_loc = glGetUniformLocation(shader, "u_mvp");
      glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, camera.viewproj.arr);
      EctRendererDrawGeometry(geom);

      glUseProgram(0);
   }

   glDeleteShader(shader);

   EctRendererFreeGeometry(&geom);
   EctFree(engine);
   return 0;
}
