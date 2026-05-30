#pragma once

#include <kyra/defines/core/containers.h>


// Return codes --------------------------------------------------- //

typedef enum Editor_Result {
    EDITOR_SUCCESS                                                              = 0,
    
    EDITOR_HELPER_ERROR_CONFIG_FILEPATH_NULL                                    = -1,
    EDITOR_HELPER_ERROR_FAILED_TO_OPEN_CONFIG_FILE                              = -2,
    EDITOR_HELPER_ERROR_FAILED_TO_GET_FILE_SIZE                                 = -3,
    EDITOR_HELPER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_CONFIG_RAW_BUFFER         = -4,
    EDITOR_HELPER_ERROR_FAILED_TO_CLOSE_CONFIG_FILE                             = -5,
    EDITOR_HELPER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_CONFIG_RAW_BUFFER        = -6,
    EDITOR_HELPER_ERROR_FAILED_TO_READ_CONFIG_FILE                              = -7,
    EDITOR_HELPER_ERROR_FAILED_TO_PARSE_TO_JSON                                 = -8,
    EDITOR_HELPER_ERROR_FAILED_TO_CONSTRUCT_WINDOW_TITLE_STRING                 = -9,

    EDITOR_ERROR_ALREADY_INITIALISED                                            = 1,
    EDITOR_ERROR_NOT_INITIALISED                                                = 2,
    EDITOR_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE                            = 3,
    EDITOR_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE                           = 4,
    EDITOR_ERROR_WINDOW_MODULE_STARTUP_FAILED                                   = 5,
    EDITOR_ERROR_FAILED_TO_CONFIGURE_EDITOR                                     = 6,
    EDITOR_ERROR_FAILED_TO_CONSTRUCT_WINDOW_FOR_EDITOR                          = 7,

    
} EditorResult;


// Configurations ------------------------------------------------- //

typedef struct Editor_Config {
    String      window_title;
    
    Int32       window_width;
    Int32       window_height;
    
    Bool        window_fullscreen;
    Bool        window_vsync;

} EditorConfig;

