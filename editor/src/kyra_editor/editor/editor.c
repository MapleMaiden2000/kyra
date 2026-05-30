#include "kyra_editor/editor/editor.h"

#include <string.h>

#include <cJSON.h>

#include <kyra/core/logger/logger.h>
#include <kyra/core/memory/zone/memory_zone.h>
#include <kyra/core/platform/filesystem/filesystem.h>
#include <kyra/core/platform/window/window.h>
#include <kyra/core/modules/window/window_module.h>
#include <kyra/core/containers/string/string.h>
#include <kyra/core/misc/console/console.h>


// Internal state ------------------------------------------------- //

typedef struct Editor_State {
    Window          window;
    Bool            closing;

    EditorConfig    config;

    // For allocations/deallocation
    ByteSize        memory_size;

} EditorState;

static EditorState *state = NULL;


// Helper functions ----------------------------------------------- //

EditorResult _editor_configure(ConstStr config_filepath) {
	if (!config_filepath) return EDITOR_HELPER_ERROR_CONFIG_FILEPATH_NULL;

    FilesystemResult fs_result = 0; 

    // Open configuration file
    File config_file = {0};
    fs_result = platform_filesystem_file_open(config_filepath, FILESYSTEM_IO_MODE_READ, FILESYSTEM_FILE_MODE_BINARY, &config_file);
    if (fs_result != FILESYSTEM_SUCCESS) {
        KYRA_PRINT_ERROR("Editor: Failed to open config file: %s (Error: %s)", config_filepath, platform_filesystem_result_to_string(fs_result));
        
        return EDITOR_HELPER_ERROR_FAILED_TO_OPEN_CONFIG_FILE;
    }

    // Get file size
    ByteSize file_size = 0;
    fs_result = platform_filesystem_file_size(&config_file, &file_size);
    if (fs_result != FILESYSTEM_SUCCESS) {
        KYRA_PRINT_ERROR("Editor: Failed to get size of file: %s (Error: %s)", config_filepath, platform_filesystem_result_to_string(fs_result));
        
        return EDITOR_HELPER_ERROR_FAILED_TO_GET_FILE_SIZE;
    }

    // Allocate raw buffer to contain file data
    Str buffer = NULL; 
    ByteSize buffer_memsize = 0;
    MemoryZoneResult mz_result = memory_zone_allocate("editor", file_size, (VoidPtr *)&buffer, &buffer_memsize);
    if (mz_result != MEMORY_ZONE_SUCCESS) {
        KYRA_PRINT_ERROR("Editor: Failed to get allocate memory for raw buffer (Error: %s)", config_filepath, memory_zone_result_to_string(mz_result));

        return EDITOR_HELPER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_CONFIG_RAW_BUFFER;
    }

    if (!buffer) {
        // If data buffer failed to allocate, close the config file
        fs_result = platform_filesystem_file_close(&config_file);
        if (fs_result != FILESYSTEM_SUCCESS) {
            KYRA_PRINT_ERROR("Editor: Failed to close file: %s (Error: %s)", config_filepath, platform_filesystem_result_to_string(fs_result));
            
            return EDITOR_HELPER_ERROR_FAILED_TO_CLOSE_CONFIG_FILE;
        }
    }

    // Read entire file
    ByteSize bytes_read = 0;
    fs_result = platform_filesystem_read_all(&config_file, &bytes_read, &buffer);
    if (fs_result != FILESYSTEM_SUCCESS) {
        KYRA_PRINT_ERROR("Editor: Failed to read file: %s (Error: %s)", config_filepath, platform_filesystem_result_to_string(fs_result));
        
        // Deallocate raw buffer 
        if (memory_zone_deallocate("editor", (VoidPtr)buffer, buffer_memsize) != MEMORY_ZONE_SUCCESS)
            return EDITOR_HELPER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_CONFIG_RAW_BUFFER;
        
        return EDITOR_HELPER_ERROR_FAILED_TO_READ_CONFIG_FILE;
    }

    // Null-terminate
    buffer[bytes_read] = '\0';

    // Close configuration file
    fs_result = platform_filesystem_file_close(&config_file);
    if (fs_result != FILESYSTEM_SUCCESS) {
        KYRA_PRINT_ERROR("Editor: Failed to close file: %s (Error: %s)", config_filepath, platform_filesystem_result_to_string(fs_result));
    
        // Deallocate raw buffer 
        if (memory_zone_deallocate("editor", (VoidPtr)buffer, buffer_memsize) != MEMORY_ZONE_SUCCESS)
            return EDITOR_HELPER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_CONFIG_RAW_BUFFER;
        
        return EDITOR_HELPER_ERROR_FAILED_TO_CLOSE_CONFIG_FILE;
    }

    // Parse to JSON
    cJSON *json = cJSON_Parse(buffer);    
    if (!json) {
        KYRA_PRINT_ERROR("Editor: Failed to parse to JSON.");
        
        return EDITOR_HELPER_ERROR_FAILED_TO_PARSE_TO_JSON;
    }

    // Deallocate raw buffer 
    if (memory_zone_deallocate("editor", (VoidPtr)buffer, buffer_memsize) != MEMORY_ZONE_SUCCESS)
        return EDITOR_HELPER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_CONFIG_RAW_BUFFER;

    // Locate window section inside config json
    cJSON *window = cJSON_GetObjectItemCaseSensitive(json, "window");
    if (!window) {
        // Failed to locate...

        KYRA_PRINT_ERROR("Editor: Failed to locate section 'window' in JSON config.\n Config: %s", json->valuestring);

        // Delete config JSON object
        cJSON_Delete(json);
        
        return LOGGER_ERROR_FAILED_TO_LOCATE_LOG_SYSTEM_SECTION_IN_CONFIG_JSON;
    }

    // Assign window configurations
    {
        cJSON *title = cJSON_GetObjectItemCaseSensitive(window, "title");
        if (container_string_construct(title->valuestring, &state->config.window_title) != CONTAINER_SUCCESS)
            return EDITOR_HELPER_ERROR_FAILED_TO_CONSTRUCT_WINDOW_TITLE_STRING;

        cJSON *width = cJSON_GetObjectItemCaseSensitive(window, "width");
		if (cJSON_IsNumber(width)) state->config.window_width = (UInt32)width->valueint;
		
		cJSON *height = cJSON_GetObjectItemCaseSensitive(window, "height");
		if (cJSON_IsNumber(height)) state->config.window_height = (UInt32)height->valueint;
		
		cJSON *fullscreen = cJSON_GetObjectItemCaseSensitive(window, "fullscreen");
		if (cJSON_IsBool(fullscreen)) state->config.window_fullscreen = cJSON_IsTrue(fullscreen);

        cJSON *vsync = cJSON_GetObjectItemCaseSensitive(window, "vsync");
		if (cJSON_IsBool(vsync)) state->config.window_vsync = cJSON_IsTrue(vsync);
    }
    
    // Delete config JSON object
    cJSON_Delete(json);

    return EDITOR_SUCCESS;
}


// API functions -------------------------------------------------- //

KYRA_EDITOR_API EditorResult editor_startup(ConstStr config_filepath) {
    if (state) return EDITOR_ERROR_ALREADY_INITIALISED;

    // Allocate for state
    ByteSize mem_size = 0;
    if (memory_zone_allocate("editor", sizeof(EditorState), (VoidPtr *)&state, &mem_size) != MEMORY_ZONE_SUCCESS)
        return EDITOR_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE;

    // Set state properties zero
    memset(state, 0, sizeof(EditorState));

    // Window module startup
    if (window_module_startup() != WINDOW_MODULE_SUCCESS) {
        KYRA_LOG_EDITOR_ERROR("Window module startup failed.");

        // Deallocate state
        if (memory_zone_deallocate("editor", (VoidPtr)state, state->memory_size) != MEMORY_ZONE_SUCCESS)
            return EDITOR_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE;

        // Set to NULL
        state = NULL;

        return EDITOR_ERROR_WINDOW_MODULE_STARTUP_FAILED;
    }

    // Configure editor
    EditorResult config_result = _editor_configure(config_filepath); 
    if (config_result != EDITOR_SUCCESS) {
        KYRA_LOG_EDITOR_ERROR("Failed to configure editor.");
        
        // Deallocate state
        if (memory_zone_deallocate("editor", (VoidPtr)state, state->memory_size) != MEMORY_ZONE_SUCCESS)
            return EDITOR_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE;

        // Set to NULL
        state = NULL;

        return EDITOR_ERROR_FAILED_TO_CONFIGURE_EDITOR;
    }

    WindowConfigs window_configs = {
        .title = container_string_cstr(state->config.window_title),
        .width = state->config.window_width,
        .height = state->config.window_height,
        .vsync = state->config.window_vsync,
        .mode = state->config.window_fullscreen ? WINDOW_MODE_FULLSCREEN : WINDOW_MODE_WINDOWED
    };
    
    // Construct editor window
    if (window_module_construct_window(window_configs, NULL, NULL, &state->window) != WINDOW_MODULE_SUCCESS) {
        KYRA_LOG_EDITOR_ERROR("Failed to construct window.");

        // Window module shutdown
        if (window_module_shutdown() != WINDOW_MODULE_SUCCESS)
            KYRA_LOG_EDITOR_ERROR("Window module shutdown failed.");

        // Deallocate state
        if (memory_zone_deallocate("editor", (VoidPtr)state, state->memory_size) != MEMORY_ZONE_SUCCESS)
            return EDITOR_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE;

        // Set to NULL
        state = NULL;

        return EDITOR_ERROR_FAILED_TO_CONSTRUCT_WINDOW_FOR_EDITOR;
    }

    state->memory_size = mem_size;

    KYRA_LOG_EDITOR_INFO("Editor is open.");

    return EDITOR_SUCCESS;
}

KYRA_EDITOR_API EditorResult editor_shutdown(void) {
    if (!state) return EDITOR_ERROR_NOT_INITIALISED;

    KYRA_LOG_EDITOR_INFO("Closing editor...");

    if (state->window) {
        if (window_module_destruct_window(state->window) != WINDOW_MODULE_SUCCESS)
            KYRA_LOG_EDITOR_ERROR("Failed to destruct window.");
    }

    // Window module shutdown
    if (window_module_shutdown() != WINDOW_MODULE_SUCCESS)
        KYRA_LOG_EDITOR_ERROR("Window module shutdown failed.");

    // Clear configurations
    {
        if (container_string_destruct(&state->config.window_title) != CONTAINER_SUCCESS)
            KYRA_LOG_EDITOR_ERROR("Failed to destruct window title string.");

        memset(&state->config, 0, sizeof(EditorConfig));
    }

    // Deallocate state
    if (memory_zone_deallocate("editor", (VoidPtr)state, state->memory_size) != MEMORY_ZONE_SUCCESS)
        return EDITOR_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE;

    // Set to NULL
    state = NULL;

    KYRA_LOG_EDITOR_INFO("Editor closed.");
    return EDITOR_SUCCESS;
}

KYRA_EDITOR_API EditorResult editor_update(void) {
    if (!state) return EDITOR_ERROR_NOT_INITIALISED;

    // Poll events
    window_module_update();

    if (state->window) platform_window_swap_buffers(state->window);

    return EDITOR_SUCCESS;
}

KYRA_EDITOR_API EditorResult editor_set_title(ConstStr title) {
    if (!state) return EDITOR_ERROR_NOT_INITIALISED;

    if (state->window) platform_window_set_title(state->window, title);

    return EDITOR_SUCCESS;
}

KYRA_EDITOR_API EditorResult editor_set_size(Int32 width, Int32 height) {
    if (!state) return EDITOR_ERROR_NOT_INITIALISED;

    if (state->window) platform_window_set_size(state->window, width, height);

    return EDITOR_SUCCESS;
}

KYRA_EDITOR_API EditorResult editor_set_vsync(Bool vsync) {
    if (!state) return EDITOR_ERROR_NOT_INITIALISED;

    if (state->window) platform_window_set_vsync(state->window, vsync);

    return EDITOR_SUCCESS;
}

KYRA_EDITOR_API EditorResult editor_request_shutdown(void) {
    if (!state) return EDITOR_ERROR_NOT_INITIALISED;

    // Flag editor state to close window
    state->closing = true;

    return EDITOR_SUCCESS;
}

KYRA_EDITOR_API Bool editor_should_close(void) {
    if (!state) return false;

    return state->closing || platform_window_get_should_close(state->window);
}

KYRA_EDITOR_API ConstStr editor_result_to_string(const EditorResult result) {
    switch (result) {
        case EDITOR_SUCCESS:                                                                return "EDITOR_SUCCESS";

        case EDITOR_HELPER_ERROR_CONFIG_FILEPATH_NULL:                                      return "EDITOR_HELPER_ERROR_CONFIG_FILEPATH_NULL";
        case EDITOR_HELPER_ERROR_FAILED_TO_OPEN_CONFIG_FILE:                                return "EDITOR_HELPER_ERROR_FAILED_TO_OPEN_CONFIG_FILE";
        case EDITOR_HELPER_ERROR_FAILED_TO_GET_FILE_SIZE:                                   return "EDITOR_HELPER_ERROR_FAILED_TO_GET_FILE_SIZE";
        case EDITOR_HELPER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_CONFIG_RAW_BUFFER:           return "EDITOR_HELPER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_CONFIG_RAW_BUFFER";
        case EDITOR_HELPER_ERROR_FAILED_TO_CLOSE_CONFIG_FILE:                               return "EDITOR_HELPER_ERROR_FAILED_TO_CLOSE_CONFIG_FILE";
        case EDITOR_HELPER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_CONFIG_RAW_BUFFER:          return "EDITOR_HELPER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_CONFIG_RAW_BUFFER";
        case EDITOR_HELPER_ERROR_FAILED_TO_READ_CONFIG_FILE:                                return "EDITOR_HELPER_ERROR_FAILED_TO_READ_CONFIG_FILE";
        case EDITOR_HELPER_ERROR_FAILED_TO_PARSE_TO_JSON:                                   return "EDITOR_HELPER_ERROR_FAILED_TO_PARSE_TO_JSON";
        case EDITOR_HELPER_ERROR_FAILED_TO_CONSTRUCT_WINDOW_TITLE_STRING:                   return "EDITOR_HELPER_ERROR_FAILED_TO_CONSTRUCT_WINDOW_TITLE_STRING";
        
        case EDITOR_ERROR_ALREADY_INITIALISED:                                              return "EDITOR_ERROR_ALREADY_INITIALISED";
        case EDITOR_ERROR_NOT_INITIALISED:                                                  return "EDITOR_ERROR_NOT_INITIALISED";
        case EDITOR_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE:                              return "EDITOR_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE";
        case EDITOR_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE:                             return "EDITOR_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE";
        case EDITOR_ERROR_WINDOW_MODULE_STARTUP_FAILED:                                     return "EDITOR_ERROR_WINDOW_MODULE_STARTUP_FAILED";
        case EDITOR_ERROR_FAILED_TO_CONFIGURE_EDITOR:                                       return "EDITOR_ERROR_FAILED_TO_CONFIGURE_EDITOR";
        case EDITOR_ERROR_FAILED_TO_CONSTRUCT_WINDOW_FOR_EDITOR:                            return "EDITOR_ERROR_FAILED_TO_CONSTRUCT_WINDOW_FOR_EDITOR";
    
        default:                                                                            return "UNKNOWN_EDITOR_RESULT";
    }
}




