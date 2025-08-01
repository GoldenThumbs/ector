#ifndef ENG_INTERNAL
#define ENG_INTERNAL

#include "util/types.h"
#include "util/keymap.h"

#include "engine.h"

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#define MAX_TEXTBUF 128
#define MAX_KEYS 512
#define MAX_MOUSE_BUTTONS 8

typedef struct eng_EngineGlobal_t
{
   struct {
      struct {
         struct {
            f64 x, y;
         } position[2];

         struct {
            f64 x, y;
         } scroll[2];

         KeyState button_state[MAX_MOUSE_BUTTONS];

      } mouse;

      struct {
         u32 text_buffer[MAX_TEXTBUF];
         KeyState key_state[MAX_KEYS];
      } keyboard;

   } input;

   GLFWwindow* window;
   f64 up_time;
   f64 frame_delta;
   resolution2d frame_size;
} eng_EngineGlobal;

struct Engine_t
{
   char* app_name;
   eng_EngineGlobal internal;
   bool exit_requested;

   Module* modules;
};

void ENG_FramebufferSizeCallback(GLFWwindow* window, i32 width, i32 height);
static void ENG_CursorCallback(GLFWwindow* window, f64 x, f64 y);
void ENG_ScrollCallback(GLFWwindow* window, f64 x, f64 y);
void ENG_KeyCallback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods);
void ENG_ButtonCallback(GLFWwindow* window, i32 button, i32 action, i32 mods);

#endif
