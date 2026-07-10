#pragma once

#include "kyra/defines/core/types.h"
#include "kyra/defines/core/hash.h"


// Return codes -------------------------------------------- //

typedef enum Timer_Result {
    TIMER_SUCCESS                                                   = 0,

    TIMER_ERROR_NOT_INITIALISED                                     = -1,
    TIMER_ERROR_ALREADY_INITIALISED                                 = -2,
    TIMER_ERROR_CONFIG_FILEPATH_NULL                                = -3,
    TIMER_ERROR_TIMER_ID_NULL                                       = -4,
    TIMER_ERROR_REF_OUT_HANDLE_NULL                                 = -5,
    TIMER_ERROR_TIMER_HANDLE_NULL                                   = -6,
    TIMER_ERROR_TIMER_HANDLE_HASH_ID_INVALID                        = -7,
    TIMER_ERROR_TIMER_REGISTRY_FULL                                 = -8,
    TIMER_ERROR_FAILED_TO_OPEN_CONFIG_FILE                          = -9,
    TIMER_ERROR_FAILED_TO_CLOSE_CONFIG_FILE                         = -10,
    TIMER_ERROR_FAILED_TO_GET_CONFIG_FILE_SIZE                      = -11,
    TIMER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_CONFIG_RAW_BUFFER     = -12,
    TIMER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_CONFIG_RAW_BUFFER    = -13,
    TIMER_ERROR_FAILED_TO_READ_CONFIG_FILE                          = -14,
    TIMER_ERROR_FAILED_TO_PARSE_CONFIG_BUFFER_TO_JSON               = -15,
    TIMER_ERROR_FAILED_TO_LOCATE_RUNTIME_SECTION_IN_CONFIG_JSON     = -16,
    TIMER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE                 = -17,
    TIMER_ERROR_FAILED_TO_DEALLOCCATE_MEMORY_OF_STATE               = -18,
    TIMER_ERROR_FAILED_TO_START_TIMER_CLOCK                         = -19,
    TIMER_ERROR_FAILED_TO_LOCATE_TIMER_SLOT_FOR_HANDLE_HASHED_ID    = -20,

} TimerResult;


// Handle -------------------------------------------------- //

typedef struct Timer_Handle {
    HashedID    hashed_id;

} TimerHandle;
