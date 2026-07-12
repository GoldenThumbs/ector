#include "util/types.h"
#include "util/files.h"
#include "engine.h"

#include "scripting/internal.h"
#include "scripting/lua_math.h"
#include "scripting/lua_input.h"
#include "scripting/lua_input_consts.h"
#include "scripting/lua_engine.h"
#include "scripting.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

error SCRP_RegisterEngine(lua_State* script_state, Engine* engine)
{
   error err = { 0 };

   if (script_state == NULL)
   {
      err.general = ERR_LEVEL_FATAL;
      err.extra = ERR_SCRIPT_LUA_INIT_FAILURE;

      Util_Log(NULL, SCRIPTING_MODULE, err, "Lua failed to initialize!");

      return err;
   }

   if (engine == NULL)
   {
      err.general = ERR_LEVEL_FATAL;
      err.extra = ERR_SCRIPT_NO_ENGINE;

      Util_Log(NULL, SCRIPTING_MODULE, err, "No Engine provided!");

      lua_close(script_state);
      return err;
   }

   const struct luaL_Reg engine_funcs[] = {
      { "RequestExit", SCRP_RequestExit },
      { "GetFrameSize", SCRP_GetFrameSize },
      { "GetFrameDelta", SCRP_GetFrameDelta },
      { "GetAppName", SCRP_GetAppName },
      { "GetAppPath", SCRP_GetAppPath },
      { "GetWindowTitle", SCRP_GetWindowTitle },
      { "SetAppName", SCRP_SetAppName },
      { "SetWindowTitle", SCRP_SetWindowTitle },
      { "RadiansToTurns", SCRP_RadiansToTurns },
      { "TurnsToRadians", SCRP_TurnsToRadians },
      { NULL, NULL }
   };

   lua_pushstring(script_state, ENGINE_DATA);
   lua_pushlightuserdata(script_state, engine);
   lua_settable(script_state, LUA_REGISTRYINDEX);

   // SCRP_RegisterMetaVector(script_state);

   lua_newtable(script_state);

   SCRP_AddVectorClass(script_state);
   SCRP_SetInput(script_state);
   luaL_setfuncs(script_state, engine_funcs, 0);

   lua_pushstring(script_state, "Module");
   lua_newtable(script_state);
   lua_rawset(script_state, -3);

   lua_setglobal(script_state, "Ector");

   return err;
}

void SCRP_SetInput(lua_State* script_state)
{
   lua_pushstring(script_state, "Input");
   lua_newtable(script_state);

   const struct luaL_Reg input_funcs[] = {
      { "GetMouseButtonDown", SCRP_GetMouseButtonDown },
      { "GetMouseButtonPressed", SCRP_GetMouseButtonPressed },
      { "GetMouseButtonReleased", SCRP_GetMouseButtonReleased },
      { "GetKeyDown", SCRP_GetKeyDown },
      { "GetKeyPressed", SCRP_GetKeyPressed },
      { "GetKeyReleased", SCRP_GetKeyReleased },
      { "GetMousePosition", SCRP_GetMousePosition },
      { "GetMouseDelta", SCRP_GetMouseDelta },
      { "GetMouseScroll", SCRP_GetMouseScroll },
      { "SetMouseMode", SCRP_SetMouseMode },
      { "SetRawMouseInput", SCRP_SetRawMouseInput },
      { NULL, NULL }
   };

   SCRP_SetMouseConstants(script_state);
   SCRP_SetKeyConstants(script_state);
   luaL_setfuncs(script_state, input_funcs, 0);

   lua_rawset(script_state, -3);

}

void SCRP_AddVectorClass(lua_State* script_state)
{
   const struct luaL_Reg vector_funcs[] = {
      { "__newindex", SCRP_NewVectorIndex },
      { "__add", SCRP_AddVector },
      { "__sub", SCRP_SubVector },
      { "__mul", SCRP_MulVector },
      { "__div", SCRP_DivVector },
      { "__unm", SCRP_NegVector },
      { "__len", SCRP_LengthOfVector },
      { "New", SCRP_NewVector },
      { "Index", SCRP_IndexVector },
      { "Normalize", SCRP_NormalizeVector },
      { "Cross", SCRP_CrossVector },
      { "Dot", SCRP_DotVector },
      { "Lerp", SCRP_LerpVector },
      { "Dup", SCRP_DupVector },
      { "QuatAngle", SCRP_QuatVectorAngle },
      { "NewQuat", SCRP_NewQuatVector },
      { "InverseQuat", SCRP_InverseQuatVector },
      { "Rotate", SCRP_RotateVector },
      { "MulQuat", SCRP_MulQuatVector },
      { "Slerp", SCRP_SlerpQuatVector },
      { "MakeLookingAt", SCRP_LookingAtQuatVector },
      { "RelativeToQuat", SCRP_RelativeToQuatVector },
      { NULL, NULL }
   };

   lua_pushstring(script_state, "Vector");
   lua_newtable(script_state);
   lua_pushnil(script_state);
   lua_copy(script_state, -2, -1);
   lua_setfield(script_state, -2, "__index");
   luaL_setfuncs(script_state, vector_funcs, 0);
   lua_rawset(script_state, -3);

}
