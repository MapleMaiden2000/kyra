#include "kyra/core/platform/window/backends/glfw/backend_glfw.h"

#include <stdlib.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "kyra/core/memory/zone/memory_zone.h"
#include "kyra/core/logger/logger.h"


// Internal structure ---------------------------------------------- //

typedef struct Window_State {
    GLFWwindow         *handle;
    WindowCallbacks     callbacks;
    VoidPtr             context;

    // For allocations/deallocations
    ByteSize            memory_size;

} WindowState;


// GLFW callbacks -------------------------------------------------- //

static void _backend_glfw_error_callback(Int32 error, ConstStr description) {
    KYRA_LOG_ENGINE_ERROR("GLFW Error: %d >> %s", error, description);
}

static void _backend_glfw_window_close_callback(GLFWwindow *window) {
    WindowState *state = (WindowState *)glfwGetWindowUserPointer(window);
    if (state && state->callbacks.on_close) state->callbacks.on_close(state->context);
}

static void _backend_glfw_window_size_callback(GLFWwindow *window, Int32 width, Int32 height) {
    WindowState *state = (WindowState *)glfwGetWindowUserPointer(window);
    if (state && state->callbacks.on_resize) state->callbacks.on_resize(state->context, width, height);
}

static void _backend_glfw_window_refresh_callback(GLFWwindow *window) {
    // This allows continuous rendering during window resizing/moving
    // By delegating applicaation to draw new frame
    WindowState *state = (WindowState *)glfwGetWindowUserPointer(window);
    if (state && state->callbacks.on_refresh) state->callbacks.on_refresh(state->context);
}

static void _backend_glfw_window_iconify_callback(GLFWwindow *window, Int32 iconify) {
    WindowState *state = (WindowState *)glfwGetWindowUserPointer(window);
    if (state && state->callbacks.on_minimise) state->callbacks.on_minimise(state->context, iconify == GLFW_TRUE);
}

static void _backend_glfw_window_focus_callback(GLFWwindow *window, Int32 focused) {
    WindowState *state = (WindowState *)glfwGetWindowUserPointer(window);
    if (state && state->callbacks.on_focus) state->callbacks.on_focus(state->context, (Bool)focused);
}

// API functions --------------------------------------------------- //

WindowResult backend_glfw_startup(void) {
    // Set error callbacks
    glfwSetErrorCallback(_backend_glfw_error_callback);

    // Initialise GLFW
    if (!glfwInit()) return WINDOW_BACKEND_GLFW_ERROR_FAILED_TO_INITIALISE;

    return WINDOW_SUCCESS;
}

WindowResult backend_glfw_shutdown(void) {
    // Terminate GLFW
    glfwTerminate();

    return WINDOW_SUCCESS;
}

WindowResult backend_glfw_construct_window(const WindowConfigs configs, const WindowCallbacks *callbacks, const VoidPtr context, Window *out_window) {
    if (!out_window) return WINDOW_BACKEND_GLFW_ERROR_REF_OUT_WINDOW_NULL;

    // Allocate for window state
    WindowState *state = NULL;
    ByteSize mem_size = 0;
    if (memory_zone_allocate("platform", sizeof(WindowState), (VoidPtr *)&state, &mem_size) != MEMORY_ZONE_SUCCESS)
        return WINDOW_BACKEND_GLFW_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_WINDOW_STATE;

    // Set properties
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);  // v[3].3
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);  // v3.[3]
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);       // Hidden until setup is done
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);      // Make resizable
        glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_TRUE);  // Make focused upon showed

        // Borderless window
        if (configs.mode == WINDOW_MODE_BORDERLESS) glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    }   
    
    // Get monitor for fullscreen mode
    GLFWmonitor *monitor = (configs.mode == WINDOW_MODE_FULLSCREEN) ? glfwGetPrimaryMonitor() : NULL;
    
    // Construct window
    state->handle = glfwCreateWindow(
        configs.width,      // Width
        configs.height,     // Height
        configs.title,      // Title
        monitor,            // Monitor for fullscreen mode
        NULL                // Share context with another window
    );

    // Validate
    if (!state->handle) {
        // Window handle was not constructed successfully...

        // Deallocate window state
        if (memory_zone_deallocate("platform", (VoidPtr)state, mem_size) != MEMORY_ZONE_SUCCESS)
            return WINDOW_BACKEND_GLFW_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_WINDOW_STATE;

        return WINDOW_BACKEND_GLFW_ERROR_FAILED_TO_CONSTRUCT_WINDOW_HANDLE;
    }

    // Assign properties
    {
        // Callbacks
        if (callbacks) state->callbacks = *callbacks;

        // Context
        state->context = context;

        // Memory size
        state->memory_size = mem_size;
    }

    // Set window user pointer
    // Make context current
    glfwSetWindowUserPointer(state->handle, (VoidPtr)state);
    glfwMakeContextCurrent(state->handle);

    // Set vsync
    glfwSwapInterval(configs.vsync ? 1 : 0);

    // Register callbacks
    {
        glfwSetWindowCloseCallback(state->handle, _backend_glfw_window_close_callback);
        glfwSetWindowSizeCallback(state->handle, _backend_glfw_window_size_callback);
        glfwSetWindowRefreshCallback(state->handle, _backend_glfw_window_refresh_callback);
        glfwSetWindowIconifyCallback(state->handle, _backend_glfw_window_iconify_callback);
        glfwSetWindowFocusCallback(state->handle, _backend_glfw_window_focus_callback);
    }

    // Show window
    glfwShowWindow(state->handle);

    // Save to ref
    *out_window = (Window)state;

    return WINDOW_SUCCESS;
}

WindowResult backend_glfw_destruct_window(Window window) {
    if (!window) return WINDOW_BACKEND_GLFW_ERROR_WINDOW_NULL;

    WindowState *state = (WindowState *)window;

    // Make context not current
    glfwMakeContextCurrent(NULL);

    // Destroy window
    glfwDestroyWindow(state->handle);

    // Deallocate window state
    if (memory_zone_deallocate("platform", (VoidPtr)state, state->memory_size) != MEMORY_ZONE_SUCCESS)
        return WINDOW_BACKEND_GLFW_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_WINDOW_STATE;

    return WINDOW_SUCCESS;
}

WindowResult backend_glfw_poll_events(void) {
    // GLFW polls for all windows from the same thread
    glfwPollEvents();

    return WINDOW_SUCCESS;
}

WindowResult backend_glfw_swap_buffers(Window window) {
    if (!window) return WINDOW_BACKEND_GLFW_ERROR_WINDOW_NULL;

    WindowState *state = (WindowState *)window;

    // Swap buffers
    glfwSwapBuffers(state->handle);

    return WINDOW_SUCCESS;
}

WindowResult backend_glfw_set_vsync(Window window, const Bool vsync) {
    if (!window) return WINDOW_BACKEND_GLFW_ERROR_WINDOW_NULL;

    WindowState *state = (WindowState *)window;

    // Make context current
    // Set vsync
    glfwMakeContextCurrent(state->handle);
    glfwSwapInterval(vsync ? 1 : 0);

    return WINDOW_SUCCESS;
}

WindowResult backend_glfw_set_title(Window window, ConstStr title) {
    if (!window) return WINDOW_BACKEND_GLFW_ERROR_WINDOW_NULL;

    WindowState *state = (WindowState *)window;

    // Set title
    glfwSetWindowTitle(state->handle, title);
    
    return WINDOW_SUCCESS;
}

WindowResult backend_glfw_set_size(Window window, const Int32 width, const Int32 height) {
    if (!window) return WINDOW_BACKEND_GLFW_ERROR_WINDOW_NULL;

    WindowState *state = (WindowState *)window;

    // Set size
    glfwSetWindowSize(state->handle, width, height);

    return WINDOW_SUCCESS;
}

ConstStr backend_glfw_window_title(const Window window) {
    if (!window) return NULL;

    WindowState *state = (WindowState *)window;

    return glfwGetWindowTitle(state->handle);
}

Int32 backend_glfw_window_width(const Window window) {
    if (!window) return -1;

    WindowState *state = (WindowState *)window;
    
    // Get width
    Int32 width = -1;
    glfwGetWindowSize(state->handle, &width, NULL);
    
    return width;
}

Int32 backend_glfw_window_height(const Window window) {
    if (!window) return -1;

    WindowState *state = (WindowState *)window;
    
    // Get height
    Int32 height = -1;
    glfwGetWindowSize(state->handle, NULL, &height);
    
    return height;
}

Int32 backend_glfw_window_framebuffer_width(const Window window) {
    if (!window) return -1;

    WindowState *state = (WindowState *)window;
    
    // Get frame-buffer width
    Int32 width = -1;
    glfwGetFramebufferSize(state->handle, &width, NULL);
    
    return width;
}

Int32 backend_glfw_window_framebuffer_height(const Window window) {
    if (!window) return -1;

    WindowState *state = (WindowState *)window;
    
    // Get frame-buffer height
    Int32 height = -1;
    glfwGetFramebufferSize(state->handle, NULL, &height);
    
    return height;
}

Flt32 backend_glfw_window_xscale(const Window window) {
    if (!window) return 0.0f;

    WindowState *state = (WindowState *)window;
    
    // Get x of content scale
    Flt32 xscale = 0.0f;
    glfwGetWindowContentScale(state->handle, &xscale, NULL);
    
    return xscale;
}

Flt32 backend_glfw_window_yscale(const Window window) {
    if (!window) return 0.0f;

    WindowState *state = (WindowState *)window;
    
    // Get y of content scale
    Flt32 yscale = 0.0f;
    glfwGetWindowContentScale(state->handle, NULL, &yscale);
    
    return yscale;
}

Bool backend_glfw_should_close(const Window window) {
    if (!window) return false;
    
    WindowState *state = (WindowState *)window;

    // Check if handle should close
    return (Bool)glfwWindowShouldClose(state->handle);
}

VoidPtr backend_glfw_raw_handle(const Window window) {
    if (!window) return NULL;
    
    WindowState *state = (WindowState *)window;

    return (VoidPtr)state->handle;
}

ConstStr backend_glfw_result_to_string(const WindowResult result) {
    switch (result) {
        case WINDOW_SUCCESS:                                                                return "WINDOW_SUCCESS";

        case WINDOW_BACKEND_GLFW_ERROR_REF_OUT_WINDOW_NULL:                                 return "WINDOW_BACKEND_GLFW_ERROR_REF_OUT_WINDOW_NULL";
        case WINDOW_BACKEND_GLFW_ERROR_WINDOW_NULL:                                         return "WINDOW_BACKEND_GLFW_ERROR_WINDOW_NULL";
        case WINDOW_BACKEND_GLFW_ERROR_FAILED_TO_INITIALISE:                                return "WINDOW_BACKEND_GLFW_ERROR_FAILED_TO_INITIALISE";
        case WINDOW_BACKEND_GLFW_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_WINDOW_STATE:          return "WINDOW_BACKEND_GLFW_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_WINDOW_STATE";
        case WINDOW_BACKEND_GLFW_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_WINDOW_STATE:         return "WINDOW_BACKEND_GLFW_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_WINDOW_STATE";
        case WINDOW_BACKEND_GLFW_ERROR_FAILED_TO_CONSTRUCT_WINDOW_HANDLE:                   return "WINDOW_BACKEND_GLFW_ERROR_FAILED_TO_CONSTRUCT_WINDOW_HANDLE";
        
        default:                                                                            return "UNKNOWN_WINDOW_RESULT";
    }
}

