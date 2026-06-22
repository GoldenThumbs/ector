#ifndef WEB_LUA_MATH_H
#define WEB_LUA_MATH_H

#include "util/types.h"
#include "util/math.h"
#include "util/vec4.h"
#include "util/quaternion.h"
#include "util/files.h"

#include "scripting.h"

#include <lua.h>
#include <lauxlib.h>

#include <stddef.h>

#define META_VECTOR_DATA "ECT_META_VECTOR"

static inline bool SCRP_Util_IsInputVector(lua_State* script_state, int index)
{
   if (!lua_istable(script_state, index))
   {
      error err = { 0 };
      err.general = ERR_ERROR;
      err.extra = ERR_SCRIPT_INVALID_FUNCTION_INPUT;
      err.flags |= ERR_FLAG_INPUT_NOT_VECTOR;

      Util_Log(NULL, SCRIPTING_MODULE, err, "Input(s) is not a vector! Ignoring operation...");

      return false;
   }

   return true;
}

static inline void SCRP_Util_PushVector(lua_State* script_state, vec4 vector)
{
   lua_newtable(script_state);
   lua_pushstring(script_state, META_VECTOR_DATA);
   lua_gettable(script_state, LUA_REGISTRYINDEX);
   lua_setmetatable(script_state, -2);

   lua_pushstring(script_state, "x");
   lua_pushnumber(script_state, (lua_Number)vector.x);
   lua_rawset(script_state, -3);

   lua_pushstring(script_state, "y");
   lua_pushnumber(script_state, (lua_Number)vector.y);
   lua_rawset(script_state, -3);

   lua_pushstring(script_state, "z");
   lua_pushnumber(script_state, (lua_Number)vector.z);
   lua_rawset(script_state, -3);

   lua_pushstring(script_state, "w");
   lua_pushnumber(script_state, (lua_Number)vector.w);
   lua_rawset(script_state, -3);

}

static inline vec4 SCRP_Util_GetVector(lua_State* script_state, int index)
{
   vec4 result = { 0 };
   if (!SCRP_Util_IsInputVector(script_state, index))
      return result;

   lua_getfield(script_state, index, "x");
   result.x = (f32)lua_tonumber(script_state, -1);
   lua_pop(script_state, 1);

   lua_getfield(script_state, index, "y");
   result.y = (f32)lua_tonumber(script_state, -1);
   lua_pop(script_state, 1);

   lua_getfield(script_state, index, "z");
   result.z = (f32)luaL_checknumber(script_state, -1);
   lua_pop(script_state, 1);

   lua_getfield(script_state, index, "w");
   result.w = (f32)lua_tonumber(script_state, -1);
   lua_pop(script_state, 1);

   return result;
}

static inline int SCRP_NewVector(lua_State* script_state)
{
   vec4 vector = { 0 };
   vector.x = (f32)luaL_optnumber(script_state, 1, 0.0);
   vector.y = (f32)luaL_optnumber(script_state, 2, 0.0);
   vector.z = (f32)luaL_optnumber(script_state, 3, 0.0);
   vector.w = (f32)luaL_optnumber(script_state, 4, 0.0);

   i32 inputs = (i32)lua_gettop(script_state);
   if (inputs > 4)
   {
      error err = { 0 };
      err.general = ERR_WARN;
      err.extra = ERR_SCRIPT_INVALID_FUNCTION_INPUT;
      err.flags |= ERR_FLAG_TOO_MANY_INPUTS;

      Util_Log(NULL, SCRIPTING_MODULE, err, "Vectors can only have up to 4 inputs, got %i...", inputs);

   }

   SCRP_Util_PushVector(script_state, vector);

   return 1;
}

static inline int SCRP_NewVectorIndex(lua_State* script_state)
{
   uS string_size = 0;
   const char* key = luaL_checklstring(script_state, 2, &string_size);
   lua_Number value = luaL_checknumber(script_state, 3);

   if (string_size != 1 || !(key[0] == 'x' || key[0] == 'y' || key[0] == 'z' || key[0] == 'w'))
   {
      error err = { 0 };
      err.general = ERR_ERROR;
      err.extra = ERR_SCRIPT_INVALID_FUNCTION_INPUT;
      err.flags |= ERR_FLAG_INVALID_VECTOR_KEY;

      Util_Log(NULL, SCRIPTING_MODULE, err, "Given key is invalid for vectors! Key: %s", key);
      lua_pushinteger(script_state, LUA_ERRERR);
      lua_error(script_state);

      return 0;
   }

   lua_pushstring(script_state, key);
   lua_pushnumber(script_state, value);
   lua_rawset(script_state, 1);

   return 0;
}

static inline int SCRP_AddVector(lua_State* script_state)
{
   // Util_Log(NULL, SCRIPTING_MODULE, (error){ 0 }, "adding vectors");

   vec4 a = SCRP_Util_GetVector(script_state, 1);
   vec4 b = SCRP_Util_GetVector(script_state, 2);
   vec4 result = Util_AddVec4(a, b);

   SCRP_Util_PushVector(script_state, result);

   return 1;
}

static inline int SCRP_SubVector(lua_State* script_state)
{
   vec4 a = SCRP_Util_GetVector(script_state, 1);
   vec4 b = SCRP_Util_GetVector(script_state, 2);
   vec4 result = Util_SubVec4(a, b);

   SCRP_Util_PushVector(script_state, result);

   return 1;
}

static inline int SCRP_MulVector(lua_State* script_state)
{
   vec4 a = SCRP_Util_GetVector(script_state, 1);
   vec4 b = SCRP_Util_GetVector(script_state, 2);
   vec4 result = Util_MulVec4(a, b);

   SCRP_Util_PushVector(script_state, result);

   return 1;
}

static inline int SCRP_DivVector(lua_State* script_state)
{
   vec4 a = SCRP_Util_GetVector(script_state, 1);
   vec4 b = SCRP_Util_GetVector(script_state, 2);
   vec4 result = Util_DivVec4(a, b);

   SCRP_Util_PushVector(script_state, result);

   return 1;
}

static inline int SCRP_IndexVector(lua_State* script_state)
{
   vec4 vector = SCRP_Util_GetVector(script_state, 1);
   i32 index = (i32)luaL_checkinteger(script_state, 2);

   if (index < 1 || index > 4)
   {
      error err = { 0 };
      err.general = ERR_WARN;
      err.extra = ERR_SCRIPT_INVALID_FUNCTION_INPUT;
      err.flags |= ERR_FLAG_INDEX_WRONG_RANGE;

      Util_Log(NULL, SCRIPTING_MODULE, err, "Index \"%i\" is out of range! Vector index range is 1 to 4. Clamping index to range...", index);
      index = M_CLAMP(index, 1, 4);

   }

   lua_pushnumber(script_state, (lua_Number)vector.arr[index - 1]);

   return 1;
}

static inline int SCRP_NormalizeVector(lua_State* script_state)
{
   vec4 vector = SCRP_Util_GetVector(script_state, 1);
   vec4 result = Util_NormalizeVec4(vector);

   SCRP_Util_PushVector(script_state, result);

   return 1;
}

static inline int SCRP_DotVector(lua_State* script_state)
{
   vec4 a = SCRP_Util_GetVector(script_state, 1);
   vec4 b = SCRP_Util_GetVector(script_state, 2);
   f32 result = Util_DotVec4(a, b);

   lua_pushnumber(script_state, (lua_Number)result);

   return 1;
}

static inline int SCRP_NewQuatVector(lua_State* script_state)
{
   vec3 euler = SCRP_Util_GetVector(script_state, 1).xyz;
   quat result = Util_MakeQuatEuler(euler);

   SCRP_Util_PushVector(script_state, result);

   return 1;
}

static inline int SCRP_MulQuatVector(lua_State* script_state)
{
   vec4 a = SCRP_Util_GetVector(script_state, 1);
   vec4 b = SCRP_Util_GetVector(script_state, 2);
   quat result = Util_MulQuat(a, b);

   SCRP_Util_PushVector(script_state, result);

   return 1;
}

static inline int SCRP_RotateVector(lua_State* script_state)
{
   vec3 vector = SCRP_Util_GetVector(script_state, 1).xyz;
   quat rotation = SCRP_Util_GetVector(script_state, 2);
   vec4 result = { 0 };
   result.xyz = Util_RotatePoint(rotation, vector);

   SCRP_Util_PushVector(script_state, result);

   return 1;
}

void SCRP_RegisterMetaVector(lua_State* script_state);
void SCRP_SetVectorFuncs(lua_State* script_state);

#endif
