#ifndef ECT_ENGINE_H
#define ECT_ENGINE_H

#include "util/types.h"

#include "util/keymap.h"

typedef enum MouseMode_t
{
   MOUSE_DEFAULT = 0,
   MOUSE_HIDE_CURSOR,
   MOUSE_DISABLE_CURSOR,
   MOUSE_CAPTURE_CURSOR
} MouseMode;

typedef struct EngineDesc_t
{
   char* app_name;
   struct {
      char* title;
      i32 width, height;
   } window;
} EngineDesc;

typedef struct Engine_t Engine;

Engine* Engine_Init(EngineDesc* desc);
void Engine_Free(Engine* engine);

void Engine_Quit(Engine* engine);
bool Engine_ShouldQuit(Engine* engine);

bool Engine_CheckKey(Engine* engine, Key key, KeyAction key_action);
bool Engine_CheckKeyAdvanced(Engine* engine, Key key, KeyAction key_action, KeyModifiers modifiers);
KeyState Engine_GetKeyState(Engine* engine, Key key);

void Engine_SetMouseMode(Engine* engine, MouseMode mouse_mode);

size2i Engine_GetSize(Engine* engine);
vec2 Engine_GetMousePos(Engine* engine);
vec2 Engine_GetMouseDelta(Engine* engine);

void Engine_Render(Engine* engine);

#endif
