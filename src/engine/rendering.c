#include "util/types.h"

#include "core/engine.h"
#include "engine/internal.h"

void Engine_Present(Engine* engine)
{
   eng_EngineGlobal eng_glb = engine->internal;
   
   glfwSwapBuffers(eng_glb.window);
}