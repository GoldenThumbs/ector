#ifndef WEB_SCRIPTING_ENGINEMODULE
#define WEB_SCRIPTING_ENGINEMODULE

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

static inline int SCRP_GetAppPath(lua_State* script_state)
{
   lua_pushstring(script_state, ENGINE_DATA);
   lua_gettable(script_state, LUA_REGISTRYINDEX);
   Engine* engine = lua_touserdata(script_state, -1);

   lua_pushstring(script_state, Engine_GetAppPath(engine));

   return 1;
}

error SCRP_RegisterEngine(lua_State* script_state, Engine* engine);

#endif
