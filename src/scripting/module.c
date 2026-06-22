#include "util/types.h"
#include "util/array.h"
#include "util/files.h"
#include "util/resource.h"
#include "engine.h"

#include "scripting/internal.h"
#include "scripting/lua_math.h"
#include "scripting/lua_engine.h"
#include "scripting.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <llimits.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

ScriptHandler* Scripting_InitHandler(Engine* engine)
{
   if (engine == NULL)
      return NULL;

   lua_State* script_state = luaL_newstate();
   if (SCRP_RegisterEngine(script_state, engine).general != ERR_OK)
      return NULL;

   luaL_openlibs(script_state);

   ScriptHandler* script_handler = malloc(sizeof(ScriptHandler));
   if (script_handler == NULL)
   {
      lua_close(script_state);
      return NULL;
   }

   script_handler->engine = engine;
   script_handler->script_state = script_state;
   script_handler->scripts = NEW_ARRAY_N(scrp_Script, 4);
   script_handler->freed_script_root = INVALID_HANDLE;

   return script_handler;
}

void Scripting_FreeHandler(ScriptHandler* script_handler)
{
   if (script_handler == NULL)
      return;

   for (u32 script_i = 0; script_i < Util_ArrayLength(script_handler->scripts); script_i++)
   {
      scrp_Script script = script_handler->scripts[script_i];
      if (script.code.data != NULL)
         free(script.code.data);

   }

   FREE_ARRAY(script_handler->scripts);
   lua_close(script_handler->script_state);
   free(script_handler);

}

Script Scripting_CreateScriptFromCode(ScriptHandler* script_handler, const char* lua_code, const char* script_name)
{
   if (script_handler == NULL || lua_code == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   scrp_Script script = { 0 };
   script.name = script_name;
   int lua_err = luaL_loadstring(script_handler->script_state, lua_code);
   if (lua_err != 0)
   {
      error err = { 0 };
      err.general = ERR_ERROR;
      err.extra = ERR_SCRIPT_LUA_COMPILE_FAILURE;
      err.flags |= (lua_err == LUA_ERRSYNTAX) ? ERR_FLAG_INVALID_SYNTAX : 0;

      const char* err_string = lua_tostring(script_handler->script_state, -1);

      Util_Log(NULL, SCRIPTING_MODULE, err, "Script failed to compile! [%s]\nPrinting Lua error...\n%s", script.name, err_string);

      return (handle){ .id = INVALID_HANDLE_ID };
   }

   lua_dump(script_handler->script_state, SCRP_LuaWriter, &script.code, 0);

   if (script_handler->freed_script_root == INVALID_HANDLE)
      return ADD_RESOURCE(script_handler->scripts, script);

   return REUSE_RESOURCE(script_handler->scripts, script, script_handler->freed_script_root);
}

Script Scripting_LoadScriptFromFile(ScriptHandler* script_handler, const char* lua_file_path)
{
   if (script_handler == NULL || lua_file_path == NULL)
      return (handle){ .id = INVALID_HANDLE_ID };

   char* script_path = Util_MakeFilePath(Engine_GetAppPath(script_handler->engine), lua_file_path);
   memblob script_code = Util_LoadFileIntoMemory(script_path, false);
   Script result = Scripting_CreateScriptFromCode(script_handler, script_code.data, lua_file_path);

   free(script_path);
   free(script_code.data);

   return result;
}

void Scripting_FreeScript(ScriptHandler* script_handler, Script res_script)
{
   if (script_handler == NULL || !Util_IsHandleValid(script_handler->scripts, res_script))
      return;

   scrp_Script* script = &script_handler->scripts[res_script.handle];
   if (SCRP_IsScriptValid(*script, res_script))
      return;

   script->next_freed = script_handler->freed_script_root;
   script_handler->freed_script_root = res_script.handle;

   if (script->code.data != NULL)
      free(script->code.data);

}

error Scripting_RunScript(ScriptHandler* script_handler, Script res_script)
{
   if (script_handler == NULL || !Util_IsHandleValid(script_handler->scripts, res_script))
      return (error){ .general = ERR_ERROR };

   scrp_Script script = script_handler->scripts[res_script.handle];
   if (!SCRP_IsScriptValid(script, res_script))
      return (error){ .general = ERR_ERROR };

   luaL_loadbuffer(
      script_handler->script_state,
      script.code.data,
      script.code.size,
      script.name
   );

   int lua_err = lua_pcall(script_handler->script_state, 0, LUA_MULTRET, 0);
   if (lua_err != 0)
   {
      error err = { 0 };
      err.general = ERR_ERROR;
      err.extra = ERR_SCRIPT_RUNTIME_ERROR;

      const char* err_string = lua_tostring(script_handler->script_state, -1);

      Util_Log(NULL, SCRIPTING_MODULE, err, "Script failed to run! [%s]\nPrinting Lua error...\n%s", script.name, err_string);

      return err;
   }

   lua_pop(script_handler->script_state, lua_gettop(script_handler->script_state));

   return (error){ 0 };
}

bool Scripting_FieldExists(void* script_state, const char* name)
{
   if (script_state == NULL || name == NULL)
      return false;

   lua_pushstring(script_state, name);
   lua_rawget(script_state, -2);

   bool result = !lua_isnil(script_state, -1);
   lua_pop(script_state, 1);

   return result;
}

void Scripting_AddGlobalVariable(ScriptHandler* script_handler, const char* name, memblob value, u8 variable_type)
{
   if (script_handler == NULL || name == NULL || value.data == NULL)
      return;

   switch (variable_type)
   {
      case SCRP_VARIABLE_I32: {
         if (value.size != sizeof(i32))
            return;

         i32* ptr = (i32*)value.data;
         lua_pushinteger(script_handler->script_state, (lua_Integer)(*ptr));

      } break;

      case SCRP_VARIABLE_F32: {
         if (value.size != sizeof(f32))
            return;

         f32* ptr = (f32*)value.data;
         lua_pushnumber(script_handler->script_state, (lua_Number)(*ptr));

      } break;

      case SCRP_VARIABLE_VEC4: {
         if (value.size != sizeof(vec4))
            return;

         vec4* ptr = (vec4*)value.data;
         SCRP_Util_PushVector(script_handler->script_state, *ptr);

      } break;

      case SCRP_VARIABLE_STRING: {
         if (value.size == 0)
            return;

         char* ptr = (char*)value.data;
         lua_pushstring(script_handler->script_state, ptr);

      } break;

      case SCRP_VARIABLE_USERDATA: {
         if (value.size == 0)
            return;

         void* ptr = lua_newuserdatauv(script_handler->script_state, value.size, 0);
         memcpy(ptr, value.data, value.size);

      } break;

      default:
         return;
   }

   lua_setglobal(script_handler->script_state, name);

}

void Scripting_AddGlobalFunction(ScriptHandler* script_handler, const char* name, LuaCFunc function)
{
   if (script_handler == NULL || name == NULL || function == NULL)
      return;

   lua_pushcfunction(script_handler->script_state, (lua_CFunction)function);
   lua_setglobal(script_handler->script_state, name);

}

void Scripting_AddModule(ScriptHandler* script_handler, ScriptModuleDesc desc)
{
   if (script_handler == NULL || desc.name == NULL)
      return;

   lua_getglobal(script_handler->script_state, "Ector");
   lua_pushstring(script_handler->script_state, "Module");
   lua_rawget(script_handler->script_state, -2);

   if (Scripting_FieldExists(script_handler->script_state, desc.name))
      return;

   lua_pushstring(script_handler->script_state, desc.name);
   lua_newtable(script_handler->script_state);

   if (desc.data != NULL)
   {
      lua_pushstring(script_handler->script_state, "data");
      lua_pushlightuserdata(script_handler->script_state, desc.data);
      lua_rawset(script_handler->script_state, -3);

   }

   if (desc.register_func != NULL)
      desc.register_func(script_handler->script_state, desc.data);

   lua_rawset(script_handler->script_state, -3);

}

void Scripting_AddFunctionsToTable(void* script_state, NamedScriptFunction named_functions[], u32 function_count)
{
   if (script_state == NULL || named_functions == NULL || !lua_istable(script_state, -1))
      return;

   for (u32 func_i = 0; func_i < function_count; func_i++)
   {
      NamedScriptFunction named_func = named_functions[func_i];
      if (named_func.name == NULL || named_func.function == NULL || Scripting_FieldExists(script_state, named_func.name))
         continue;

      lua_pushstring(script_state, named_func.name);
      lua_pushcfunction(script_state, (lua_CFunction)named_func.function);
      lua_rawset(script_state, -3);

   }

}

void Scripting_AddEnumToTable(void* script_state, const char* enum_name, EnumPair enum_pairs[], u32 enum_count)
{
   if (script_state == NULL || enum_name == NULL || enum_pairs == NULL || !lua_istable(script_state, -1) || Scripting_FieldExists(script_state, enum_name))
      return;

   lua_pushstring(script_state, enum_name);
   lua_newtable(script_state);

   for (u32 enum_i = 0; enum_i < enum_count; enum_i++)
   {
      EnumPair enum_pair = enum_pairs[enum_i];
      if (enum_pair.name == NULL)
         continue;

      lua_pushstring(script_state, enum_pair.name);
      lua_pushinteger(script_state, (lua_Integer)enum_pair.value);
      lua_rawset(script_state, -3);

   }

   lua_rawset(script_state, -3);

}

Engine* Scripting_GetEngine(void* script_state)
{
   if (script_state == NULL)
      return NULL;

   lua_pushstring(script_state, ENGINE_DATA);
   lua_gettable(script_state, LUA_REGISTRYINDEX);
   Engine* engine = lua_touserdata(script_state, -1);

   return engine;
}

void* Scripting_GetModuleData(void* script_state, const char* module_name)
{
   if (script_state == NULL || module_name == NULL)
      return NULL;

   lua_getglobal(script_state, "Ector");
   lua_pushstring(script_state, "Module");
   lua_rawget(script_state, -2);

   if (!Scripting_FieldExists(script_state, module_name))
      return NULL;

   lua_pushstring(script_state, module_name);
   lua_rawget(script_state, -2);

   if (!Scripting_FieldExists(script_state, "data") || !lua_islightuserdata(script_state, 0))
      return NULL;

   lua_pushstring(script_state, "data");
   lua_rawget(script_state, -2);

   return lua_touserdata(script_state, -1);
}

i32 Scripting_GetI32(void* script_state, LuaIndex index)
{
   if (script_state == NULL || lua_isnil(script_state, index) || !lua_isinteger(script_state, index))
      return 0;

   return (i32)lua_tointeger(script_state, index);
}

f32 Scripting_GetF32(void* script_state, LuaIndex index)
{
   if (script_state == NULL || lua_isnil(script_state, index) || !lua_isnumber(script_state, index))
      return 0;

   return (f32)lua_tonumber(script_state, index);
}

vec4 Scripting_GetVec4(void* script_state, LuaIndex index)
{
   if (script_state == NULL || lua_isnil(script_state, index))
      return (vec4){ 0 };

   return SCRP_Util_GetVector(script_state, index);
}

const char* Scripting_GetString(void* script_state, LuaIndex index)
{
   if (script_state == NULL || lua_isnil(script_state, index) || !lua_isstring(script_state, index))
      return NULL;

   return lua_tostring(script_state, index);
}

memblob Scripting_GetUserData(void* script_state, LuaIndex index)
{
   if (script_state == NULL || lua_isnil(script_state, index) || !lua_isuserdata(script_state, index))
      return (memblob){ NULL, 0 };

   uS size = (uS)lua_rawlen(script_state, index);
   return (memblob){ lua_touserdata(script_state, index), size };
}

void Scripting_PushI32(void* script_state, i32 value)
{
   if (script_state == NULL)
      return;

   lua_pushinteger(script_state, (lua_Integer)value);

}

void Scripting_PushF32(void* script_state, f32 value)
{
   if (script_state == NULL)
      return;

   lua_pushnumber(script_state, (lua_Number)value);

}

void Scripting_PushVec4(void* script_state, vec4 value)
{
   if (script_state == NULL)
      return;

   SCRP_Util_PushVector(script_state, value);

}

void Scripting_PushString(void* script_state, const char* value)
{
   if (script_state == NULL || value == NULL)
      return;

   lua_pushstring(script_state, value);

}

void Scripting_PushUserData(void* script_state, memblob value)
{
   if (script_state == NULL || value.data == NULL || value.size == 0)
      return;

   void* ptr = lua_newuserdatauv(script_state, value.size, 0);
   memcpy(ptr, value.data, value.size);

}

int SCRP_LuaWriter(lua_State* script_state, const void* buffer, size_t size, void* data)
{
   if (data == NULL || buffer == NULL)
      return LUA_ERRERR;

   memblob* data_blob = (memblob*)data;

   u8* tmp_ptr = realloc(data_blob->data, data_blob->size + size);
   if (tmp_ptr == NULL)
      return LUA_ERRMEM;

   data_blob->data = tmp_ptr;
   tmp_ptr = data_blob->data + data_blob->size;
   memmove(tmp_ptr, buffer, size);
   data_blob->size += size;

   return 0;
}
