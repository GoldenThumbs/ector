#ifndef WEB_SCRIPTING_ENGINEMODULE
#define WEB_SCRIPTING_ENGINEMODULE

#include "scripting.h"
#include "util/types.h"
#include "engine.h"

#include "scripting/internal.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

static inline int SCRP_RequestExit(lua_State* script_state)
{
   lua_pushstring(script_state, ENGINE_DATA);
   lua_gettable(script_state, LUA_REGISTRYINDEX);
   Engine* engine = lua_touserdata(script_state, -1);

   Engine_RequestExit(engine);

   return 0;
}

static inline int SCRP_GetFrameSize(lua_State* script_state)
{
   lua_pushstring(script_state, ENGINE_DATA);
   lua_gettable(script_state, LUA_REGISTRYINDEX);
   Engine* engine = lua_touserdata(script_state, -1);

   res2D result = Engine_GetFrameSize(engine);

   lua_pushinteger(script_state, (lua_Integer)result.width);
   lua_pushinteger(script_state, (lua_Integer)result.height);

   return 2;
}

static inline int SCRP_GetFrameDelta(lua_State* script_state)
{
   lua_pushstring(script_state, ENGINE_DATA);
   lua_gettable(script_state, LUA_REGISTRYINDEX);
   Engine* engine = lua_touserdata(script_state, -1);

   f64 result = Engine_GetFrameDelta(engine);

   lua_pushnumber(script_state, (lua_Number)result);

   return 1;
}

static inline int SCRP_GetAppName(lua_State* script_state)
{
   lua_pushstring(script_state, ENGINE_DATA);
   lua_gettable(script_state, LUA_REGISTRYINDEX);
   Engine* engine = lua_touserdata(script_state, -1);

   lua_pushstring(script_state, Engine_GetAppName(engine));

   return 1;
}

static inline int SCRP_GetAppPath(lua_State* script_state)
{
   lua_pushstring(script_state, ENGINE_DATA);
   lua_gettable(script_state, LUA_REGISTRYINDEX);
   Engine* engine = lua_touserdata(script_state, -1);

   lua_pushstring(script_state, Engine_GetAppPath(engine));

   return 1;
}

static inline int SCRP_GetWindowTitle(lua_State* script_state)
{
   lua_pushstring(script_state, ENGINE_DATA);
   lua_gettable(script_state, LUA_REGISTRYINDEX);
   Engine* engine = lua_touserdata(script_state, -1);

   lua_pushstring(script_state, Engine_GetWindowTitle(engine));

   return 1;
}

static inline int SCRP_SetAppName(lua_State* script_state)
{
   lua_pushstring(script_state, ENGINE_DATA);
   lua_gettable(script_state, LUA_REGISTRYINDEX);
   Engine* engine = lua_touserdata(script_state, -1);

   const char* app_name = Scripting_GetString(script_state, 1);
   Engine_SetAppName(engine, app_name);

   return 0;
}

static inline int SCRP_SetWindowTitle(lua_State* script_state)
{
   lua_pushstring(script_state, ENGINE_DATA);
   lua_gettable(script_state, LUA_REGISTRYINDEX);
   Engine* engine = lua_touserdata(script_state, -1);

   const char* window_title = Scripting_GetString(script_state, 1);
   Engine_SetWindowTitle(engine, window_title);

   return 0;
}

error SCRP_RegisterEngine(lua_State* script_state, Engine* engine);

#endif
