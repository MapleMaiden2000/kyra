#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/window.h"


// API functions ------------------------------------------------- //

KYRA_ENGINE_API WindowModuleResult      window_module_startup(void);
KYRA_ENGINE_API WindowModuleResult      window_module_shutdown(void);
KYRA_ENGINE_API WindowModuleResult      window_module_update(void);

KYRA_ENGINE_API WindowModuleResult      window_module_construct_window(const WindowConfigs configs, const WindowCallbacks *callbacks, const VoidPtr context, Window *out_window);
KYRA_ENGINE_API WindowModuleResult      window_module_destruct_window(Window window);

KYRA_ENGINE_API ConstStr                window_module_result_to_string(const WindowModuleResult result);

