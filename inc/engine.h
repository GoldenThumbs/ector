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

struct Module_t;
typedef error (*ModuleFunc)(struct Module_t* self, Engine* engine);

typedef struct Module_t
{
   char* name;
   ModuleFunc mod_init;
   ModuleFunc mod_free;
   void* data;
} Module;

Engine* Engine_Init(EngineDesc* desc);
void Engine_Free(Engine* engine);

void Engine_RequestExit(Engine* engine);
bool Engine_CheckExitConditions(Engine* engine);

void* Engine_FetchModule(Engine* engine, const char* name);
void Engine_RegisterModule(Engine* engine, Module module);

bool Engine_CheckKey(Engine* engine, Key key, KeyAction key_action);
bool Engine_CheckKeyAdvanced(Engine* engine, Key key, KeyAction key_action, KeyModifiers modifiers);
KeyState Engine_GetKeyState(Engine* engine, Key key);

void Engine_SetMouseMode(Engine* engine, MouseMode mouse_mode);

void Engine_Present(Engine* engine);

resolution2d Engine_GetFrameSize(Engine* engine);
f64 Engine_GetFrameDelta(Engine* engine);

vec2 Engine_GetMousePos(Engine* engine);
vec2 Engine_GetMouseDelta(Engine* engine);
vec2 Engine_GetMouseScroll(Engine* engine);

bool Engine_CheckMouseButton(Engine* engine, MouseButton button, KeyAction button_action);
bool Engine_CheckMouseButtonAdvanced(Engine* engine, MouseButton button, KeyAction button_action, KeyModifiers modifiers);
KeyState Engine_GetMouseButtonState(Engine* engine, MouseButton button);

#endif
