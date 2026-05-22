#pragma once

#include "kyra/defines/core/memory.h"
#include "kyra/defines/core/filesystem.h"


// Return codes ---------------------------------------------------- //

typedef enum Logger_Result {
    LOGGER_SUCCESS                                                          = 0,

    LOGGER_ERROR_ALREADY_INITIALISED                                        = -1,
    LOGGER_ERROR_NOT_INITIALISED                                            = -2,
    LOGGER_ERROR_CONFIG_FILEPATH_NULL                                       = -3,
    LOGGER_ERROR_FAILED_TO_OPEN_CONFIG_FILE                                 = -4,
    LOGGER_ERROR_FAILED_TO_GET_CONFIG_FILE_SIZE                             = -5,
    LOGGER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_CONFIG_RAW_BUFFER            = -6,
    LOGGER_ERROR_FAILED_TO_READ_CONFIG_FILE                                 = -7,
    LOGGER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_CONFIG_RAW_BUFFER           = -8,
    LOGGER_ERROR_FAILED_TO_PARSE_CONFIG_BUFFER_TO_JSON                      = -9,
    LOGGER_ERROR_FAILED_TO_LOCATE_LOG_SYSTEM_SECTION_IN_CONFIG_JSON         = -10,
    LOGGER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE                        = -11,
    LOGGER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE                       = -12,
    LOGGER_ERROR_FAILED_TO_CONSTRUCT_LOGGER_MAP                             = -13,
    LOGGER_ERROR_FAILED_TO_DESTRUCT_LOGGER_MAP                              = -14,
    LOGGER_ERROR_FAILED_TO_CONSTRUCT_OUT_DIRECTORY_STRING                   = -15,
    LOGGER_ERROR_FAILED_TO_DESTRUCT_OUT_DIRECTORY_STRING                    = -16,
    LOGGER_ERROR_FAILED_TO_CONSTRUCT_ID_STRING_FOR_LOGGER                   = -17,
    LOGGER_ERROR_FAILED_TO_DESTRUCT_ID_STRING_FOR_LOGGER                    = -18,
    LOGGER_ERROR_FAILED_TO_CLOSE_OUTPUT_LOG_FILE_FOR_LOGGER                 = -19,
    LOGGER_ERROR_FAILED_TO_REGISTER_LOGGER                                  = -20,
    LOGGER_ERROR_FAILED_TO_UNREGISTER_LOGGER                                = -21,
    LOGGER_ERROR_FAILED_TO_UPDATE_LOGGER                                    = -22,
    LOGGER_ERROR_FAILED_TO_GET_LOGGER                                       = -23,

} LoggerResult;


// Verbosity levels ------------------------------------------------ //

typedef enum Logger_Verbosity {
    LOGGER_VERBOSITY_NONE = 0,
    
    LOGGER_VERBOSITY_FATAL,
    LOGGER_VERBOSITY_ERROR,
    LOGGER_VERBOSITY_WARNING,
    LOGGER_VERBOSITY_INFO,
    
    LOGGER_VERBOSITY_DEBUG,
    LOGGER_VERBOSITY_TRACE

} LoggerVerbosity;


// Log flags ------------------------------------------------------------ //

typedef enum Logger_Flags {
    LOGGER_FLAG_NONE = 0,

    LOGGER_FLAG_TIMESTAMP = KYRA_BITFLAG_FIELD(0),
    LOGGER_FLAG_VERBOSITY = KYRA_BITFLAG_FIELD(1),
    LOGGER_FLAG_FILE_LINE = KYRA_BITFLAG_FIELD(2),
    LOGGER_FLAG_FUNCTION = KYRA_BITFLAG_FIELD(3),
    
    LOGGER_FLAG_ALL = LOGGER_FLAG_TIMESTAMP | LOGGER_FLAG_VERBOSITY | LOGGER_FLAG_FILE_LINE | LOGGER_FLAG_FUNCTION

} LoggerFlags;

