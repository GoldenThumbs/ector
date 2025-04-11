
#include "util/types.h"
// #include "util/math.h"
#include "util/keymap.h"
#include "module_glue.h"

#include "engine.h"
#include "engine/internal.h"

#include <GLFW/glfw3.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

static eng_EngineGlobal* ENGINE_G;

Engine* Engine_Init(EngineDesc* desc)
{
   char* app_name = (desc->app_name == NULL) ? "Ector App" : desc->app_name;
   char* window_title = app_name;
   i32 width = 1280;
   i32 height = 720;

   if (desc->window.title != NULL)
      window_title = desc->window.title;
   if (desc->window.width > 0)
      width = desc->window.width;
   if (desc->window.height > 0)
      height = desc->window.height;

   if (!glfwInit())
      abort();

   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   GLFWwindow* window = glfwCreateWindow(width, height, window_title, NULL, NULL);

   if (window == NULL)
   {
      glfwTerminate();
      abort();
   }

   glfwMakeContextCurrent(window);

   Engine* engine = malloc(sizeof(Engine));
   engine->app_name = app_name;
   engine->quit = false;

   engine->internal = (eng_EngineGlobal){ 0 };
   engine->internal.frame_size = (size2i){ width, height };
   engine->internal.up_time = 0.0;
   engine->internal.window = window;
   ENGINE_G = &engine->internal;

   glfwSetFramebufferSizeCallback(window, ENG_FramebufferSizeCallback);
   glfwSetCursorPosCallback(window, ENG_CursorCallback);
   glfwSetScrollCallback(window, ENG_ScrollCallback);
   glfwSetKeyCallback(window, ENG_KeyCallback);

   error err = { 0 };
   engine->graphics_context = MOD_InitGraphics(&err);

   if (err.general == ERR_FATAL)
   {
      glfwTerminate();
      abort();
   }

   return engine;
}

void Engine_Free(Engine* engine)
{
   glfwTerminate();
   MOD_FreeGraphics(engine->graphics_context);
   free(engine);
}

void Engine_Quit(Engine* engine)
{
   engine->quit = true;
}

bool Engine_ShouldQuit(Engine* engine)
{
   eng_EngineGlobal* eng_glb = &engine->internal;

   eng_glb->input.mouse.position[1] = eng_glb->input.mouse.position[0];
   eng_glb->input.mouse.scroll[1] = eng_glb->input.mouse.scroll[0];

   for (i32 key=0; key<MAX_KEYS; key++)
   {
      eng_glb->input.keyboard.key_state[key].was_down = eng_glb->input.keyboard.key_state[key].is_down;
   }

   f64 new_time = glfwGetTime();
   eng_glb->frame_delta = new_time - eng_glb->up_time;
   eng_glb->up_time = new_time;

   glfwPollEvents();

   return (glfwWindowShouldClose(eng_glb->window) || engine->quit);
}

void ENG_FramebufferSizeCallback(GLFWwindow* window, i32 width, i32 height)
{
   ENGINE_G->frame_size.width = width;
   ENGINE_G->frame_size.height = height;
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
