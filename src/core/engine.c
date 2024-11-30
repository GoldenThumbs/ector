#include "GLFW/glfw3.h"
#include "core/keymap.h"
#include "ect_types.h"
// #include "ect_math.h"

#include "core/engine_i.h"
#include "core/engine.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

void ECT_FramebufferSizeCallback(GLFWwindow* window, i32 width, i32 height)
{
   ENGINE_G.frame_width = width;
   ENGINE_G.frame_height = height;
}

static void ECT_CursorCallback(GLFWwindow* window, f64 x, f64 y)
{
   ENGINE_G.input.cursor_pos_old[0] = ENGINE_G.input.cursor_pos[0];
   ENGINE_G.input.cursor_pos_old[1] = ENGINE_G.input.cursor_pos[1];
   ENGINE_G.input.cursor_pos[0] = x;
   ENGINE_G.input.cursor_pos[1] = y;
}

void ECT_ScrollCallback(GLFWwindow* window, f64 x, f64 y)
{
   ENGINE_G.input.scroll_move_old[0] = ENGINE_G.input.scroll_move[0];
   ENGINE_G.input.scroll_move_old[1] = ENGINE_G.input.scroll_move[1];
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

EctEngine EctInitEngine(EctEngineDesc* desc)
{
   char* app_name = "Ector App";
   char* window_title = "Ector Window";
   i32 width = 1280;
   i32 height = 720;

   if (desc->app_name != NULL)
      app_name = (char*)desc->app_name;
   if (desc->window.title != NULL)
      window_title = (char*)desc->window.title;
   if (desc->window.width > 0)
      width = desc->window.width;
   if (desc->window.height > 0)
      height = desc->window.height;

   assert(glfwInit());

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

   ENGINE_G.frame_width = width;
   ENGINE_G.frame_height = height;

   EctEngine engine = { .app_name = app_name, .quit = false };
   return engine;
}

void EctFreeEngine(void)
{
   glfwTerminate();
}

bool EctShouldQuit(EctEngine* engine)
{
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

void EctGetFrameSize(i32* width, i32* height)
{
   *width = ENGINE_G.frame_width;
   *height = ENGINE_G.frame_height;
}
