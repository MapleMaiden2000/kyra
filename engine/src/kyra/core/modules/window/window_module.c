#include "kyra/core/modules/window/window_module.h"

#include <stdlib.h>
#include <string.h>

#include "kyra/core/platform/window/window.h"
#include "kyra/core/engine/engine.h"


// Internal state ------------------------------------------------ //

typedef struct Window_Module_State {
    Bool active;
    
} WindowModuleState;

static WindowModuleState state = { active: false };


// API functions ------------------------------------------------- //

KYRA_ENGINE_API WindowModuleResult window_module_startup(void) {
    if (state.active) return WINDOW_MODULE_ERROR_ALREADY_INITIALISED;

    // Platform window startup
    if (platform_window_startup(WINDOW_BACKEND_GLFW) != WINDOW_SUCCESS)
        return WINDOW_MODULE_ERROR_PLATFORM_WINDOW_STARTUP_FAILED;

    // Set state active
    state.active = true;

    return WINDOW_MODULE_SUCCESS;
}

KYRA_ENGINE_API WindowModuleResult window_module_shutdown(void) {
    if (!state.active) return WINDOW_MODULE_ERROR_NOT_INITIALISED;

    // Platform window shutdown
    if (platform_window_shutdown() != WINDOW_SUCCESS)
        return WINDOW_MODULE_ERROR_PLATFORM_WINDOW_SHUTDOWN_FAILED;

    // Set state inactive
    state.active = false;

    return WINDOW_MODULE_SUCCESS;
}

KYRA_ENGINE_API WindowModuleResult window_module_update(void) {
    if (!state.active) return WINDOW_MODULE_ERROR_NOT_INITIALISED;
    
    // Poll all windows for events
    if (platform_window_poll_events() != WINDOW_SUCCESS)
        return WINDOW_MODULE_ERROR_FAILED_TO_POLL_EVENTS;

    return WINDOW_MODULE_SUCCESS;
}

KYRA_ENGINE_API WindowModuleResult window_module_construct_window(const WindowConfigs configs, const WindowCallbacks *callbacks, const VoidPtr context, Window *out_window) {
    if (!state.active) return WINDOW_MODULE_ERROR_NOT_INITIALISED;
    
    // Construct window
    if (platform_window_construct_window(configs, callbacks, context, out_window) != WINDOW_SUCCESS)
        return WINDOW_MODULE_ERROR_FAILED_TO_CONSTRUCT_WINDOW;

    return WINDOW_MODULE_SUCCESS;
}

KYRA_ENGINE_API WindowModuleResult window_module_destruct_window(Window window) {
    if (!state.active) return WINDOW_MODULE_ERROR_NOT_INITIALISED;
    
    // Destruct window
    if (platform_window_destruct_window(window) != WINDOW_SUCCESS)
        return WINDOW_MODULE_ERROR_FAILED_TO_DESTRUCT_WINDOW;

    return WINDOW_MODULE_SUCCESS;
}

KYRA_ENGINE_API ConstStr window_module_result_to_string(const WindowModuleResult result) {
    switch (result) {
        case WINDOW_MODULE_SUCCESS:                                                         return "WINDOW_MODULE_SUCCESS";

        case WINDOW_MODULE_ERROR_NOT_INITIALISED:                                           return "WINDOW_MODULE_ERROR_NOT_INITIALISED";
        case WINDOW_MODULE_ERROR_ALREADY_INITIALISED:                                       return "WINDOW_MODULE_ERROR_ALREADY_INITIALISED";
        case WINDOW_MODULE_ERROR_PLATFORM_WINDOW_STARTUP_FAILED:                            return "WINDOW_MODULE_ERROR_PLATFORM_WINDOW_STARTUP_FAILED";
        case WINDOW_MODULE_ERROR_PLATFORM_WINDOW_SHUTDOWN_FAILED:                           return "WINDOW_MODULE_ERROR_PLATFORM_WINDOW_SHUTDOWN_FAILED";
        case WINDOW_MODULE_ERROR_FAILED_TO_POLL_EVENTS:                                     return "WINDOW_MODULE_ERROR_FAILED_TO_POLL_EVENTS";
        case WINDOW_MODULE_ERROR_FAILED_TO_CONSTRUCT_WINDOW:                                return "WINDOW_MODULE_ERROR_FAILED_TO_CONSTRUCT_WINDOW";
        case WINDOW_MODULE_ERROR_FAILED_TO_DESTRUCT_WINDOW:                                 return "WINDOW_MODULE_ERROR_FAILED_TO_DESTRUCT_WINDOW";
    
        default:                                                                            return "UNKNOWN_WINDOW_MDOULE_RESULT";
    }
}


