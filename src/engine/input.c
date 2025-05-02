#include "util/types.h"
#include "util/keymap.h"

#include "engine.h"
#include "engine/internal.h"

#include <GLFW/glfw3.h>



bool Engine_CheckKey(Engine* engine, Key key, KeyAction key_action)
{
   eng_EngineGlobal eng_glb = engine->internal;

   KeyState key_state = eng_glb.input.keyboard.key_state[key];
   switch(key_action)
   {
      case KEY_IS_UP:
         return !key_state.is_down;
      case KEY_IS_DOWN:
         return key_state.is_down;
      case KEY_JUST_UP:
         return (!key_state.is_down && key_state.was_down);
      case KEY_JUST_DOWN:
         return (key_state.is_down && !key_state.was_down);
      default:
         return false;
   }
}

bool Engine_CheckKeyAdvanced(Engine* engine, Key key, KeyAction key_action, KeyModifiers modifiers)
{
   eng_EngineGlobal eng_glb = engine->internal;

   bool ignore_mods = false;
   bool mods_true = true;
   KeyState key_state = eng_glb.input.keyboard.key_state[key];

   const u8 mask = ~((KeyModifiers){ .matching_type = 3 }).total_bits;
   u8 mod_bits = 0;
   u8 match_against = 1;

   switch(modifiers.matching_type)
   {
      case KEY_IGNORE_MODS:
         ignore_mods = true;
         break;

      case KEY_MATCH_ENABLED_MODS:
         match_against = modifiers.total_bits & mask;
         mod_bits = (key_state.total_bits & match_against);
         break;

      case KEY_MATCH_DISABLED_MODS:
         match_against = ~modifiers.total_bits & mask;
         mod_bits = (key_state.total_bits & match_against);
         break;

      case KEY_MATCH_EXACT_MODS:
         match_against = modifiers.total_bits & mask;
         mod_bits = (key_state.total_bits & mask);
         break;

      default:
         mods_true = false;
         ignore_mods = true;
         break;
   }

   if (!ignore_mods)
      mods_true = (mod_bits == match_against);

   return mods_true && Engine_CheckKey(engine, key, key_action);
}

KeyState Engine_KeyState(Engine* engine, Key key)
{
   eng_EngineGlobal eng_glb = engine->internal;
   return eng_glb.input.keyboard.key_state[key];
}

void Engine_SetMouseMode(Engine* engine, MouseMode mouse_mode)
{
   eng_EngineGlobal eng_glb = engine->internal;
   glfwSetInputMode(eng_glb.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL + mouse_mode);
}

vec2 Engine_GetMousePos(Engine* engine)
{
   eng_EngineGlobal eng_glb = engine->internal;

   vec2 res = { 0 };
   res.x = (f32)eng_glb.input.mouse.position[0].x;
   res.y = (f32)eng_glb.input.mouse.position[0].y;

   return res;
}

vec2 Engine_GetMouseDelta(Engine* engine)
{
   eng_EngineGlobal eng_glb = engine->internal;

   vec2 res = { 0 };
   res.x = (f32)(eng_glb.input.mouse.position[0].x - eng_glb.input.mouse.position[1].x);
   res.y = (f32)(eng_glb.input.mouse.position[0].y - eng_glb.input.mouse.position[1].y);

   return res;
}
