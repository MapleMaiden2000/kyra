#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/types.h"


// Return codes --------------------------------------------------------- //

typedef enum Engine_Result {
    ENGINE_SUCCESS                                          = 0,

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


// API functions -------------------------------------------------------- //

KYRA_ENGINE_API EngineResult    engine_preconstruct(ConstStr config_filepath);
KYRA_ENGINE_API EngineResult    engine_construct(void);
KYRA_ENGINE_API EngineResult    engine_update(Flt32 delta_time);
KYRA_ENGINE_API EngineResult    engine_destruct(void);

