#pragma once

#include "kyra/defines/core/window.h"


// API functions --------------------------------------------------- //

WindowResult    backend_glfw_startup(void);
WindowResult    backend_glfw_shutdown(void);

WindowResult    backend_glfw_construct_window(const WindowConfigs configs, const WindowCallbacks *callbacks, const VoidPtr context, Window *out_window);
WindowResult    backend_glfw_destruct_window(Window window);

WindowResult    backend_glfw_poll_events(void);
WindowResult    backend_glfw_swap_buffers(Window window);

WindowResult    backend_glfw_set_vsync(Window window, const Bool vsync);
WindowResult    backend_glfw_set_title(Window window, ConstStr title);
WindowResult    backend_glfw_set_size(Window window, const Int32 width, const Int32 height);

ConstStr        backend_glfw_window_title(const Window window);
Int32           backend_glfw_window_width(const Window window); 
Int32           backend_glfw_window_height(const Window window);
Int32           backend_glfw_window_framebuffer_width(const Window window);
Int32           backend_glfw_window_framebuffer_height(const Window window);
Flt32           backend_glfw_window_xscale(const Window window);
Flt32           backend_glfw_window_yscale(const Window window);
Bool            backend_glfw_should_close(const Window window);
VoidPtr         backend_glfw_raw_handle(const Window window);

ConstStr        backend_glfw_result_to_string(const WindowResult result);


