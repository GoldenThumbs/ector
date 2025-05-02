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
      bool hidden;
   } window;
   struct {
      bool enabled;
   } renderer;
} EngineDesc;

typedef struct Engine_t Engine;

Engine* Engine_Init(EngineDesc* desc);
void Engine_Free(Engine* engine);

void Engine_RequestExit(Engine* engine);
bool Engine_CheckExitConditions(Engine* engine);

bool Engine_CheckKey(Engine* engine, Key key, KeyAction key_action);
bool Engine_CheckKeyAdvanced(Engine* engine, Key key, KeyAction key_action, KeyModifiers modifiers);
KeyState Engine_GetKeyState(Engine* engine, Key key);

void Engine_SetMouseMode(Engine* engine, MouseMode mouse_mode);

void Engine_Present(Engine* engine);

resolution2d Engine_GetFrameSize(Engine* engine);
f64 Engine_GetFrameDelta(Engine* engine);

vec2 Engine_GetMousePos(Engine* engine);
vec2 Engine_GetMouseDelta(Engine* engine);

struct GraphicsContext_t* Engine_GraphicsContext(Engine* engine);
struct Renderer_t* Engine_Renderer(Engine* engine);

#endif
