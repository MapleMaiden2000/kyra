#pragma once

#include "kyra/defines/core/hash.h"
#include "kyra/defines/core/containers.h"
#include "kyra/defines/core/delegates.h"


// Return codes ---------------------------------------------------- //

typedef enum Command_Module_Result {
    COMMAND_MODULE_POLL_STATUS_PENDING                                          = 1,

    COMMAND_MODULE_SUCCESS                                                      = 0,
    
    COMMAND_MODULE_ERROR_ALREADY_INITIALISED                                    = -1,
    COMMAND_MODULE_ERROR_NOT_INITIALISED                                        = -2,
    COMMAND_MODULE_ERROR_CONFIG_FILEPATH_NULL                                   = -3,
    COMMAND_MODULE_ERROR_SCHEMA_FILEPATH_NULL                                   = -4,
    COMMAND_MODULE_ERROR_ROOT_NAME_NULL                                         = -5,
    COMMAND_MODULE_ERROR_ACTION_NAME_NULL                                       = -6,
    COMMAND_MODULE_ERROR_REF_OUT_STRING_NULL                                    = -7,
    COMMAND_MODULE_ERROR_COMMAND_NULL                                           = -8,
    COMMAND_MODULE_ERROR_COMMAND_EMPTY                                          = -9,
    COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE                    = -10,
    COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE                   = -11,
    COMMAND_MODULE_ERROR_FAILED_TO_OPEN_CONFIG_FILE                             = -12,
    COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_CONFIG_RAW_BUFFER        = -13,
    COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_CONFIG_RAW_BUFFER       = -14,
    COMMAND_MODULE_ERROR_FAILED_TO_READ_CONFIG_FILE                             = -15,
    COMMAND_MODULE_ERROR_FAILED_TO_CLOSE_CONFIG_FILE                            = -16,
    COMMAND_MODULE_ERROR_FAILED_TO_PARSE_CONFIG_BUFFER_TO_JSON                  = -17,
    COMMAND_MODULE_ERROR_FAILED_TO_CONSTRUCT_REGISTRY                           = -18,
    COMMAND_MODULE_ERROR_FAILED_TO_DESTRUCT_REGISTRY                            = -19,
    COMMAND_MODULE_ERROR_FAILED_TO_OPEN_SCHEMA_FILE                             = -20,
    COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_SCHEMA_RAW_BUFFER        = -21,
    COMMAND_MODULE_ERROR_FAILED_TO_READ_SCHEMA_FILE                             = -22,
    COMMAND_MODULE_ERROR_FAILED_TO_CLOSE_SCHEMA_FILE                            = -23,
    COMMAND_MODULE_ERROR_FAILED_TO_PARSE_SCHEMA_BUFFER_TO_JSON                  = -24,
    COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_SCHEMA_RAW_BUFFER       = -25,
    COMMAND_MODULE_ERROR_FAILED_TO_CONSTRUCT_ACTION_MAP_FOR_ROOT                = -26,
    COMMAND_MODULE_ERROR_FAILED_TO_DESTRUCT_ACTION_MAP_FOR_ROOT                 = -27,
    COMMAND_MODULE_ERROR_FAILED_TO_CONSTRUCT_COMMAND_ACTION_DESCRIPTION_STRING  = -28,
    COMMAND_MODULE_ERROR_FAILED_TO_DESTRUCT_COMMAND_ACTION_DESCRIPTION_STRING   = -29,
    COMMAND_MODULE_ERROR_FAILED_TO_CONSTRUCT_COMMAND_ACTION_USAGE_STRING        = -30,
    COMMAND_MODULE_ERROR_FAILED_TO_DESTRUCT_COMMAND_ACTION_USAGE_STRING         = -31,
    COMMAND_MODULE_ERROR_FAILED_TO_REGISTER_COMMAND_ACTION                      = -32,
    COMMAND_MODULE_ERROR_FAILED_TO_REGISTER_ACTION_MAP_TO_REGISTRY              = -33,
    COMMAND_MODULE_ERROR_FAILED_TO_LOCATE_ROOT_IN_REGISTRY                      = -34,
    COMMAND_MODULE_ERROR_FAILED_TO_LOCATE_ACTION_MAP_FOR_ROOT                   = -35,
    COMMAND_MODULE_ERROR_FAILED_TO_REGISTER_DELEGATE_FOR_ACTION                 = -36,
    COMMAND_MODULE_ERROR_FAILED_TO_UPDATE_DELEGATE_FOR_ACTION                   = -37,
    COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_INPUT_BUFFER             = -38,
    COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_INPUT_BUFFER            = -39,
    COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_PROMPT_HEADER            = -40,
    COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_PROMPT_HEADER           = -41,
    COMMAND_MODULE_ERROR_FAILED_TO_CONSTRUCT_INPUT_BUFFER_STRING                = -42,
    COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_TOKEN_BUFFER             = -43,
    COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_TOKEN_BUFFER            = -44,
    COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_TOKEN_LIST               = -45,
    COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_TOKEN_LIST              = -46,
    COMMAND_MODULE_ERROR_FAILED_TO_LOCATE_COMMAND_ACTION                        = -47,
    COMMAND_MODULE_ERROR_NOT_ENOUGH_ARGUMENTS_FOR_COMMAND_DISPATCH              = -48,
    COMMAND_MODULE_ERROR_FAILED_TO_INVOKE_DELEGATE_FOR_ACTION                   = -49,

} CommandModuleResult;


// Context --------------------------------------------------------- //

typedef struct Command_Context {
    HashedID    root_hash;      // For command root (e.g. '/engine', '/editor', etc.)
    HashedID    action_hash;    // For action verb (e.g. 'exit', etc.)

    ConstStr   *args;           // Array of arguments
    ByteSize    num_args;       // Number of arguments

} CommandContext;


// Action ---------------------------------------------------------- //

typedef struct Command_Action {
    UInt32      num_args;

    String      desc;
    String      usage; 

} CommandAction;


