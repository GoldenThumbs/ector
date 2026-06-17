#ifndef WEB_LUA_INPUT_H
#define WEB_LUA_INPUT_H

#include "util/types.h"
#include "util/keymap.h"
#include "engine.h"

#include "scripting/lua_math.h"
#include "scripting/internal.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

static inline int SCRP_GetMousePosition(lua_State* script_state)
{
   lua_pushstring(script_state, ENGINE_DATA);
   lua_gettable(script_state, LUA_REGISTRYINDEX);
   Engine* engine = lua_touserdata(script_state, -1);

   vec4 result = { 0 };
   result.xy = Engine_GetMousePos(engine);

   SCRP_PushVector(script_state, result);

   return 1;
}

static inline int SCRP_GetMouseDelta(lua_State* script_state)
{
   lua_pushstring(script_state, ENGINE_DATA);
   lua_gettable(script_state, LUA_REGISTRYINDEX);
   Engine* engine = lua_touserdata(script_state, -1);

   vec4 result = { 0 };
   result.xy = Engine_GetMouseDelta(engine);

   SCRP_PushVector(script_state, result);

   return 1;
}

static inline int SCRP_GetMouseScroll(lua_State* script_state)
{
   lua_pushstring(script_state, ENGINE_DATA);
   lua_gettable(script_state, LUA_REGISTRYINDEX);
   Engine* engine = lua_touserdata(script_state, -1);

   vec4 result = { 0 };
   result.xy = Engine_GetMouseScroll(engine);

   return 1;
}

static inline int SCRP_SetMouseMode(lua_State* script_state)
{
   lua_pushstring(script_state, ENGINE_DATA);
   lua_gettable(script_state, LUA_REGISTRYINDEX);
   Engine* engine = lua_touserdata(script_state, -1);

   MouseMode mouse_mode = (MouseMode)lua_tointeger(script_state, 1);

   Engine_SetMouseMode(engine, mouse_mode);

   return 0;
}

static inline int SCRP_SetRawMouseInput(lua_State* script_state)
{
   lua_pushstring(script_state, ENGINE_DATA);
   lua_gettable(script_state, LUA_REGISTRYINDEX);
   Engine* engine = lua_touserdata(script_state, -1);

   bool raw_mouse_input = (bool)lua_toboolean(script_state, 1);

   Engine_SetRawMouseInput(engine, raw_mouse_input);

   return 0;
}

static inline int SCRP_GetMouseButton(lua_State* script_state, KeyAction action)
{
   lua_pushstring(script_state, ENGINE_DATA);
   lua_gettable(script_state, LUA_REGISTRYINDEX);
   Engine* engine = lua_touserdata(script_state, -1);

   MouseButton mouse_button = (MouseButton)lua_tointeger(script_state, 1);
   bool result = Engine_CheckMouseButton(engine, mouse_button, action);

   lua_pushboolean(script_state, result);
   return 1;
}

static inline int SCRP_GetMouseButtonDown(lua_State* script_state)
{
   return SCRP_GetMouseButton(script_state, KEY_IS_DOWN);
}

static inline int SCRP_GetMouseButtonPressed(lua_State* script_state)
{
   return SCRP_GetMouseButton(script_state, KEY_JUST_PRESSED);
}

static inline int SCRP_GetMouseButtonReleased(lua_State* script_state)
{
   return SCRP_GetMouseButton(script_state, KEY_JUST_RELEASED);
}

static inline int SCRP_GetKey(lua_State* script_state, KeyAction action)
{
   lua_pushstring(script_state, ENGINE_DATA);
   lua_gettable(script_state, LUA_REGISTRYINDEX);
   Engine* engine = lua_touserdata(script_state, -1);

   Key key = (Key)lua_tointeger(script_state, 1);
   bool result = Engine_CheckKey(engine, key, action);

   lua_pushboolean(script_state, result);
   return 1;
}

static inline int SCRP_GetKeyDown(lua_State* script_state)
{
   return SCRP_GetKey(script_state, KEY_IS_DOWN);
}

static inline int SCRP_GetKeyPressed(lua_State* script_state)
{
   return SCRP_GetKey(script_state, KEY_JUST_PRESSED);
}

static inline int SCRP_GetKeyReleased(lua_State* script_state)
{
   return SCRP_GetKey(script_state, KEY_JUST_RELEASED);
}

void SCRP_SetInput(lua_State* script_state);

#endif
