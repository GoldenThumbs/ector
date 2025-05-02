#ifndef MODULE_GLUE_H
#define MODULE_GLUE_H

#include "engine.h"

enum {
   ERR_FLAG_ENGINE_FAILED = (1u << 0),
   ERR_FLAG_GRAPHICS_FAILED = (1u << 1),
   ERR_FLAG_RENDERER_FAILED = (1u << 2)
};

void MOD_DefaultModules(Engine* engine);

error MOD_GraphicsInit(Module* self, Engine* engine);
error MOD_GraphicsFree(Module* self, Engine* engine);

error MOD_RendererInit(Module* self, Engine* engine);
error MOD_RendererFree(Module* self, Engine* engine);

#endif
