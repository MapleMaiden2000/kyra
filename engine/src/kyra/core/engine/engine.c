#include "kyra/core/engine/engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Engine state --------------------------------------------------------- //

typedef struct Engine_State {
    Bool        initialised;
} EngineState;

static EngineState *state = NULL;


// API functions -------------------------------------------------------- //

KYRA_ENGINE_API EngineResult engine_preconstruct(ConstStr config_filepath) {
    if (state) return ENGINE_PRECONSTRUCT_ERROR_STATE_ALREADY_INITIALISED;
    if (!config_filepath) return ENGINE_PRECONSTRUCT_ERROR_CONFIG_FILEPATH_NULL;

    // Allocate and reset engine state
    state = malloc(sizeof(EngineState));
    memset(state, 0, sizeof(EngineState));

    printf("Preconstructing engine...\n");
    
    printf("Engine preconstructed.\n");

    // Set engine state status as initialised
    state->initialised = true;

    return ENGINE_SUCCESS;
}

KYRA_ENGINE_API EngineResult engine_construct(void) {
    if (!state) return ENGINE_CONSTRUCT_ERROR_STATE_NOT_INITIALISED;

    printf("Constructing engine...\n");
    
    printf("Engine constructed.\n");

    return ENGINE_SUCCESS;
}

KYRA_ENGINE_API EngineResult engine_update(Flt32 delta_time) {
    if (!state) return ENGINE_UPDATE_ERROR_STATE_NOT_INITIALISED;
    
    printf("Updating engine...\n");
    
    return ENGINE_SUCCESS;
}

KYRA_ENGINE_API EngineResult engine_destruct(void) {
    if (!state) return ENGINE_DESTRUCT_ERROR_STATE_NOT_INITIALISED;

    printf("Destructing engine...\n");
    
    free(state);
    state = NULL;

    printf("Engine destructed.\n");

    return ENGINE_SUCCESS;
}


