#ifndef WEB_SCRIPTING_H
#define WEB_SCRIPTING_H

#include "util/types.h"
#include "engine.h"

#define SCRIPTING_MODULE "Scripting"

enum {
   ERR_SCRIPT_NO_ENGINE = 0x8001,
   ERR_SCRIPT_LUA_INIT_FAILURE,
   ERR_SCRIPT_LUA_COMPILE_FAILURE,
   ERR_SCRIPT_INVALID_HANDLE,
   ERR_SCRIPT_INVALID_FUNCTION_INPUT,
   ERR_SCRIPT_RUNTIME_ERROR
};

// script compile error flags
#define ERR_FLAG_INVALID_SYNTAX (1u << 0)

// function input error flags
#define ERR_FLAG_INVALID_VECTOR_KEY (1u << 0)
#define ERR_FLAG_INVALID_VECTOR_VALUE (1u << 1)
#define ERR_FLAG_INPUT_NOT_VECTOR (1u << 2)
#define ERR_FLAG_TOO_MANY_INPUTS (1u << 3)
#define ERR_FLAG_INVALID_USER_DATA (1u << 4)

enum {
   SCRP_VARIABLE_I32 = 0,
   SCRP_VARIABLE_F32,
   SCRP_VARIABLE_VEC4,
   SCRP_VARIABLE_STRING,
   SCRP_VARIABLE_USERDATA

};

typedef handle Script;
typedef int ReturnCount;
typedef int LuaIndex;

typedef ReturnCount (*LuaCFunc)(void* script_state);
typedef void (*ModuleRegisterFunc)(void* script_state, void* module_data);

typedef struct ScriptHandler_t ScriptHandler;

typedef struct EnumPair_t
{
   const char* name;
   i32 value;

} EnumPair;

typedef struct NamedScriptFunction_t
{
   const char* name;
   LuaCFunc function;

} NamedScriptFunction;

typedef struct ScriptModuleDesc_t
{
   const char* name;
   void* data;
   ModuleRegisterFunc register_func;

} ScriptModuleDesc;

ScriptHandler* Scripting_InitHandler(Engine* engine);
void Scripting_FreeHandler(ScriptHandler* script_handler);

Script Scripting_CreateScriptFromCode(ScriptHandler* script_handler, const char* lua_code, const char* script_name);
// Script Scripting_CreateScriptFromByteCode(ScriptHandler* script_handler, const void* lua_byte_code);
Script Scripting_LoadScriptFromFile(ScriptHandler* script_handler, const char* lua_file_path);
void Scripting_FreeScript(ScriptHandler* script_handler, Script res_script);
error Scripting_RunScript(ScriptHandler* script_handler, Script res_script);

bool Scripting_FieldExists(void* script_state, const char* name);

void Scripting_AddGlobalVariable(ScriptHandler* script_handler, const char* name, memblob value, u8 variable_type);
void Scripting_AddGlobalFunction(ScriptHandler* script_handler, const char* name, LuaCFunc function);
void Scripting_AddModule(ScriptHandler* script_handler, ScriptModuleDesc desc);

void Scripting_AddFunctionsToTable(void* script_state, NamedScriptFunction named_functions[], u32 function_count);
void Scripting_AddEnumToTable(void* script_state, const char* enum_name, EnumPair enum_pairs[], u32 enum_count);

Engine* Scripting_GetEngine(void* script_state);
void* Scripting_GetModuleData(void* script_state, const char* module_name);

i32 Scripting_GetI32(void* script_state, LuaIndex index);
f32 Scripting_GetF32(void* script_state, LuaIndex index);
vec4 Scripting_GetVec4(void* script_state, LuaIndex index);
const char* Scripting_GetString(void* script_state, LuaIndex index);
memblob Scripting_GetUserData(void* script_state, LuaIndex index);

void Scripting_PushI32(void* script_state, i32 value);
void Scripting_PushF32(void* script_state, f32 value);
void Scripting_PushVec4(void* script_state, vec4 value);
void Scripting_PushString(void* script_state, const char* value);
void Scripting_PushUserData(void* script_state, memblob value);

#endif
