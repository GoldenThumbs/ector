
#include "util/types.h"
// #include "ect_math.h"

#include "core/keymap.h"
#include "core/engine_i.h"
#include "core/renderer_i.h"
#include "core/renderer.h"
#include "core/engine.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

static ENG_EngineGlobal* ENGINE_G;

struct Engine_t
{
   char* app_name;
   Renderer* renderer;
   u64 resize_timer;
   ENG_EngineGlobal internal;
   bool quit;
};

Engine* Engine_Init(EngineDesc* desc, RendererDesc* renderer_desc)
{
   char* app_name = "Ector App";
   char* window_title = "Ector Window";
   i32 width = 1280;
   i32 height = 720;

   if (desc->app_name != NULL)
      app_name = desc->app_name;
   if (desc->window.title != NULL)
      window_title = desc->window.title;
   if (desc->window.width > 0)
      width = desc->window.width;
   if (desc->window.height > 0)
      height = desc->window.height;

   if (!glfwInit())
      abort();

   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   GLFWwindow* window = glfwCreateWindow(width, height, window_title, NULL, NULL);
   if (window == NULL)
   {
      glfwTerminate();
      abort();
   }
   glfwMakeContextCurrent(window);

   Engine* engine = calloc(1, sizeof(Engine));
   engine->app_name = app_name;
   engine->renderer = Renderer_Init(renderer_desc);
   engine->quit = false;

   engine->internal = (ENG_EngineGlobal){ 0 };
   engine->internal.frame_size = (size2i){ width, height };
   engine->internal.window = window;
   ENGINE_G = &engine->internal;

   glfwSetFramebufferSizeCallback(window, ENG_FramebufferSizeCallback);
   glfwSetCursorPosCallback(window, ENG_CursorCallback);
   glfwSetScrollCallback(window, ENG_ScrollCallback);
   glfwSetKeyCallback(window, ENG_KeyCallback);

   return engine;
}

void Engine_Free(Engine* engine)
{
   Renderer_Free(engine->renderer);
   glfwTerminate();
   free(engine);
}

void Engine_Quit(Engine* engine)
{
   engine->quit = true;
}

bool Engine_ShouldQuit(Engine* engine)
{
   ENG_EngineGlobal* eng_glb = &engine->internal;

   eng_glb->input.mouse.position[1] = eng_glb->input.mouse.position[0];
   eng_glb->input.mouse.scroll[1] = eng_glb->input.mouse.scroll[0];

   for (i32 key=0; key<MAX_KEYS; key++)
   {
      eng_glb->input.keyboard.key_state[key].was_down = eng_glb->input.keyboard.key_state[key].is_down;
   }

   glfwPollEvents();

   return (glfwWindowShouldClose(eng_glb->window) || engine->quit);
}

bool Engine_CheckKey(Engine* engine, Key key, KeyAction key_action)
{
   ENG_EngineGlobal eng_glb = engine->internal;

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
   ENG_EngineGlobal eng_glb = engine->internal;

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
   ENG_EngineGlobal eng_glb = engine->internal;
   return eng_glb.input.keyboard.key_state[key];
}

void Engine_SetMouseMode(Engine* engine, MouseMode mouse_mode)
{
   ENG_EngineGlobal eng_glb = engine->internal;
   glfwSetInputMode(eng_glb.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL + mouse_mode);
}

size2i Engine_GetSize(Engine* engine)
{
   ENG_EngineGlobal eng_glb = engine->internal;
   return eng_glb.frame_size;
}

vec2 Engine_GetMousePos(Engine* engine)
{
   ENG_EngineGlobal eng_glb = engine->internal;

   vec2 res = { 0 };
   res.x = (f32)eng_glb.input.mouse.position[0].x;
   res.y = (f32)eng_glb.input.mouse.position[0].y;

   return res;
}

vec2 Engine_GetMouseDelta(Engine* engine)
{
   ENG_EngineGlobal eng_glb = engine->internal;

   vec2 res = { 0 };
   res.x = (f32)(
      eng_glb.input.mouse.position[0].x - eng_glb.input.mouse.position[1].x
   );
   res.y = (f32)(
      eng_glb.input.mouse.position[0].y - eng_glb.input.mouse.position[1].y
   );

   return res;
}

Renderer* Engine_GetRenderer(Engine* engine)
{
   return engine->renderer;
}

void Engine_Render(Engine* engine, vec4 clear_color, ClearTargets clear_targets)
{
   ENG_EngineGlobal eng_glb = engine->internal;

   if (eng_glb.resize_time > 0)
   {
      if (eng_glb.resize_time == engine->resize_timer)
      {
         RNDR_MarkFramebufferDirty(engine->renderer);
         eng_glb.resize_time = 0;
         engine->resize_timer = 0;
      }

      engine->resize_timer = eng_glb.resize_time;
   }
   RNDR_BeginFrame(engine->renderer, clear_color, clear_targets);

   RNDR_RenderFrame(engine->renderer);

   RNDR_FinishFrame(engine->renderer, eng_glb.frame_size);
   glfwSwapBuffers(eng_glb.window);
}

void ENG_FramebufferSizeCallback(GLFWwindow* window, i32 width, i32 height)
{
   ENGINE_G->frame_size.width = width;
   ENGINE_G->frame_size.height = height;
   ENGINE_G->resize_time += 1;
}

static void ENG_CursorCallback(GLFWwindow* window, f64 x, f64 y)
{
   ENGINE_G->input.mouse.position[0].x = x;
   ENGINE_G->input.mouse.position[0].y = y;
}

void ENG_ScrollCallback(GLFWwindow* window, f64 x, f64 y)
{
   ENGINE_G->input.mouse.scroll[0].x = x;
   ENGINE_G->input.mouse.scroll[0].y = y;
}

void ENG_KeyCallback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods)
{
   KeyState key_state = (KeyState){
      .mod_shift = ((mods & GLFW_MOD_SHIFT) == GLFW_MOD_SHIFT),
      .mod_ctrl = ((mods & GLFW_MOD_CONTROL) == GLFW_MOD_CONTROL),
      .mod_alt = ((mods & GLFW_MOD_ALT) == GLFW_MOD_ALT),
      .mod_super = ((mods & GLFW_MOD_SUPER) == GLFW_MOD_SUPER),
      .mod_capslock = ((mods & GLFW_MOD_CAPS_LOCK) == GLFW_MOD_CAPS_LOCK),
      .mod_numlock = ((mods & GLFW_MOD_NUM_LOCK) == GLFW_MOD_NUM_LOCK)
   };
   key_state.was_down = ENGINE_G->input.keyboard.key_state[key].was_down;

   switch (action)
   {
      case GLFW_RELEASE:
         break;
      case GLFW_PRESS:
      case GLFW_REPEAT:
         key_state.is_down = 1;
         break;
      default:
         ENGINE_G->input.keyboard.key_state[key].total_bits = 0;
         return;
   }

   ENGINE_G->input.keyboard.key_state[key] = key_state;
}
