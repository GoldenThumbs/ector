#ifndef ECT_DEFAULTMODULES_H
#define ECT_DEFAULTMODULES_H

#include "engine.h"

Module Module_Graphics(void);
Module Module_Renderer(void);

void Module_Defaults(Engine* engine, u32 exclude_count, char* module_excludes[]);

#endif
