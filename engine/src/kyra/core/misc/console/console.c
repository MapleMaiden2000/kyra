#include "kyra/core/misc/console/console.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if KYRA_PLATFORM_WINDOWS
    #include <windows.h>
    #include <conio.h>

    // Enable virtual terminal processing if not already enabled
    #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
        #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
    #endif

    // Enable console input processing if not already enabled
    #ifndef ENABLE_INPUT_PROCESSING
        #define ENABLE_INPUT_PROCESSING 0x0002
    #endif

    // --- Console handles --- //

    static HANDLE   console_input_handle = INVALID_HANDLE_VALUE;
    static HANDLE   console_output_handle = INVALID_HANDLE_VALUE;
    static DWORD    console_input_mode = 0;
    static DWORD    console_output_mode = 0;

#endif

static Bool initialised = false;


// API functions ------------------------------------------------ //

KYRA_ENGINE_API ConsoleResult console_startup(void) {
    if (initialised) return CONSOLE_ERROR_ALREADY_INITIALISED;
    
    #if KYRA_PLATFORM_WINDOWS
        // Initialise console handles
        console_input_handle = GetStdHandle(STD_INPUT_HANDLE);
        console_output_handle = GetStdHandle(STD_OUTPUT_HANDLE);

        if (console_input_handle == INVALID_HANDLE_VALUE || console_output_handle == INVALID_HANDLE_VALUE) 
		    return CONSOLE_ERROR_FAILED_TO_GET_HANDLES;

        // Initialise console modes
        if (!GetConsoleMode(console_input_handle, &console_input_mode) || !GetConsoleMode(console_output_handle, &console_output_mode))
		    return CONSOLE_ERROR_FAILED_TO_GET_MODES;

        // Integrate input/output processing flags
        console_input_mode |= ENABLE_INPUT_PROCESSING;
        console_output_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

        // Save modes to console handles
        if (!SetConsoleMode(console_input_handle, console_input_mode) || !SetConsoleMode(console_output_handle, console_output_mode))
		    return CONSOLE_ERROR_FAILED_TO_SET_MODES;
    
    #elif KYRA_PLATFORM_LINUX
        setvbuf(stdout, NULL, _IONBF, 0);
    
    #endif

    // Set status as initialised
    initialised = true; 

    return CONSOLE_SUCCESS;
}

KYRA_ENGINE_API ConsoleResult console_shutdown(void) {
    if (!initialised) return CONSOLE_ERROR_NOT_INITIALISED;

    #if KYRA_PLATFORM_WINDOWS
        // Restore original console modes to handles
        {
            if (console_input_handle != INVALID_HANDLE_VALUE) 
                SetConsoleMode(console_input_handle, console_input_mode);
            
            if (console_output_handle != INVALID_HANDLE_VALUE) 
                SetConsoleMode(console_output_handle, console_output_mode);
        }
    #endif

    return CONSOLE_SUCCESS;
}

KYRA_ENGINE_API ConsoleResult console_reset(void) {
    if (!initialised) return CONSOLE_ERROR_NOT_INITIALISED;

    fprintf(stdout, "\x1b[0m");
    fflush(stdout);

    return CONSOLE_SUCCESS;
}

KYRA_ENGINE_API ConsoleResult console_set_title(ConstStr title) {
    if (!initialised) return CONSOLE_ERROR_NOT_INITIALISED;

    #if KYRA_PLATFORM_WINDOWS
        SetConsoleTitleA(title);

    #elif KYRA_PLATFORM_LINUX
        fprintf(stdout, "\x1b]2;%s\x07", title);
    
    #endif

    return CONSOLE_SUCCESS;
}


// -- Colour -- //

KYRA_ENGINE_API ConsoleResult console_set_foreground(ConsoleColour colour) {
    if (!initialised) return CONSOLE_ERROR_NOT_INITIALISED;

    fprintf(stdout, "\x1b[38;5;%dm", colour);

    return CONSOLE_SUCCESS;
}

KYRA_ENGINE_API ConsoleResult console_set_background(ConsoleColour colour) {
    if (!initialised) return CONSOLE_ERROR_NOT_INITIALISED;

    fprintf(stdout, "\x1b[48;5;%dm", colour);

    return CONSOLE_SUCCESS;
}

KYRA_ENGINE_API ConsoleResult console_get_foreground_rgb(UInt8 r, UInt8 g, UInt8 b) {
    if (!initialised) return CONSOLE_ERROR_NOT_INITIALISED;

    fprintf(stdout, "\x1b[38;2;%d;%d;%dm", r, g, b);

    return CONSOLE_SUCCESS;
}

KYRA_ENGINE_API ConsoleResult console_get_background_rgb(UInt8 r, UInt8 g, UInt8 b) {
    if (!initialised) return CONSOLE_ERROR_NOT_INITIALISED;

    fprintf(stdout, "\x1b[48;2;%d;%d;%dm", r, g, b);

    return CONSOLE_SUCCESS;
}


// -- Output -- //

KYRA_ENGINE_API ConsoleResult console_write(ConstStr format, ...) {
    if (!initialised) return CONSOLE_ERROR_NOT_INITIALISED;

    VaList args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);

    return CONSOLE_SUCCESS;
}

KYRA_ENGINE_API ConsoleResult console_write_line(ConstStr format, ...) {
    if (!initialised) return CONSOLE_ERROR_NOT_INITIALISED;

    VaList args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
    fprintf(stdout, "\n");

    return CONSOLE_SUCCESS;
}


// -- Text formatting -- //

KYRA_ENGINE_API ConsoleResult console_format_bold(void) {
    if (!initialised) return CONSOLE_ERROR_NOT_INITIALISED;

    fprintf(stdout, "\x1b[1m");

    return CONSOLE_SUCCESS;
}

KYRA_ENGINE_API ConsoleResult console_format_faint(void) {
    if (!initialised) return CONSOLE_ERROR_NOT_INITIALISED;

    fprintf(stdout, "\x1b[2m");

    return CONSOLE_SUCCESS;
}

KYRA_ENGINE_API ConsoleResult console_format_italic(void) {
    if (!initialised) return CONSOLE_ERROR_NOT_INITIALISED;

    fprintf(stdout, "\x1b[3m");

    return CONSOLE_SUCCESS;
}

KYRA_ENGINE_API ConsoleResult console_format_underline(void) {
    if (!initialised) return CONSOLE_ERROR_NOT_INITIALISED;

    fprintf(stdout, "\x1b[4m");

    return CONSOLE_SUCCESS;
}

KYRA_ENGINE_API ConsoleResult console_format_reverse(void) {
    if (!initialised) return CONSOLE_ERROR_NOT_INITIALISED;

    fprintf(stdout, "\x1b[7m");

    return CONSOLE_SUCCESS;
}

KYRA_ENGINE_API ConsoleResult console_format_strike(void) {
    if (!initialised) return CONSOLE_ERROR_NOT_INITIALISED;

    fprintf(stdout, "\x1b[9m");

    return CONSOLE_SUCCESS;
}


// -- Result to string -- //

KYRA_ENGINE_API ConstStr console_result_to_string(const ConsoleResult result) {
    switch (result) {
        case CONSOLE_SUCCESS:                           return "CONSOLE_SUCCESS";

        case CONSOLE_ERROR_ALREADY_INITIALISED:         return "CONSOLE_ERROR_ALREADY_INITIALISED";
        case CONSOLE_ERROR_NOT_INITIALISED:             return "CONSOLE_ERROR_NOT_INITIALISED";
        case CONSOLE_ERROR_FAILED_TO_GET_HANDLES:       return "CONSOLE_ERROR_FAILED_TO_GET_HANDLES";
        case CONSOLE_ERROR_FAILED_TO_GET_MODES:         return "CONSOLE_ERROR_FAILED_TO_GET_MODES";
        case CONSOLE_ERROR_FAILED_TO_SET_MODES:         return "CONSOLE_ERROR_FAILED_TO_SET_MODES";
    
        default:                                        return "UNKNOWN_CONSOLE_RESULT";
    }
}

