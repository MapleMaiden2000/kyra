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
    CONTAINER_STRING_ERROR_STRING_NULL                                          = 103,
    CONTAINER_STRING_ERROR_SUBSTRING_NULL                                       = 104,
    CONTAINER_STRING_ERROR_SUBSTRING_LONGER_THAN_STRING                         = 105,
    CONTAINER_STRING_ERROR_PREFIX_NULL                                          = 106,
    CONTAINER_STRING_ERROR_PREFIX_LONGER_THAN_STRING                            = 107,
    CONTAINER_STRING_ERROR_SUFFIX_NULL                                          = 108,
    CONTAINER_STRING_ERROR_SUFFIX_LONGER_THAN_STRING                            = 109,
    CONTAINER_STRING_ERROR_INDEX_OUT_OF_BOUNDS                                  = 110,
    CONTAINER_STRING_ERROR_LEFT_AND_RIGHT_SIZES_MISMATCHED                      = 111,
    CONTAINER_STRING_ERROR_REF_OUT_STRING_NULL                                  = 112,
    CONTAINER_STRING_ERROR_REF_STRING_NULL                                      = 113,
    CONTAINER_STRING_ERROR_REF_STRING_NOT_VALID                                 = 114,
    CONTAINER_STRING_ERROR_REF_OUT_NEW_SUBSTRING_NULL                           = 115,
    CONTAINER_STRING_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STRING                 = 116,
    CONTAINER_STRING_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STRING                = 117,
    CONTAINER_STRING_ERROR_FAILED_TO_FIND_ANY_MATCH                             = 118,
    

    // -- Map -- //

    CONTAINER_MAP_HELPER_ERROR_REF_MAP_NULL                                     = -200,
    CONTAINER_MAP_HELPER_ERROR_OLD_SLOT_NULL                                    = -201,
    CONTAINER_MAP_HELPER_ERROR_FAILED_TO_COPY_OLD_SLOT_KEY                      = -202,
    CONTAINER_MAP_HELPER_ERROR_NEW_CAPACITY_SHORTER_THAN_MAP_SIZE               = -203,
    CONTAINER_MAP_HELPER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_NEW_MAP            = -204,
    CONTAINER_MAP_HELPER_ERROR_FAILED_TO_DEALLOCATE_NEW_MAP                     = -205,
    CONTAINER_MAP_HELPER_ERROR_REHASH_INSERT_FAILED                             = -206,
    CONTAINER_MAP_HELPER_ERROR_FAILED_TO_DEALLOCATE_OLD_MAP                     = -207,

    CONTAINER_MAP_ERROR_DATA_SIZE_ZERO                                          = 200,
    CONTAINER_MAP_ERROR_REF_OUT_MAP_NULL                                        = 201,
    CONTAINER_MAP_ERROR_REF_MAP_NULL                                            = 202,
    CONTAINER_MAP_ERROR_MAP_NULL                                                = 203,
    CONTAINER_MAP_ERROR_KEY_NULL                                                = 204,
    CONTAINER_MAP_ERROR_VALUE_NULL                                              = 205,
    CONTAINER_MAP_ERROR_NEW_VALUE_NULL                                          = 206,
    CONTAINER_MAP_ERROR_INDEX_OUT_OF_BOUNDS                                     = 207,
    CONTAINER_MAP_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_NEW_MAP                   = 208,
    CONTAINER_MAP_ERROR_FAILED_TO_DEALLOCATE_MAP                                = 209,
    CONTAINER_MAP_ERROR_FAILED_TO_CONSTRUCT_SLOT_KEY                            = 210,
    CONTAINER_MAP_ERROR_FAILED_TO_LOCATE_SLOT_FOR_KEY                           = 211,
    CONTAINER_MAP_ERROR_FAILED_TO_LOCATE_SLOT_FOR_INDEX                         = 212,
    CONTAINER_MAP_ERROR_REACHED_PROBING_LIMIT                                   = 213,


    // -- Array -- //

    CONTAINER_ARRAY_HELPER_ERROR_ARRAY_NULL                                     = -300,
    CONTAINER_ARRAY_HELPER_ERROR_NEW_CAPACITY_ZERO                              = -301,
    CONTAINER_ARRAY_HELPER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_NEW_ARRAY        = -302,

    CONTAINER_ARRAY_ERROR_DATA_SIZE_ZERO                                        = 301,
    CONTAINER_ARRAY_ERROR_DATA_NULL                                             = 302,
    CONTAINER_ARRAY_ERROR_INDEX_OUT_OF_BOUNDS                                   = 303,
    CONTAINER_ARRAY_ERROR_INVALID_COMPARE_FUNCPTR                               = 304,
    CONTAINER_ARRAY_ERROR_REF_OUT_ARRAY_NULL                                    = 305,
    CONTAINER_ARRAY_ERROR_REF_ARRAY_NULL                                        = 306,
    CONTAINER_ARRAY_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_ARRAY                   = 307,
    CONTAINER_ARRAY_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_ARRAY                  = 308,

} ContainerResult;


// Types ----------------------------------------------------------- //

typedef struct Container_String             *String;
typedef struct Container_Map                *Map;
typedef struct Container_Array              *Array;
