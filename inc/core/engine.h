#ifndef ECT_ENGINE_H
#define ECT_ENGINE_H

#include "ect_types.h"

#include "core/keymap.h"
#include "core/renderer.h"

typedef enum EctMouseMode_t
{
   ECT_MOUSE_DEFAULT = 0,
   ECT_MOUSE_HIDE_CURSOR,
   ECT_MOUSE_DISABLE_CURSOR,
   ECT_MOUSE_CAPTURE_CURSOR
} EctMouseMode;

typedef struct EctEngineDesc_t
{
   char* app_name;
   struct {
      char* title;
      i32 width, height;
   } window;
} EctEngineDesc;

typedef struct EctEngine_t EctEngine;

EctEngine* EctInit(EctEngineDesc* desc);
void EctFree(EctEngine* engine);

void EctQuit(EctEngine* engine);
bool EctShouldQuit(EctEngine* engine);

bool EctCheckKey(EctKey key, u8 desired_state, bool exclusive);
bool EctCheckKeyMods(EctKey key, u8 desired_state, u8 modifiers, bool exclusive);
u8 EctKeyState(EctKey key);

void EctSetMouseMode(EctMouseMode mouse_mode);

frame EctGetSize(void);
vec2 EctGetMousePos(void);
vec2 EctGetMouseDelta(void);

EctRenderer* EctGetRenderer(EctEngine* engine);

#endif
