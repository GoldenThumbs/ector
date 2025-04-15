#ifndef MODULE_GLUE_H
#define MODULE_GLUE_H

#include "graphics.h"
#include "renderer.h"

enum {
   ERR_FLAG_ENGINE_FAILED = (1u << 0),
   ERR_FLAG_GRAPHICS_FAILED = (1u << 1),
   ERR_FLAG_RENDERER_FAILED = (1u << 2)
};

GraphicsContext* MOD_InitGraphics(error* err);
void MOD_FreeGraphics(GraphicsContext* context);

#endif
