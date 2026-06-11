#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/input.h"
#include "kyra/defines/core/delegates.h"


// API functions ----------------------------------------------- //

KYRA_ENGINE_API InputModuleResult   input_module_startup(void);
KYRA_ENGINE_API InputModuleResult   input_module_shutdown(void);

KYRA_ENGINE_API InputModuleResult   input_module_update(void);


// -- Keyboard -- //

KYRA_ENGINE_API Bool                input_module_is_key_pressed(const InputKeyCode code);
KYRA_ENGINE_API Bool                input_module_is_key_just_pressed(const InputKeyCode code);
KYRA_ENGINE_API Bool                input_module_is_key_just_released(const InputKeyCode code);


// -- Mouse -- //

KYRA_ENGINE_API Bool                input_module_is_mouse_button_pressed(const InputMouseCode code);
KYRA_ENGINE_API Bool                input_module_is_mouse_button_just_pressed(const InputMouseCode code);
KYRA_ENGINE_API Bool                input_module_is_mouse_button_just_released(const InputMouseCode code);

KYRA_ENGINE_API InputModuleResult   input_module_get_mouse_position(Int32 *out_x, Int32 *out_y);


// -- Return codes -- //

KYRA_ENGINE_API ConstStr            input_module_result_to_string(const InputModuleResult result); 


// Backend API functions --------------------------------------- //

KYRA_ENGINE_API void                input_process_key(const InputKeyCode code, const Bool pressed);
KYRA_ENGINE_API void                input_process_mouse_button(const InputMouseCode code, const Bool pressed);
KYRA_ENGINE_API void                input_process_mouse_move(const Int32 x, const Int32 y);


