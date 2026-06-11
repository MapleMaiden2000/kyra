#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/console.h"


// API functions ------------------------------------------------ //

KYRA_ENGINE_API ConsoleResult   console_startup(void);
KYRA_ENGINE_API ConsoleResult   console_shutdown(void);

KYRA_ENGINE_API ConsoleResult   console_reset(void);
KYRA_ENGINE_API ConsoleResult   console_set_title(ConstStr title);

KYRA_ENGINE_API ConsoleResult   console_init_prompt(ConstStr header);


// -- Colour -- //

KYRA_ENGINE_API ConsoleResult   console_set_foreground(const ConsoleColour colour);
KYRA_ENGINE_API ConsoleResult   console_set_background(const ConsoleColour colour);
KYRA_ENGINE_API ConsoleResult   console_set_foreground_rgb(const UInt8 r, const UInt8 g, const UInt8 b);
KYRA_ENGINE_API ConsoleResult   console_set_background_rgb(const UInt8 r, const UInt8 g, const UInt8 b);


// -- Cursor -- //

KYRA_ENGINE_API ConsoleResult   console_cursor_move_absolute(const Int32 column);
KYRA_ENGINE_API ConsoleResult   console_cursor_clear(const ConsoleCursorMode mode);


// -- Output -- //

KYRA_ENGINE_API ConsoleResult   console_write(ConstStr format, ...);
KYRA_ENGINE_API ConsoleResult   console_write_line(ConstStr format, ...);


// -- Text formatting -- //

KYRA_ENGINE_API ConsoleResult   console_format_bold(void);
KYRA_ENGINE_API ConsoleResult   console_format_faint(void);
KYRA_ENGINE_API ConsoleResult   console_format_italic(void);
KYRA_ENGINE_API ConsoleResult   console_format_underline(void);
KYRA_ENGINE_API ConsoleResult   console_format_reverse(void);
KYRA_ENGINE_API ConsoleResult   console_format_strike(void);


// -- Result to string -- //

KYRA_ENGINE_API ConstStr        console_result_to_string(const ConsoleResult result);


// Universal colour printing ------------------------------------ //

// Console print with foreground colour
#define KYRA_CONSOLE_PRINT_FG(colour, format, ...)              \
    do {                                                        \
        console_set_foreground(colour);                         \
        console_write_line(format, ##__VA_ARGS__);              \
        console_reset();                                        \
    } while(0)

// Console print with background colour 
#define KYRA_CONSOLE_PRINT_BG(colour, format, ...)              \
    do {                                                        \
        console_set_background(colour);                         \
        console_write_line(format, ##__VA_ARGS__);              \
        console_reset();                                        \
    } while(0)

// Console print with both foreground and background colours
#define KYRA_CONSOLE_PRINT_COLOURED(fg_colour, bg_colour, format, ...)      \
    do {                                                                    \
        console_set_foreground(fg_colour);                                  \
        console_set_background(bg_colour);                                  \
        console_write_line(format, ##__VA_ARGS__);                          \
        console_reset();                                                    \
    } while(0)

#define KYRA_CONSOLE_PRINT_FATAL(format, ...)   KYRA_CONSOLE_PRINT_COLOURED(CONSOLE_COLOUR_BRIGHT_WHITE, CONSOLE_COLOUR_BRIGHT_RED, "[Kyra-Fatal] " format, ##__VA_ARGS__)
#define KYRA_CONSOLE_PRINT_ERROR(format, ...)   KYRA_CONSOLE_PRINT_FG(CONSOLE_COLOUR_BRIGHT_RED, "[Kyra-Error] " format, ##__VA_ARGS__)
#define KYRA_CONSOLE_PRINT_WARN(format, ...)    KYRA_CONSOLE_PRINT_FG(CONSOLE_COLOUR_BRIGHT_YELLOW, "[Kyra-Warn] " format, ##__VA_ARGS__)
#define KYRA_CONSOLE_PRINT_INFO(format, ...)    KYRA_CONSOLE_PRINT_FG(CONSOLE_COLOUR_BRIGHT_WHITE, "[Kyra-Info] " format, ##__VA_ARGS__)
#define KYRA_CONSOLE_PRINT_DEBUG(format, ...)   KYRA_CONSOLE_PRINT_FG(CONSOLE_COLOUR_BRIGHT_GREEN, "[Kyra-Debug] " format, ##__VA_ARGS__)
#define KYRA_CONSOLE_PRINT_TRACE(format, ...)   KYRA_CONSOLE_PRINT_FG(CONSOLE_COLOUR_BRIGHT_BLUE, "[Kyra-Trace] " format, ##__VA_ARGS__)
