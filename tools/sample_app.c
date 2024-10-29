#define GLFW_INCLUDE_NONE

#include <sokol_gfx.h>
#include <sokol_log.h>
#include <GLFW/glfw3.h>

sg_environment GFX_EctEnvironment(void);
sg_swapchain GFX_EctSwapChain(GLFWwindow* win);

int main(int argc, char* argv[])
{
   GLFWwindow* win = NULL;
   if (!glfwInit())
      return -1;

   win = glfwCreateWindow(640, 480, "Ector", NULL, NULL);
   if (!win)
   {
      glfwTerminate();
      return -1;
   }
   glfwMakeContextCurrent(win);

   sg_setup(&(sg_desc) {
      .environment = GFX_EctEnvironment(),
      .logger.func = slog_func
   });

   if (!sg_isvalid())
      return -1;

   sg_pass_action pass_action = {
      .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = { 0.1f, 0.1f, 0.1f, 1.0f } }
   };

   while(!glfwWindowShouldClose(win))
   {
      glfwPollEvents();

      sg_begin_pass(&(sg_pass) { .action = pass_action, .swapchain = GFX_EctSwapChain(win) });
      sg_end_pass();
      sg_commit();

      glfwSwapBuffers(win);
   }

   sg_shutdown();
   glfwTerminate();
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

sg_swapchain GFX_EctSwapChain(GLFWwindow* win)
{
   int width, height;
   glfwGetFramebufferSize(win, &width, &height);

   return (sg_swapchain) {
      .width = width,
      .height = height,
      .sample_count = 1,
      .color_format = SG_PIXELFORMAT_RGBA8,
      .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
      .gl.framebuffer = 0
   };
}
