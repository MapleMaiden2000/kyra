#pragma once

#include "kyra/defines/core/types.h"


// Return codes ---------------------------------------------------- //

typedef enum Window_Result {
    WINDOW_SUCCESS                                                              = 0,


    // -- Window interface -- //

    WINDOW_ERROR_ALREADY_INITIALISED                                            = -1,
    WINDOW_ERROR_NOT_INITIALISED                                                = -2,
    WINDOW_ERROR_INVALID_BACKEND_OPTION                                         = -3,


    // -- Window backend (GLFW) -- //

    WINDOW_BACKEND_GLFW_ERROR_REF_OUT_WINDOW_NULL                               = -100,
    WINDOW_BACKEND_GLFW_ERROR_WINDOW_NULL                                       = -101,
    WINDOW_BACKEND_GLFW_ERROR_FAILED_TO_INITIALISE                              = -102,
    WINDOW_BACKEND_GLFW_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_WINDOW_STATE        = -103,
    WINDOW_BACKEND_GLFW_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_WINDOW_STATE       = -104,
    WINDOW_BACKEND_GLFW_ERROR_FAILED_TO_CONSTRUCT_WINDOW_HANDLE                 = -105,

} WindowResult;

typedef enum Window_Module_Result {
    WINDOW_MODULE_SUCCESS                                                       = 0,

    WINDOW_MODULE_ERROR_NOT_INITIALISED                                         = -1,
    WINDOW_MODULE_ERROR_ALREADY_INITIALISED                                     = -2,
    WINDOW_MODULE_ERROR_PLATFORM_WINDOW_STARTUP_FAILED                          = -3,
    WINDOW_MODULE_ERROR_PLATFORM_WINDOW_SHUTDOWN_FAILED                         = -4,
    WINDOW_MODULE_ERROR_FAILED_TO_POLL_EVENTS                                   = -5,
    WINDOW_MODULE_ERROR_FAILED_TO_CONSTRUCT_WINDOW                              = -6,
    WINDOW_MODULE_ERROR_FAILED_TO_DESTRUCT_WINDOW                               = -7,

} WindowModuleResult;


// Window modes ---------------------------------------------------- //

typedef enum Window_Mode {
    WINDOW_MODE_WINDOWED,
    WINDOW_MODE_FULLSCREEN,
    WINDOW_MODE_BORDERLESS

} WindowMode;


// Callbacks ------------------------------------------------------- //

typedef struct Window_Callbacks {
    void    (*on_close)(VoidPtr context);
    void    (*on_resize)(VoidPtr context, Int32 width, Int32 height);
    void    (*on_refresh)(VoidPtr context);    
    void    (*on_minimise)(VoidPtr context, Bool minimise);
    void    (*on_focus)(VoidPtr context, Bool focus);

    void    (*on_key)(VoidPtr context, Int32 key, Int32 scancode, Int32 action, Int32 mods);
    
    void    (*on_mouse_button)(VoidPtr context, Int32 button, Int32 action, Int32 mods);
    void    (*on_mouse_move)(VoidPtr context, Flt64 x, Flt64 y);

} WindowCallbacks;


// Configurations -------------------------------------------------- //

typedef struct Window_Configurations {
    ConstStr            title;
    
    Int32               width;
    Int32               height;
    
    WindowMode          mode;
    Bool                vsync;

} WindowConfigs;


// Window handle --------------------------------------------------- //

typedef struct Window_Handle    *Window;



