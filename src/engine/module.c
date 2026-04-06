
#include "util/types.h"
#include "util/array.h"
#include "util/keymap.h"
#include "util/files.h"

#include "engine.h"
#include "engine/internal.h"

#include <GLFW/glfw3.h>

//#include <assert.h>
#include <stdlib.h>
#include <string.h>

static eng_EngineGlobal* ENGINE_G;

Engine* Engine_Init(i32 argc, char* argv[], const EngineDesc* desc)
{
   char* app_name = "Ector App";
   char* window_title = app_name;
   i32 width = 1920;
   i32 height = 1080;

   if (desc != NULL)
   {
      app_name = (desc->app_name != NULL) ? desc->app_name : app_name;
      window_title = (desc->window.title != NULL) ? desc->window.title : app_name;
      width = (desc->window.size.width > 0) ? desc->window.size.width : width;
      height = (desc->window.size.height > 0) ? desc->window.size.height : height;

   }

   if (!glfwInit())
   {
      error err = { 0 };
      err.general = ERR_FATAL;
      err.extra = ERR_ENG_INIT_FAILED;
      err.flags |= ERR_FLAG_WINDOW_INIT_FAILED | ERR_FLAG_WINDOW_CREATION_FAILED;

      Util_Log(NULL, ENGINE_LOG_NAME, err, "Window initialization failed! Exiting...");

      abort();

   }

   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   if (desc->window.hidden)
      glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

   GLFWwindow* window = glfwCreateWindow(width, height, window_title, NULL, NULL);

   if (window == NULL)
   {
      error err = { 0 };
      err.general = ERR_FATAL;
      err.extra = ERR_ENG_INIT_FAILED;
      err.flags |= ERR_FLAG_WINDOW_CREATION_FAILED;

      glfwTerminate();
      Util_Log(NULL, ENGINE_LOG_NAME, err, "Window creation failed! Exiting...");

      abort();

   }

   glfwMakeContextCurrent(window);

   Engine* engine = malloc(sizeof(Engine));
   if (engine == NULL)
   {
      error err = { 0 };
      err.general = ERR_FATAL;
      err.extra = ERR_ENG_INIT_FAILED;
      err.flags |= ERR_FLAG_ENGINE_ALLOC_FAILED;

      glfwTerminate();
      Util_Log(NULL, ENGINE_LOG_NAME, err, "Engine allocation failed! Exiting...");

      abort();

   }

   char* app_path = (argc >= 1) ? argv[0] : "";
   uS app_path_size = strnlen(app_path, ECT_PATH_MAX) + 1;
   uS app_name_size = strnlen(app_name, ECT_NAME_MAX) + 1;
   uS window_title_size = strnlen(window_title, ECT_NAME_MAX) + 1;
   window_title_size = (strncmp(window_title, app_name, ECT_NAME_MAX) == 0) ? 0 : window_title_size;

   engine->app_path_ofs = 0;
   engine->app_name_ofs = engine->app_path_ofs + app_path_size;
   engine->window_title_ofs = engine->app_name_ofs + ((window_title_size > 0) ? app_name_size : 0);
   engine->exit_requested = false;

   engine->engine_string_size = app_path_size + app_name_size + window_title_size;
   engine->engine_strings = malloc(engine->engine_string_size);
   if (engine->engine_strings != NULL)
   {
      char* engine_app_path = engine->engine_strings + engine->app_path_ofs;
      char* engine_app_name = engine->engine_strings + engine->app_name_ofs;
      char* engine_window_title = engine->engine_strings + engine->window_title_ofs;

      strncpy(engine_app_path, app_path, app_path_size);
      strncpy(engine_app_name, app_name, app_name_size);
      if (window_title_size != 0)
         strncpy(engine_window_title, window_title, window_title_size);

   } else {
      engine->engine_string_size = 0;
      engine->app_path_ofs = 0;
      engine->app_name_ofs = 0;
      engine->window_title_ofs = 0;

   }

   engine->modules = NEW_ARRAY_N(Module, 2);

   engine->internal = (eng_EngineGlobal){ 0 };
   engine->internal.up_time = 0.0;
   engine->internal.window = window;
   ENGINE_G = &engine->internal;

   glfwGetFramebufferSize(window,
      &ENGINE_G->frame_size.width,
      &ENGINE_G->frame_size.height
   );

   glfwSetFramebufferSizeCallback(window, ENG_FramebufferSizeCallback);
   glfwSetCursorPosCallback(window, ENG_CursorCallback);
   glfwSetScrollCallback(window, ENG_ScrollCallback);
   glfwSetKeyCallback(window, ENG_KeyCallback);
   glfwSetMouseButtonCallback(window, ENG_ButtonCallback);

   return engine;
}

void Engine_Free(Engine* engine)
{
   if (engine == NULL)
      return;

   if (engine->engine_strings != NULL)
      free(engine->engine_strings);

   u32 module_count = Util_ArrayLength(engine->modules);
   for (u32 i = 0; i < module_count; i++)
   {
      Module* m = engine->modules + (uS)(module_count - i - 1);

      if (m == NULL)
         break;

      error err = m->mod_free(m, engine);
      if (err.general != ERR_OK)
         abort();

   }
   
   FREE_ARRAY(engine->modules);

   glfwTerminate();
   free(engine);

}

void Engine_RequestExit(Engine* engine)
{
   if (engine == NULL)
      return;

   engine->exit_requested = true;
}

bool Engine_CheckExitConditions(Engine* engine)
{
   if (engine == NULL)
      return true;

   eng_EngineGlobal* eng_glb = &engine->internal;

   eng_glb->input.mouse.position[1] = eng_glb->input.mouse.position[0];
   eng_glb->input.mouse.scroll.x = 0.0;
   eng_glb->input.mouse.scroll.y = 0.0;

   for (i32 key=0; key<MAX_KEYS; key++)
   {
      eng_glb->input.keyboard.key_state[key].was_down = eng_glb->input.keyboard.key_state[key].is_down;
   }

   for (i32 button=0; button<MAX_MOUSE_BUTTONS; button++)
   {
      eng_glb->input.mouse.button_state[button].was_down = eng_glb->input.mouse.button_state[button].is_down;
   }

   f64 new_time = glfwGetTime();
   eng_glb->frame_delta = new_time - eng_glb->up_time;
   eng_glb->up_time = new_time;

   glfwPollEvents();

   return (glfwWindowShouldClose(eng_glb->window) || engine->exit_requested);
}

void* Engine_FetchModule(Engine* engine, const char* name)
{
   if (engine == NULL)
      return NULL;

   u32 module_count = Util_ArrayLength(engine->modules);
   for (u32 i=0; i<module_count; i++)
   {
      Module* m = engine->modules + (uS)i;
      if (m == NULL)
         break;

      if (strncmp(m->name, name, 64) == 0)
      {
         return m->data;
      }

   }

   return NULL;
}

void Engine_RegisterModule(Engine* engine, Module module)
{
   if (engine == NULL)
      return;

   ADD_BACK_ARRAY(engine->modules, module);
   Module* m = engine->modules + (uS)(Util_ArrayLength(engine->modules) - 1);
   error err = module.mod_init(m, engine);

   if (err.general != ERR_OK)
      abort();
   
}

const char* Engine_GetAppPath(Engine* engine)
{
   if (engine == NULL || engine->engine_strings == NULL)
      return NULL;

   return engine->engine_strings + engine->app_path_ofs;
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
   ENGINE_G->input.mouse.scroll.x = x;
   ENGINE_G->input.mouse.scroll.y = y;
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

void ENG_ButtonCallback(GLFWwindow* window, i32 button, i32 action, i32 mods)
{
   KeyState button_state = (KeyState){
      .mod_shift = ((mods & GLFW_MOD_SHIFT) == GLFW_MOD_SHIFT),
      .mod_ctrl = ((mods & GLFW_MOD_CONTROL) == GLFW_MOD_CONTROL),
      .mod_alt = ((mods & GLFW_MOD_ALT) == GLFW_MOD_ALT),
      .mod_super = ((mods & GLFW_MOD_SUPER) == GLFW_MOD_SUPER),
      .mod_capslock = ((mods & GLFW_MOD_CAPS_LOCK) == GLFW_MOD_CAPS_LOCK),
      .mod_numlock = ((mods & GLFW_MOD_NUM_LOCK) == GLFW_MOD_NUM_LOCK)
   };
   button_state.was_down = ENGINE_G->input.mouse.button_state[button].was_down;

   switch (action)
   {
      case GLFW_RELEASE:
         break;
      case GLFW_PRESS:
      case GLFW_REPEAT:
         button_state.is_down = 1;
         break;
      default:
         ENGINE_G->input.mouse.button_state[button].total_bits = 0;
         return;
   }

   ENGINE_G->input.mouse.button_state[button] = button_state;
}
