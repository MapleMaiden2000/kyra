#include "kyra/core/modules/input/input_module.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "kyra/core/memory/zone/memory_zone.h"
#include "kyra/core/logger/logger.h"


// Internal structure ------------------------------------------ //

typedef struct Input_Keyboard_State {
    Bool                    keys[KYRA_KEYCODES_TOTAL];

} InputKeyboardState;

typedef struct Input_Mouse_State {
    Bool                    buttons[KYRA_MOUSECODES_TOTAL];
    Int32                   x, y;

} InputMouseState;

typedef struct Input_Module_State {
    InputKeyboardState      current_keyboard;
    InputKeyboardState      previous_keyboard;
    
    InputMouseState         current_mouse;
    InputMouseState         previous_mouse;

    // For allocations/deallocations
    ByteSize                memory_size;

} InputModuleState;

static InputModuleState *state = NULL;


// API functions ----------------------------------------------- //

KYRA_ENGINE_API InputModuleResult input_module_startup(void) {
    if (state) return INPUT_MODULE_ERROR_ALREADY_INITIALISED;

    // Allocate for state
    ByteSize mem_size = 0;
    if (memory_zone_allocate("input", sizeof(InputModuleState), (VoidPtr *)&state, &mem_size) != MEMORY_ZONE_SUCCESS)
        return INPUT_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE;

    // Zero out state
    memset(state, 0, sizeof(InputModuleState));

    // Assign memory size
    state->memory_size = mem_size;

    return INPUT_MODULE_SUCCESS;
}

KYRA_ENGINE_API InputModuleResult input_module_shutdown(void) {
    if (!state) return INPUT_MODULE_ERROR_NOT_INITIALISED;

    // Deallocate state
    if (memory_zone_deallocate("input", (VoidPtr)state, state->memory_size) != MEMORY_ZONE_SUCCESS)
        return INPUT_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE;

    // Set to NULL
    state = NULL;

    return INPUT_MODULE_SUCCESS;
}

KYRA_ENGINE_API InputModuleResult input_module_update(void) {
    if (!state) return INPUT_MODULE_ERROR_NOT_INITIALISED;

    // Snapshot!
    // Copy current states to previous states
    memcpy(&state->previous_keyboard, &state->current_keyboard, sizeof(InputKeyboardState));
    memcpy(&state->previous_mouse, &state->current_mouse, sizeof(InputMouseState));

    return INPUT_MODULE_SUCCESS;
}


// -- Keyboard -- //

KYRA_ENGINE_API Bool input_module_is_key_pressed(const InputKeyCode code) {
    if (!state || code >= KYRA_KEYCODES_TOTAL) return false;

    // Key is pressed
    return state->current_keyboard.keys[code];
}

KYRA_ENGINE_API Bool input_module_is_key_just_pressed(const InputKeyCode code) {
    if (!state || code >= KYRA_KEYCODES_TOTAL) return false;

    // Key is pressed and was not pressed in the previous frame
    return state->current_keyboard.keys[code] && !state->previous_keyboard.keys[code];
}

KYRA_ENGINE_API Bool input_module_is_key_just_released(const InputKeyCode code) {
    if (!state || code >= KYRA_KEYCODES_TOTAL) return false;

    // Key is released and was pressed in the previous frame
    return !state->current_keyboard.keys[code] && state->previous_keyboard.keys[code];
}


// -- Mouse -- //

KYRA_ENGINE_API Bool input_module_is_mouse_button_pressed(const InputMouseCode code) {
    if (!state || code >= KYRA_MOUSECODES_TOTAL) return false;

    // Mouse button is pressed
    return state->current_mouse.buttons[code];
}

KYRA_ENGINE_API Bool input_module_is_mouse_button_just_pressed(const InputMouseCode code) {
    if (!state || code >= KYRA_MOUSECODES_TOTAL) return false;

    // Mouse button is pressed and was not pressed in the previous frame
    return state->current_mouse.buttons[code] && !state->previous_mouse.buttons[code];
}

KYRA_ENGINE_API Bool input_module_is_mouse_button_just_released(const InputMouseCode code) {
    if (!state || code >= KYRA_MOUSECODES_TOTAL) return false;

    // Mouse button is released and was pressed in the previous frame
    return !state->current_mouse.buttons[code] && state->previous_mouse.buttons[code];
}

KYRA_ENGINE_API InputModuleResult input_module_get_mouse_position(Int32 *out_x, Int32 *out_y) {
    if (!state) return INPUT_MODULE_ERROR_NOT_INITIALISED;
    
    // Get mouse position
    if (out_x) *out_x = state->current_mouse.x;
    if (out_y) *out_y = state->current_mouse.y;

    return INPUT_MODULE_SUCCESS;
}


// -- Return codes -- //

KYRA_ENGINE_API ConstStr input_module_result_to_string(const InputModuleResult result) {
    switch (result) {
        case INPUT_MODULE_SUCCESS:                                          return "INPUT_MODULE_SUCCESS";

        case INPUT_MODULE_ERROR_ALREADY_INITIALISED:                        return "INPUT_MODULE_ERROR_ALREADY_INITIALISED";
        case INPUT_MODULE_ERROR_NOT_INITIALISED:                            return "INPUT_MODULE_ERROR_NOT_INITIALISED";
        case INPUT_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE:        return "INPUT_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE";
        case INPUT_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE:       return "INPUT_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE";
    
        default:                                                            return "UNKNOWN_INPUT_MODULE_RESULT";
    }
}


// Backend API functions --------------------------------------- //

KYRA_ENGINE_API void input_process_key(const InputKeyCode code, const Bool pressed) {
    if (!state || code >= KYRA_KEYCODES_TOTAL) return;

    // Process key 'press/release' state
    state->current_keyboard.keys[code] = pressed;
}

KYRA_ENGINE_API void input_process_mouse_button(const InputMouseCode code, const Bool pressed) {
    if (!state || code >= KYRA_MOUSECODES_TOTAL) return;

    // Process mouse button 'press/release' state
    state->current_mouse.buttons[code] = pressed;
}

KYRA_ENGINE_API void input_process_mouse_move(const Int32 x, const Int32 y) {
    if (!state) return;

    // Process mouse movement
    state->current_mouse.x = x;
    state->current_mouse.y = y;
}




