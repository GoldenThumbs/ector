#ifndef ECT_KEYMAP_H
#define ECT_KEYMAP_H

#define ECT_KEY_IS_DOWN_BIT   (1u<<0)
#define ECT_KEY_WAS_DOWN_BIT  (1u<<1)
#define ECT_KEY_MOD_SHIFT     (1u<<2)
#define ECT_KEY_MOD_CONTROL   (1u<<3)
#define ECT_KEY_MOD_ALT       (1u<<4)
#define ECT_KEY_MOD_SUPER     (1u<<5)
#define ECT_KEY_MOD_CAPS_LOCK (1u<<6)
#define ECT_KEY_MOD_NUM_LOCK  (1u<<7)

#define ECT_KEY_BASE_MASK (ECT_KEY_IS_DOWN_BIT | ECT_KEY_WAS_DOWN_BIT)
#define ECT_KEY_IS_DOWN       ECT_KEY_IS_DOWN_BIT
#define ECT_KEY_HELD      (ECT_KEY_IS_DOWN_BIT | ECT_KEY_WAS_DOWN_BIT)
#define ECT_KEY_IS_UP         ECT_KEY_WAS_DOWN_BIT


typedef enum EctKey_t {
   ECT_KEY_UNKNOWN = 0,

   // --< ALPHANUMERIC

   ECT_KEY_APOSTROPHE    = 39,
   ECT_KEY_COMMA         = 44,
   ECT_KEY_MINUS         = 45,
   ECT_KEY_PERIOD        = 46,
   ECT_KEY_SLASH         = 47,
   ECT_KEY_0             = 48,
   ECT_KEY_1             = 49,
   ECT_KEY_2             = 50,
   ECT_KEY_3             = 51,
   ECT_KEY_4             = 52,
   ECT_KEY_5             = 53,
   ECT_KEY_6             = 54,
   ECT_KEY_7             = 55,
   ECT_KEY_8             = 56,
   ECT_KEY_9             = 57,
   ECT_KEY_SEMICOLON     = 59,
   ECT_KEY_EQUAL         = 61,
   ECT_KEY_A             = 65,
   ECT_KEY_B             = 66,
   ECT_KEY_C             = 67,
   ECT_KEY_D             = 68,
   ECT_KEY_E             = 69,
   ECT_KEY_F             = 70,
   ECT_KEY_G             = 71,
   ECT_KEY_H             = 72,
   ECT_KEY_I             = 73,
   ECT_KEY_J             = 74,
   ECT_KEY_K             = 75,
   ECT_KEY_L             = 76,
   ECT_KEY_M             = 77,
   ECT_KEY_N             = 78,
   ECT_KEY_O             = 79,
   ECT_KEY_P             = 80,
   ECT_KEY_Q             = 81,
   ECT_KEY_R             = 82,
   ECT_KEY_S             = 83,
   ECT_KEY_T             = 84,
   ECT_KEY_U             = 85,
   ECT_KEY_V             = 86,
   ECT_KEY_W             = 87,
   ECT_KEY_X             = 88,
   ECT_KEY_Y             = 89,
   ECT_KEY_Z             = 90,
   ECT_KEY_LEFT_BRACKET  = 91,
   ECT_KEY_BACKSLASH     = 92,
   ECT_KEY_RIGHT_BRACKET = 93,
   ECT_KEY_GRAVE_ACCENT  = 96,

   // --< FUNCTION

   ECT_KEY_SPACE         = 32,
   ECT_KEY_ESCAPE        = 256,
   ECT_KEY_ENTER         = 257,
   ECT_KEY_TAB           = 258,
   ECT_KEY_BACKSPACE     = 259,
   ECT_KEY_INSERT        = 260,
   ECT_KEY_DELETE        = 261,
   ECT_KEY_RIGHT         = 262,
   ECT_KEY_LEFT          = 263,
   ECT_KEY_DOWN          = 264,
   ECT_KEY_UP            = 265,
   ECT_KEY_PAGE_UP       = 266,
   ECT_KEY_PAGE_DOWN     = 267,
   ECT_KEY_HOME          = 268,
   ECT_KEY_END           = 269,
   ECT_KEY_CAPS_LOCK     = 280,
   ECT_KEY_SCROLL_LOCK   = 281,
   ECT_KEY_NUM_LOCK      = 282,
   ECT_KEY_PRINT_SCREEN  = 283,
   ECT_KEY_PAUSE         = 284,
   ECT_KEY_F1            = 290,
   ECT_KEY_F2            = 291,
   ECT_KEY_F3            = 292,
   ECT_KEY_F4            = 293,
   ECT_KEY_F5            = 294,
   ECT_KEY_F6            = 295,
   ECT_KEY_F7            = 296,
   ECT_KEY_F8            = 297,
   ECT_KEY_F9            = 298,
   ECT_KEY_F10           = 299,
   ECT_KEY_F11           = 300,
   ECT_KEY_F12           = 301,
   ECT_KEY_F13           = 302,
   ECT_KEY_F14           = 303,
   ECT_KEY_F15           = 304,
   ECT_KEY_F16           = 305,
   ECT_KEY_F17           = 306,
   ECT_KEY_F18           = 307,
   ECT_KEY_F19           = 308,
   ECT_KEY_F20           = 309,
   ECT_KEY_F21           = 310,
   ECT_KEY_F22           = 311,
   ECT_KEY_F23           = 312,
   ECT_KEY_F24           = 313,
   ECT_KEY_F25           = 314,
   ECT_KEY_LEFT_SHIFT    = 340,
   ECT_KEY_LEFT_CONTROL  = 341,
   ECT_KEY_LEFT_ALT      = 342,
   ECT_KEY_LEFT_SUPER    = 343,
   ECT_KEY_RIGHT_SHIFT   = 344,
   ECT_KEY_RIGHT_CONTROL = 345,
   ECT_KEY_RIGHT_ALT     = 346,
   ECT_KEY_RIGHT_SUPER   = 347,
   ECT_KEY_MENU          = 348,

   // --< KEYPAD

   ECT_KEY_KP_0          = 320,
   ECT_KEY_KP_1          = 321,
   ECT_KEY_KP_2          = 322,
   ECT_KEY_KP_3          = 323,
   ECT_KEY_KP_4          = 324,
   ECT_KEY_KP_5          = 325,
   ECT_KEY_KP_6          = 326,
   ECT_KEY_KP_7          = 327,
   ECT_KEY_KP_8          = 328,
   ECT_KEY_KP_9          = 329,
   ECT_KEY_KP_DECIMAL    = 330,
   ECT_KEY_KP_DIVIDE     = 331,
   ECT_KEY_KP_MULTIPLY   = 332,
   ECT_KEY_KP_SUBTRACT   = 333,
   ECT_KEY_KP_ADD        = 334,
   ECT_KEY_KP_ENTER      = 335,
   ECT_KEY_KP_EQUAL      = 336
} EctKey;

#endif
