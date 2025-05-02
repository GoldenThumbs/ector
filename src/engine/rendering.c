#include "util/types.h"

#include "engine.h"
#include "engine/internal.h"
#include "graphics.h"
#include "renderer.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

resolution2d Engine_GetFrameSize(Engine* engine)
{
   eng_EngineGlobal eng_glb = engine->internal;
   return eng_glb.frame_size;
}

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

GraphicsContext* Engine_GraphicsContext(Engine* engine)
{
   return engine->graphics_context;
}

Renderer* Engine_Renderer(Engine* engine)
{
   return engine->renderer;
}
