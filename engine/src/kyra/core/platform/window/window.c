#include "kyra/core/platform/window/window.h"

#include <stdlib.h>
#include <string.h>

#include "kyra/core/platform/window/backends/glfw/backend_glfw.h"


// Backend interface ---------------------------------------------------- //

typedef struct Window_Backend_Interface {
    WindowResult    (*startup)(void);
    WindowResult    (*shutdown)(void);

    WindowResult    (*construct_window)(const WindowConfigs, const WindowCallbacks *, const VoidPtr, Window *);
    WindowResult    (*destruct_window)(Window);
    
    WindowResult    (*poll_events)(void);
    WindowResult    (*swap_buffers)(Window);

    WindowResult    (*set_vsync)(Window, const Bool);
    WindowResult    (*set_title)(Window, ConstStr);
    WindowResult    (*set_size)(Window, const Int32, const Int32);

    ConstStr        (*window_title)(const Window);
    Int32           (*window_width)(const Window); 
    Int32           (*window_height)(const Window);
    Int32           (*window_framebuffer_width)(const Window);
    Int32           (*window_framebuffer_height)(const Window);
    Flt32           (*window_xscale)(const Window);
    Flt32           (*window_yscale)(const Window);
    Bool            (*should_close)(const Window);
    VoidPtr         (*raw_handle)(const Window);

} WindowBackendInterface;


// Backend APIs --------------------------------------------------------- //

static const WindowBackendInterface backend_api_glfw = {
    backend_glfw_startup,
    backend_glfw_shutdown,

    backend_glfw_construct_window,
    backend_glfw_destruct_window,

    backend_glfw_poll_events,
    backend_glfw_swap_buffers,

    backend_glfw_set_vsync,
    backend_glfw_set_title,
    backend_glfw_set_size,

    backend_glfw_window_title,
    backend_glfw_window_width,
    backend_glfw_window_height,
    backend_glfw_window_framebuffer_width,
    backend_glfw_window_framebuffer_height,
    backend_glfw_window_xscale,
    backend_glfw_window_yscale,
    backend_glfw_should_close,
    backend_glfw_raw_handle,
};


// Internal state -------------------------------------------------- //

static WindowBackendInterface *backend_api = NULL;


// API functions --------------------------------------------------- //

KYRA_ENGINE_API WindowResult platform_window_startup(const WindowBackend backend) {
    if (backend_api) return WINDOW_ERROR_ALREADY_INITIALISED;

    // Select backend
    switch (backend) {
        case WINDOW_BACKEND_GLFW:
            backend_api = (WindowBackendInterface *)&backend_api_glfw;
            break;

        default:
            return WINDOW_ERROR_INVALID_BACKEND_OPTION;
    }

    // Backend startup
    return backend_api->startup();
}

KYRA_ENGINE_API WindowResult platform_window_shutdown(void) {
    if (!backend_api) return WINDOW_ERROR_NOT_INITIALISED;

    // Backend shutdown
    WindowResult shutdown_result = backend_api->shutdown();
    if (shutdown_result != WINDOW_SUCCESS) return shutdown_result;

    // Set backend api to NULL
    backend_api = NULL;

    return shutdown_result;
}

KYRA_ENGINE_API WindowResult platform_window_construct_window(const WindowConfigs configs, const WindowCallbacks *callbacks, const VoidPtr context, Window *out_window) {
    if (!backend_api) return WINDOW_ERROR_NOT_INITIALISED;

    // Construct window
    return backend_api->construct_window(configs, callbacks, context, out_window);
}   

KYRA_ENGINE_API WindowResult platform_window_destruct_window(Window window) {
    if (!backend_api) return WINDOW_ERROR_NOT_INITIALISED;

    // Destruct window
    return backend_api->destruct_window(window);
}

KYRA_ENGINE_API WindowResult platform_window_poll_events(void) {
    if (!backend_api) return WINDOW_ERROR_NOT_INITIALISED;

    // Poll window events
    return backend_api->poll_events();
}

KYRA_ENGINE_API WindowResult platform_window_swap_buffers(Window window) {
    if (!backend_api) return WINDOW_ERROR_NOT_INITIALISED;

    // Swap buffers
    return backend_api->swap_buffers(window);
}

KYRA_ENGINE_API WindowResult platform_window_set_vsync(Window window, const Bool vsync) {
    if (!backend_api) return WINDOW_ERROR_NOT_INITIALISED;

    // Set window vsync setting
    return backend_api->set_vsync(window, vsync);
}

KYRA_ENGINE_API WindowResult platform_window_set_title(Window window, ConstStr title) {
    if (!backend_api) return WINDOW_ERROR_NOT_INITIALISED;

    // Set window title
    return backend_api->set_title(window, title);
}

KYRA_ENGINE_API WindowResult platform_window_set_size(Window window, const Int32 width, const Int32 height) {
    if (!backend_api) return WINDOW_ERROR_NOT_INITIALISED;

    // Set window size
    return backend_api->set_size(window, width, height);
}

KYRA_ENGINE_API ConstStr platform_window_get_window_title(const Window window) {
    if (!backend_api) return NULL;

    // Get window title
    return backend_api->window_title(window);
}

KYRA_ENGINE_API Int32 platform_window_get_window_width(const Window window) {
    if (!backend_api) return -1;

    // Get window width
    return backend_api->window_width(window);
}

KYRA_ENGINE_API Int32 platform_window_get_window_height(const Window window) {
    if (!backend_api) return -1;

    // Get window height
    return backend_api->window_height(window);
}

KYRA_ENGINE_API Int32 platform_window_get_window_framebuffer_width(const Window window) {
    if (!backend_api) return -1;

    // Get window's frame-buffer width
    return backend_api->window_framebuffer_width(window);
}

KYRA_ENGINE_API Int32 platform_window_get_window_framebuffer_height(const Window window) {
    if (!backend_api) return -1;

    // Get window's frame-buffer height
    return backend_api->window_framebuffer_height(window);
}

KYRA_ENGINE_API Flt32 platform_window_get_window_xscale(const Window window) {
    if (!backend_api) return 0.0f;

    // Get x of window content scale
    return backend_api->window_xscale(window);
}

KYRA_ENGINE_API Flt32 platform_window_get_window_yscale(const Window window) {
    if (!backend_api) return 0.0f;

    // Get y of window content scale
    return backend_api->window_yscale(window);
}

KYRA_ENGINE_API Bool platform_window_get_should_close(const Window window) {
    if (!backend_api) return false;

    // Get window 'should_close' status
    return backend_api->should_close(window);
}

KYRA_ENGINE_API VoidPtr platform_window_get_raw_handle(const Window window) {
    if (!backend_api) return NULL;

    // Get raw handle
    return backend_api->raw_handle(window);
}

KYRA_ENGINE_API ConstStr platform_window_result_to_string(const WindowResult result) {
    switch (result) {
        case WINDOW_SUCCESS:                                                                return "WINDOW_SUCCESS";

        case WINDOW_ERROR_ALREADY_INITIALISED:                                              return "WINDOW_ERROR_ALREADY_INITIALISED";
        case WINDOW_ERROR_NOT_INITIALISED:                                                  return "WINDOW_ERROR_NOT_INITIALISED";
        case WINDOW_ERROR_INVALID_BACKEND_OPTION:                                           return "WINDOW_ERROR_INVALID_BACKEND_OPTION";
    
        default:                                                                            return "UNKNOWN_WINDOW_RESULT";
    }
}

