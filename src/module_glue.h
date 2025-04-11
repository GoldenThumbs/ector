#ifndef MODULE_GLUE_H
#define MODULE_GLUE_H

#include "graphics.h"

GraphicsContext* MOD_InitGraphics(error* err);
void MOD_FreeGraphics(GraphicsContext* context);

#endif
