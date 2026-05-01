#pragma once

#include "kyra/defines/core/types.h"


// Return codes --------------------------------------------------------- //

typedef enum Console_Result {
    CONSOLE_SUCCESS                         = 0,

    CONSOLE_ERROR_ALREADY_INITIALISED       = -1,
    CONSOLE_ERROR_NOT_INITIALISED           = -2,
    CONSOLE_ERROR_FAILED_TO_GET_HANDLES     = -3,
    CONSOLE_ERROR_FAILED_TO_GET_MODES       = -4,
    CONSOLE_ERROR_FAILED_TO_SET_MODES       = -5,

} ConsoleResult;


// Colours -------------------------------------------------------------- //

typedef enum Console_Colour {
    // Standard
    CONSOLE_COLOUR_BLACK                    = 0,
    CONSOLE_COLOUR_RED                      = 1,
    CONSOLE_COLOUR_GREEN                    = 2,
    CONSOLE_COLOUR_YELLOW                   = 3,
    CONSOLE_COLOUR_BLUE                     = 4,
    CONSOLE_COLOUR_MAGENTA                  = 5,
    CONSOLE_COLOUR_CYAN                     = 6,
    CONSOLE_COLOUR_WHITE                    = 7,

    // High-Intensity
    CONSOLE_COLOUR_BRIGHT_BLACK             = 8,
    CONSOLE_COLOUR_BRIGHT_RED               = 9,
    CONSOLE_COLOUR_BRIGHT_GREEN             = 10,
    CONSOLE_COLOUR_BRIGHT_YELLOW            = 11,
    CONSOLE_COLOUR_BRIGHT_BLUE              = 12,
    CONSOLE_COLOUR_BRIGHT_MAGENTA           = 13,
    CONSOLE_COLOUR_BRIGHT_CYAN              = 14,
    CONSOLE_COLOUR_BRIGHT_WHITE             = 15
} ConsoleColour;

