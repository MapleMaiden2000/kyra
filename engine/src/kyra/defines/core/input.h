#pragma once

#include "kyra/defines/core/types.h"


// Return codes ---------------------------------------------------- //

typedef enum Input_Module_Result {
    INPUT_MODULE_SUCCESS                                        = 0,

    INPUT_MODULE_ERROR_ALREADY_INITIALISED                      = -1,
    INPUT_MODULE_ERROR_NOT_INITIALISED                          = -2,
    INPUT_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE      = -3,
    INPUT_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE     = -4 

} InputModuleResult;


// Key codes (aligned with GLFW) ----------------------------------- //

typedef enum Input_Key_Code {
    KYRA_KEYCODE_SPACE          = 32,
    KYRA_KEYCODE_APOSTROPHE     = 39,
    KYRA_KEYCODE_COMMA          = 44,
    KYRA_KEYCODE_MINUS          = 45,
    KYRA_KEYCODE_PERIOD         = 46,
    KYRA_KEYCODE_SLASH          = 47,

    KYRA_KEYCODE_0              = 48,
    KYRA_KEYCODE_1              = 49,
    KYRA_KEYCODE_2              = 50,
    KYRA_KEYCODE_3              = 51,
    KYRA_KEYCODE_4              = 52,
    KYRA_KEYCODE_5              = 53,
    KYRA_KEYCODE_6              = 54,
    KYRA_KEYCODE_7              = 55,
    KYRA_KEYCODE_8              = 56,
    KYRA_KEYCODE_9              = 57,
    
    KYRA_KEYCODE_SEMICOLON      = 59,
    KYRA_KEYCODE_EQUAL          = 61,
    
    KYRA_KEYCODE_A              = 65,
    KYRA_KEYCODE_B              = 66,
    KYRA_KEYCODE_C              = 67,
    KYRA_KEYCODE_D              = 68,
    KYRA_KEYCODE_E              = 69,
    KYRA_KEYCODE_F              = 70,
    KYRA_KEYCODE_G              = 71,
    KYRA_KEYCODE_H              = 72,
    KYRA_KEYCODE_I              = 73,
    KYRA_KEYCODE_J              = 74,
    KYRA_KEYCODE_K              = 75,
    KYRA_KEYCODE_L              = 76,
    KYRA_KEYCODE_M              = 77,
    KYRA_KEYCODE_N              = 78,
    KYRA_KEYCODE_O              = 79,
    KYRA_KEYCODE_P              = 80,
    KYRA_KEYCODE_Q              = 81,
    KYRA_KEYCODE_R              = 82,
    KYRA_KEYCODE_S              = 83,
    KYRA_KEYCODE_T              = 84,
    KYRA_KEYCODE_U              = 85,
    KYRA_KEYCODE_V              = 86,
    KYRA_KEYCODE_W              = 87,
    KYRA_KEYCODE_X              = 88,
    KYRA_KEYCODE_Y              = 89,
    KYRA_KEYCODE_Z              = 90,
    
    KYRA_KEYCODE_ESCAPE         = 256,
    KYRA_KEYCODE_ENTER          = 257,
    KYRA_KEYCODE_TAB            = 258,
    KYRA_KEYCODE_BACKSPACE      = 259,
    KYRA_KEYCODE_INSERT         = 260,
    KYRA_KEYCODE_DELETE         = 261,
    
    KYRA_KEYCODE_RIGHT          = 262,
    KYRA_KEYCODE_LEFT           = 263,
    KYRA_KEYCODE_DOWN           = 264,
    KYRA_KEYCODE_UP             = 265,
    
    KYRA_KEYCODE_PAGE_UP        = 266,
    KYRA_KEYCODE_PAGE_DOWN      = 267,
    
    KYRA_KEYCODE_HOME           = 268,
    KYRA_KEYCODE_END            = 269,

    KYRA_KEYCODE_CAPS_LOCK      = 280,
    KYRA_KEYCODE_SCROLL_LOCK    = 281,
    KYRA_KEYCODE_NUM_LOCK       = 282,
    
    KYRA_KEYCODE_PRINT_SCREEN   = 283,
    KYRA_KEYCODE_PAUSE          = 284,
    
    KYRA_KEYCODE_F1             = 290,
    KYRA_KEYCODE_F2             = 291,
    KYRA_KEYCODE_F3             = 292,
    KYRA_KEYCODE_F4             = 293,
    KYRA_KEYCODE_F5             = 294,
    KYRA_KEYCODE_F6             = 295,
    KYRA_KEYCODE_F7             = 296,
    KYRA_KEYCODE_F8             = 297,
    KYRA_KEYCODE_F9             = 298,
    KYRA_KEYCODE_F10            = 299,
    KYRA_KEYCODE_F11            = 300,
    KYRA_KEYCODE_F12            = 301,

    KYRA_KEYCODE_LEFT_SHIFT     = 340,
    KYRA_KEYCODE_LEFT_CONTROL   = 341,
    KYRA_KEYCODE_LEFT_ALT       = 342,
    
    KYRA_KEYCODE_RIGHT_SHIFT    = 344,
    KYRA_KEYCODE_RIGHT_CONTROL  = 345,
    KYRA_KEYCODE_RIGHT_ALT      = 346,

    KYRA_KEYCODES_TOTAL         = 512

} InputKeyCode;


// Mouse codes ----------------------------------------------------- //

typedef enum Input_Mouse_Code {
    KYRA_MOUSECODE_LEFT         = 0,
    KYRA_MOUSECODE_RIGHT        = 1,
    KYRA_MOUSECODE_MIDDLE       = 2,

    KYRA_MOUSECODE_4            = 3,
    KYRA_MOUSECODE_5            = 4,
    KYRA_MOUSECODE_6            = 5,
    KYRA_MOUSECODE_7            = 6,
    KYRA_MOUSECODE_8            = 7,

    KYRA_MOUSECODES_TOTAL

} InputMouseCode;


// Event codes ----------------------------------------------------- //

typedef enum Input_Event_Code {
    INPUT_EVENT_KEY_PRESSED,
    INPUT_EVENT_KEY_RELEASED,
    
    INPUT_EVENT_MOUSE_PRESSED,
    INPUT_EVENT_MOUSE_RELEASED,
    INPUT_EVENT_MOUSE_MOVED,
    INPUT_EVENT_MOUSE_SCROLLED,
    
    INPUT_EVENTS_TOTAL

} InputEventCode;


