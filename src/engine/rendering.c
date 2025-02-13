#include "util/types.h"

#include "core/engine.h"
#include "engine/internal.h"

void Engine_Render(Engine* engine)
{
   ENG_EngineGlobal eng_glb = engine->internal;
   
   glfwSwapBuffers(eng_glb.window);
}