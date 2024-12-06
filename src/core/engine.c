#include "GLFW/glfw3.h"
#include "core/keymap.h"
#include "ect_types.h"
// #include "ect_math.h"

#include "core/engine_i.h"
#include "core/renderer.h"
#include "core/engine.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

struct EctEngine_t
{
   char* app_name;
   EctRenderer* renderer;
   bool quit;
};

void ECT_FramebufferSizeCallback(GLFWwindow* window, i32 width, i32 height)
{
   ENGINE_G.frame_width = width;
   ENGINE_G.frame_height = height;
}

static void ECT_CursorCallback(GLFWwindow* window, f64 x, f64 y)
{
   ENGINE_G.input.cursor_pos[0] = x;
   ENGINE_G.input.cursor_pos[1] = y;
}

void ECT_ScrollCallback(GLFWwindow* window, f64 x, f64 y)
{
   ENGINE_G.input.scroll_move[0] = x;
   ENGINE_G.input.scroll_move[1] = y;
}

void ECT_KeyCallback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods)
{
   u8 key_state = (u8)mods << 2;
   key_state |= ENGINE_G.input.keyboard_state[key] & ECT_KEY_WAS_DOWN_BIT;

   switch (action)
   {
      case GLFW_RELEASE:
         break;
      case GLFW_PRESS:
         key_state |= ECT_KEY_IS_DOWN_BIT;
         break;
      case GLFW_REPEAT:
         key_state |= ECT_KEY_IS_DOWN_BIT;
         break;
      default:
         ENGINE_G.input.keyboard_state[key] = 0;
         return;
   }

   ENGINE_G.input.keyboard_state[key] = key_state;
}

EctEngine* EctInit(EctEngineDesc* desc)
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

   assert(glfwInit());

   ENGINE_G.frame_width = width;
   ENGINE_G.frame_height = height;

   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   ENGINE_G.window = glfwCreateWindow(width, height, window_title, NULL, NULL);
   if (!ENGINE_G.window)
   {
      glfwTerminate();
      abort();
   }
   glfwMakeContextCurrent(ENGINE_G.window);

   glfwSetFramebufferSizeCallback(ENGINE_G.window, ECT_FramebufferSizeCallback);
   glfwSetCursorPosCallback(ENGINE_G.window, ECT_CursorCallback);
   glfwSetScrollCallback(ENGINE_G.window, ECT_ScrollCallback);
   glfwSetKeyCallback(ENGINE_G.window, ECT_KeyCallback);

   EctEngine* engine = (EctEngine*)calloc(1, sizeof(EctEngine));
   engine->app_name = app_name;
   engine->renderer = EctRendererInit();
   engine->quit = false;

   return engine;
}

void EctFree(EctEngine* engine)
{
   glfwTerminate();
   EctRendererFree(engine->renderer);
   free(engine);
}

void EctQuit(EctEngine* engine)
{
   engine->quit = true;
}

bool EctShouldQuit(EctEngine* engine)
{
   ENGINE_G.input.cursor_pos_old[0] = ENGINE_G.input.cursor_pos[0];
   ENGINE_G.input.cursor_pos_old[1] = ENGINE_G.input.cursor_pos[1];

   ENGINE_G.input.scroll_move_old[0] = ENGINE_G.input.scroll_move[0];
   ENGINE_G.input.scroll_move_old[1] = ENGINE_G.input.scroll_move[1];

   for (i32 key=0; key<ECT_MAX_KEYS; key++)
   {
      ENGINE_G.input.keyboard_state[key] &= ENGINE_G.input.keyboard_state[key] & ~ECT_KEY_WAS_DOWN_BIT;
      ENGINE_G.input.keyboard_state[key] |= (ENGINE_G.input.keyboard_state[key] & ECT_KEY_IS_DOWN_BIT) << 1;
   }

   glfwPollEvents();
   glfwSwapBuffers(ENGINE_G.window);

   return (glfwWindowShouldClose(ENGINE_G.window) || engine->quit);
}

bool EctCheckKey(EctKey key, u8 desired_state, bool exclusive)
{
   u8 mask = (exclusive) ? ECT_KEY_BASE_MASK : desired_state;

   return ((ENGINE_G.input.keyboard_state[key] & mask) == desired_state);
}

bool EctCheckKeyMods(EctKey key, u8 desired_state, u8 modifiers, bool exclusive)
{
   u8 mask = (exclusive) ? ECT_KEY_BASE_MASK : desired_state;

   return ((ENGINE_G.input.keyboard_state[key] & (mask | modifiers)) == (desired_state | modifiers));
}

u8 EctKeyState(EctKey key)
{
   return ENGINE_G.input.keyboard_state[key];
}

void EctSetMouseMode(EctMouseMode mouse_mode)
{
   glfwSetInputMode(ENGINE_G.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL + mouse_mode);
}

frame EctGetSize(void)
{
   return (frame){ ENGINE_G.frame_width, ENGINE_G.frame_height };
}

vec2 EctGetMousePos(void)
{
   vec2 res = { 0 };
   res.x = (f32)ENGINE_G.input.cursor_pos[0];
   res.y = (f32)ENGINE_G.input.cursor_pos[1];

   return res;
}

vec2 EctGetMouseDelta(void)
{
   vec2 res = { 0 };
   res.x = (f32)(ENGINE_G.input.cursor_pos[0] - ENGINE_G.input.cursor_pos_old[0]);
   res.y = (f32)(ENGINE_G.input.cursor_pos[1] - ENGINE_G.input.cursor_pos_old[1]);

   return res;
}

EctRenderer* EctGetRenderer(EctEngine* engine)
{
   return engine->renderer;
}

sg_swapchain EctGetSwapchain(EctEngine* engine)
{
   return EctRendererGetSwapchain(engine->renderer);
}
