#include "core/keymap.h"
#include <ect_types.h>
#include <ect_math.h>
#include <core/engine.h>

#include <sokol_gfx.h>
#include <sokol_log.h>

sg_environment GFX_EctEnvironment(void);
sg_swapchain GFX_EctSwapChain(void);

int main(int argc, char* argv[])
{
   EctEngine engine = EctInitEngine(&(EctEngineDesc){ 0 });

   sg_setup(&(sg_desc) {
      .environment = GFX_EctEnvironment(),
      .logger.func = slog_func
   });

   if (!sg_isvalid())
      return -1;

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

   camera.origin = VEC3(0, 0, 0);
   camera.euler = VEC3(0);
   camera.fov = 25.0f;
   camera.aspect = 1.0f;
   camera.viewproj = MAT4(0);

   f32 vrt[] = {
       0,-1, 0,  0.5f, 0.5f, 1.0f,
      -1, 0, 0,  1.0f, 0.5f, 0.5f,
       0, 0, 1,  0.5f, 1.0f, 0.5f,
       0, 1, 0,  0.5f, 0.5f, 0.0f,
       1, 0, 0,  0.0f, 0.5f, 0.5f,
       0, 0,-1,  0.5f, 0.0f, 0.5f
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

   sg_buffer vrt_buf = sg_make_buffer(&(sg_buffer_desc){
      .data = SG_RANGE(vrt)
   });

   sg_buffer idx_buf = sg_make_buffer(&(sg_buffer_desc){
      .type = SG_BUFFERTYPE_INDEXBUFFER,
      .data = SG_RANGE(idx)
   });

   sg_bindings bin = {
      .vertex_buffers[0] = vrt_buf,
      .index_buffer = idx_buf
   };

   sg_shader shd = sg_make_shader(&(sg_shader_desc){
      .vertex_func.source =
         "#version 410\n"
         "uniform mat4 u_mvp;\n"
         "layout(location=0) in vec4 a_position;\n"
         "layout(location=1) in vec4 a_color;\n"
         "out vec4 v_color;\n"
         "void main()\n"
         "{ gl_Position = u_mvp*a_position; v_color = a_color; }\n",
      .fragment_func.source =
         "#version 410\n"
         "in vec4 v_color;\n"
         "out vec4 f_color;\n"
         "void main()\n"
         "{ f_color = v_color; }\n",
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
         .buffers[0].stride = 24,
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

   while(!EctShouldQuit(&engine))
   {
      i32 width, height = 0;
      EctGetFrameSize(&width, &height);
      camera.aspect = (f32)height / (f32)width;

      if (EctCheckKey(ECT_KEY_ESCAPE, ECT_KEY_IS_DOWN, false))
         engine.quit = true;

      camera.viewproj = EctMulMat4(
         EctPerspectiveMatrix(camera.fov, camera.aspect, 0.05f, 100.0f),
         EctViewMatrix(camera.origin, camera.euler, 2)
      );

      sg_begin_pass(&(sg_pass) { .action = pass_action, .swapchain = GFX_EctSwapChain() });

      sg_apply_pipeline(pip);
      sg_apply_bindings(&bin);
      sg_apply_uniforms(0, &SG_RANGE(camera.viewproj));
      sg_draw(0, 24, 1);

      sg_end_pass();
      sg_commit();

      camera.euler.y += 0.08f;
   }

   sg_shutdown();
   return 0;
}

sg_environment GFX_EctEnvironment(void)
{
   return (sg_environment) {
      .defaults = {
         .color_format = SG_PIXELFORMAT_RGBA8,
         .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
         .sample_count = 1
      }
   };
}

sg_swapchain GFX_EctSwapChain(void)
{
   i32 width, height = 0;
   EctGetFrameSize(&width, &height);

   return (sg_swapchain) {
      .width = width,
      .height = height,
      .sample_count = 1,
      .color_format = SG_PIXELFORMAT_RGBA8,
      .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
      .gl.framebuffer = 0
   };
}
