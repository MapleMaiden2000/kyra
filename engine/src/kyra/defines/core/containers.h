#pragma once

#include "kyra/defines/core/types.h"


// Return codes ---------------------------------------------------- //

typedef enum Container_Result {
    CONTAINER_SUCCESS                                                           = 0,


    // -- String -- //

    CONTAINER_STRING_HELPER_ERROR_REF_STRING_NULL                               = -100,
    CONTAINER_STRING_HELPER_ERROR_NEW_CAPACITY_ZERO                             = -101,
    CONTAINER_STRING_HELPER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_NEW_STRING      = -102,
    CONTAINER_STRING_HELPER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_OLD_STRING     = -103,

    CONTAINER_STRING_ERROR_VALUE_NULL                                           = 100,
    CONTAINER_STRING_ERROR_FORMAT_NULL                                          = 101,
    CONTAINER_STRING_ERROR_CAPACITY_ZERO                                        = 102,
    CONTAINER_STRING_ERROR_SUBSTRING_NULL                                       = 103,
    CONTAINER_STRING_ERROR_INDEX_OUT_OF_BOUNDS                                  = 104,
    CONTAINER_STRING_ERROR_REF_OUT_STRING_NULL                                  = 105,
    CONTAINER_STRING_ERROR_REF_STRING_NULL                                      = 106,
    CONTAINER_STRING_ERROR_REF_STRING_NOT_VALID                                 = 107,
    CONTAINER_STRING_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STRING                 = 108,
    CONTAINER_STRING_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STRING                = 109
    
} ContainerResult;


// Types ----------------------------------------------------------- //

typedef struct Container_String             *String;

