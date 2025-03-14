#include "util/types.h"

#include "engine.h"
#include "engine/internal.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

void Engine_Present(Engine* engine)
{
   eng_EngineGlobal eng_glb = engine->internal;
   
   glfwSwapBuffers(eng_glb.window);
}

error ENG_InitGL(GLFWwindow* window)
{
   error res = { 0 };

   if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress))
   {
      res.general = ERR_FATAL;
      res.extra = ERR_ENG_RNDR_OPENGL_FAILED;
   }

   return res;
}
