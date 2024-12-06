#include <ect_types.h>
#include <ect_math.h>
#include <core/keymap.h>
#include <core/renderer.h>
#include <core/engine.h>

#include <sokol_gfx.h>
#include <sokol_log.h>

int main(int argc, char* argv[])
{
   EctEngine* engine = EctInit(&(EctEngineDesc){ 0 });

   sg_pass_action pass_action = {
      .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = { 0.1f, 0.1f, 0.1f, 1.0f } }
   };

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
      (EctBuffer){ .data = (void*)idx, .count = 24, .size = sizeof(u16) }
   );

   sg_shader shd = sg_make_shader(&(sg_shader_desc){
      .vertex_func.source =
         "#version 410\n"
         "uniform mat4 u_mvp;\n"
         "layout(location=0) in vec4 a_position;\n"
         "layout(location=1) in vec4 a_color;\n"
         "out vec4 v_color;\n"
         "out vec3 v_pos;\n"
         "void main()\n"
         "{ gl_Position = u_mvp*a_position; v_pos = a_position.xyz; v_color = a_color; }\n",

      .fragment_func.source =
         "#version 410\n"
         "in vec4 v_color;\n"
         "in vec3 v_pos;\n"
         "out vec4 f_color;\n"
         "void main()\n"
         "{\n"
         "   vec3 n = normalize(cross(dFdx(v_pos), dFdy(v_pos)));\n"
         "   f_color.rgb = v_color.rgb * (0.5 * dot(n, vec3(0.707, 0.707, 0.707)) + 0.5);\n"
         "   f_color.a = v_color.a;\n"
         "}\n",

      .uniform_blocks[0] = {
         .stage = SG_SHADERSTAGE_VERTEX,
         .size = sizeof(mat4x4),
         .glsl_uniforms = {
            [0] = { .glsl_name = "u_mvp", .type = SG_UNIFORMTYPE_MAT4 }
         }
      }
   });

   sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
      .layout = {
         .buffers[0].stride = (i32)geom.vertex_size,
         .attrs = {
            [0].format = SG_VERTEXFORMAT_FLOAT3,
            [1].format = SG_VERTEXFORMAT_FLOAT3
         }
      },

      .shader = shd,
      .index_type = SG_INDEXTYPE_UINT16,
      .depth = {
         .compare = SG_COMPAREFUNC_LESS_EQUAL,
         .write_enabled = true
      },

      .cull_mode = SG_CULLMODE_BACK
   });

   EctSetMouseMode(ECT_MOUSE_DISABLE_CURSOR);

   while(!EctShouldQuit(engine))
   {
      if (EctCheckKey(ECT_KEY_ESCAPE, ECT_KEY_IS_DOWN, false))
         EctQuit(engine);

      frame frame_size = EctGetSize();
      camera.aspect = (f32)frame_size.height / (f32)frame_size.width;

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

      sg_begin_pass(&(sg_pass) { .action = pass_action, .swapchain = EctGetSwapchain(engine) });

      sg_apply_pipeline(pip);
      sg_apply_uniforms(0, &SG_RANGE(camera.viewproj));
      EctRendererDrawGeometry(geom);

      sg_end_pass();
      sg_commit();
   }

   EctFree(engine);
   return 0;
}
