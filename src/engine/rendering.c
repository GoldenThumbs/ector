#include "util/types.h"

#include "engine.h"
#include "engine/internal.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

f64 Engine_GetFrameDelta(Engine* engine)
{
   eng_EngineGlobal* eng_glb = &engine->internal;

   return eng_glb->frame_delta;
}

void Engine_Present(Engine* engine)
{
   eng_EngineGlobal* eng_glb = &engine->internal;
   glfwSwapBuffers(eng_glb->window);
}
