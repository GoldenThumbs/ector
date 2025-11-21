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
      case KEY_JUST_RELEASED:
         return (!key_state.is_down && key_state.was_down);
      case KEY_JUST_PRESSED:
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

void Engine_SetRawMouseInput(Engine* engine, bool raw_mouse_input)
{
   eng_EngineGlobal eng_glb = engine->internal;
   
   if (glfwRawMouseMotionSupported())
   {
      glfwSetInputMode(
         eng_glb.window,
         GLFW_RAW_MOUSE_MOTION,
         raw_mouse_input ? GLFW_TRUE : GLFW_FALSE
      );

   }

}

void Engine_SetMouseMode(Engine* engine, MouseMode mouse_mode)
{
   eng_EngineGlobal* eng_glb = &engine->internal;

   glfwSetInputMode(eng_glb->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL + mouse_mode);

   f64 mouse_pos[2] = { 0 };
   glfwGetCursorPos(eng_glb->window, &mouse_pos[0], &mouse_pos[1]);

   eng_glb->input.mouse.position[1].x = eng_glb->input.mouse.position[0].x = mouse_pos[0];
   eng_glb->input.mouse.position[1].y = eng_glb->input.mouse.position[0].y = mouse_pos[1];

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

vec2 Engine_GetMouseScroll(Engine* engine)
{
   eng_EngineGlobal eng_glb = engine->internal;

   vec2 res = { 0 };
   res.x = (f32)(eng_glb.input.mouse.scroll[0].x - eng_glb.input.mouse.scroll[1].x);
   res.y = (f32)(eng_glb.input.mouse.scroll[0].y - eng_glb.input.mouse.scroll[1].y);

   return res;
}

bool Engine_CheckMouseButton(Engine* engine, MouseButton button, KeyAction button_action)
{
   eng_EngineGlobal eng_glb = engine->internal;

   KeyState button_state = eng_glb.input.mouse.button_state[button];
   switch(button_action)
   {
      case KEY_IS_UP:
         return !button_state.is_down;
      case KEY_IS_DOWN:
         return button_state.is_down;
      case KEY_JUST_RELEASED:
         return (!button_state.is_down && button_state.was_down);
      case KEY_JUST_PRESSED:
         return (button_state.is_down && !button_state.was_down);
      default:
         return false;
   }
}
bool Engine_CheckMouseButtonAdvanced(Engine* engine, MouseButton button, KeyAction button_action, KeyModifiers modifiers)
{
   eng_EngineGlobal eng_glb = engine->internal;

   bool ignore_mods = false;
   bool mods_true = true;
   KeyState button_state = eng_glb.input.mouse.button_state[button];

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
         mod_bits = (button_state.total_bits & match_against);
         break;

      case KEY_MATCH_DISABLED_MODS:
         match_against = ~modifiers.total_bits & mask;
         mod_bits = (button_state.total_bits & match_against);
         break;

      case KEY_MATCH_EXACT_MODS:
         match_against = modifiers.total_bits & mask;
         mod_bits = (button_state.total_bits & mask);
         break;

      default:
         mods_true = false;
         ignore_mods = true;
         break;
   }

   if (!ignore_mods)
      mods_true = (mod_bits == match_against);

   return mods_true && Engine_CheckMouseButton(engine, button, button_action);
}

KeyState Engine_GetMouseButtonState(Engine* engine, MouseButton button)
{
   eng_EngineGlobal eng_glb = engine->internal;
   return eng_glb.input.mouse.button_state[button];
}

