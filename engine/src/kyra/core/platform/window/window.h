#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/window.h"


// Backend options ------------------------------------------------- //

typedef enum Window_Backend {
    WINDOW_BACKEND_GLFW,
    
} WindowBackend;


// API functions --------------------------------------------------- //

KYRA_ENGINE_API WindowResult    platform_window_startup(const WindowBackend backend);
KYRA_ENGINE_API WindowResult    platform_window_shutdown(void);

KYRA_ENGINE_API WindowResult    platform_window_construct_window(const WindowConfigs configs, const WindowCallbacks *callbacks, const VoidPtr context, Window *out_window);
KYRA_ENGINE_API WindowResult    platform_window_destruct_window(Window window);

KYRA_ENGINE_API WindowResult    platform_window_poll_events(void);
KYRA_ENGINE_API WindowResult    platform_window_swap_buffers(Window window);

KYRA_ENGINE_API WindowResult    platform_window_set_vsync(Window window, const Bool vsync);
KYRA_ENGINE_API WindowResult    platform_window_set_title(Window window, ConstStr title);
KYRA_ENGINE_API WindowResult    platform_window_set_size(Window window, const Int32 width, const Int32 height);

KYRA_ENGINE_API ConstStr        platform_window_get_window_title(const Window window);
KYRA_ENGINE_API Int32           platform_window_get_window_width(const Window window); 
KYRA_ENGINE_API Int32           platform_window_get_window_height(const Window window);
KYRA_ENGINE_API Int32           platform_window_get_window_framebuffer_width(const Window window);
KYRA_ENGINE_API Int32           platform_window_get_window_framebuffer_height(const Window window);
KYRA_ENGINE_API Flt32           platform_window_get_window_xscale(const Window window);
KYRA_ENGINE_API Flt32           platform_window_get_window_yscale(const Window window);
KYRA_ENGINE_API Bool            platform_window_get_should_close(const Window window);
KYRA_ENGINE_API VoidPtr         platform_window_get_raw_handle(const Window window);

KYRA_ENGINE_API ConstStr        platform_window_result_to_string(const WindowResult result);
