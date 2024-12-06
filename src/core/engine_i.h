#ifndef ECT_ENGINEI_H
#define ECT_ENGINEI_H

#include "ect_types.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define ECT_MAX_TEXTBUF 128
#define ECT_MAX_KEYS 512

static struct ECT_EngineGlobal_t
{
   struct Input_t
   {
      u8 keyboard_state[ECT_MAX_KEYS];
      f64 cursor_pos_old[2];
      f64 cursor_pos[2];
      f64 scroll_move_old[2];
      f64 scroll_move[2];
      u32 text_buffer_n;
      u32 text_buffer[ECT_MAX_TEXTBUF];
      u32 _padding;
   } input;
   GLFWwindow* window;
   i32 frame_width, frame_height;
} ENGINE_G = { 0 };

void ECT_FramebufferSizeCallback(GLFWwindow* window, i32 width, i32 height);
static void ECT_CursorCallback(GLFWwindow* window, f64 x, f64 y);
void ECT_ScrollCallback(GLFWwindow* window, f64 x, f64 y);
void ECT_KeyCallback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods);

#endif
