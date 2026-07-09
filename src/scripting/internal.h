#ifndef WEB_SCRIPTING_INTERNAL
#define WEB_SCRIPTING_INTERNAL

#include "util/types.h"
#include "util/files.h"

#include "scripting.h"

#include <lua.h>

#include <stddef.h>

#define ENGINE_DATA "ECT_ENGINE"

typedef struct scrp_Script_t
{
   memblob code;
   const char* name;

   handle compare;
   u16 next_freed;

} scrp_Script;

struct ScriptHandler_t
{
   Engine* engine;
   lua_State* script_state;

   scrp_Script* scripts;
   u16 freed_script_root;

   u16 module_count;

};

static inline bool SCRP_IsScriptValid(scrp_Script script, Script res_script)
{
   if (script.compare.id != res_script.id)
   {
      error err = { 0 };
      err.general = ERR_LEVEL_ERROR;
      err.extra = ERR_SCRIPT_INVALID_HANDLE;

      Util_Log(NULL, SCRIPTING_MODULE, err, "Invalid handle! Handle ID: %u", res_script.id);

      return false;
   }

   return true;
}

void SCRP_PushVariable(lua_State* script_state, memblob value, u8 variable_type);
int SCRP_LuaWriter(lua_State* script_state, const void* ptr, size_t size, void* data);

#endif
