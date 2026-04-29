#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/types.h"
#include "kyra/defines/core/memory.h"


// Return codes --------------------------------------------------------- //

typedef enum Engine_Result {
    ENGINE_SUCCESS                                          = 0,
        
    ENGINE_HELPER_ERROR_CONFIG_FILEPATH_NULL                = -1,
    ENGINE_HELPER_ERROR_FAILED_TO_OPEN_CONFIG_FILE          = -2,
    ENGINE_HELPER_ERROR_FAILED_TO_GET_FILE_SIZE             = -3,
    ENGINE_HELPER_ERROR_FAILED_TO_CLOSE_CONFIG_FILE         = -4,
    ENGINE_HELPER_ERROR_FAILED_TO_PARSE_TO_JSON             = -5,
    ENGINE_HELPER_ERROR_SIZE_STRING_NULL                    = -6,

    // Pre-construct
    ENGINE_PRECONSTRUCT_ERROR_STATE_ALREADY_INITIALISED     = -100,
    ENGINE_PRECONSTRUCT_ERROR_CONFIG_FILEPATH_NULL          = -101,

    // Construct
    ENGINE_CONSTRUCT_ERROR_STATE_NOT_INITIALISED            = -200,
    
    // Update
    ENGINE_UPDATE_ERROR_STATE_NOT_INITIALISED               = -300,
    
    // Destruct
    ENGINE_DESTRUCT_ERROR_STATE_NOT_INITIALISED             = -300,
} EngineResult;


// Engine configurations ------------------------------------------------ //

typedef struct Engine_Config {
    Str     author;
    Str     version;
} EngineConfig;


// API functions -------------------------------------------------------- //

KYRA_ENGINE_API EngineResult    engine_preconstruct(ConstStr config_filepath);
KYRA_ENGINE_API EngineResult    engine_construct(void);
KYRA_ENGINE_API EngineResult    engine_update(Flt32 delta_time);
KYRA_ENGINE_API EngineResult    engine_destruct(void);

