#include "kyra/core/engine/engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cJSON.h>

#include "kyra/core/platform/filesystem/filesystem.h"


// Engine state --------------------------------------------------------- //

typedef struct Engine_State {
    EngineConfig    config;
} EngineState;

static EngineState *state = NULL;


// Helper functions ----------------------------------------------------- //

EngineResult _engine_configure(ConstStr config_filepath, EngineConfig *out_config) {
    if (!out_config) return ENGINE_HELPER_ERROR_REF_OUT_CONFIG_NULL;

    FilesystemResult fs_result = 0; 

    // Open configuration file
    File config_file = {0};
    fs_result = platform_filesystem_file_open(config_filepath, FILESYSTEM_IO_MODE_READ, FILESYSTEM_FILE_MODE_BINARY, &config_file);
    if (fs_result != FILESYSTEM_SUCCESS) {
        printf("Error: Failed to open config file: %s (Error: %d)\n", config_filepath, fs_result);
        return ENGINE_HELPER_ERROR_FAILED_TO_OPEN_CONFIG_FILE;
    }

    // Get file size
    ByteSize file_size = 0;
    fs_result = platform_filesystem_file_size(&config_file, &file_size);
    if (fs_result != FILESYSTEM_SUCCESS) {
        printf("Error: Failed to get size of file: %s (Error: %d)\n", config_filepath, fs_result);
        return ENGINE_HELPER_ERROR_FAILED_TO_GET_FILE_SIZE;
    }

    // Allocate buffer to contain file data
    Str buffer = malloc(file_size + 1); // +1 for null-terminator
    if (!buffer) {
        // If data buffer failed to allocate, close the config file
        fs_result = platform_filesystem_file_close(&config_file);
        if (fs_result != FILESYSTEM_SUCCESS) {
            printf("Error: Failed to close file: %s (Error: %d)\n", config_filepath, fs_result);
            return ENGINE_HELPER_ERROR_FAILED_TO_CLOSE_CONFIG_FILE;
        }
    }

    // Read entire file
    ByteSize bytes_read = 0;
    fs_result = platform_filesystem_read_all(&config_file, &bytes_read, &buffer);

    // Close the config file
    fs_result = platform_filesystem_file_close(&config_file);
    if (fs_result != FILESYSTEM_SUCCESS) {
        free(buffer);
        printf("Error: Failed to close file: %s (Error: %d)\n", config_filepath, fs_result);
        return ENGINE_HELPER_ERROR_FAILED_TO_CLOSE_CONFIG_FILE;
    }

    // Parse to JSON
    cJSON *json = cJSON_Parse(buffer);    
    if (!json) {
        printf("Error: Failed to parse to JSON.\n");
        return ENGINE_HELPER_ERROR_FAILED_TO_PARSE_TO_JSON;
    }

    // Free data buffer
    free(buffer);


    // --- Info section --- //

    cJSON *sect_info = cJSON_GetObjectItemCaseSensitive(json, "info");
    if (sect_info) {
        // Author
        cJSON *sect_info_author = cJSON_GetObjectItemCaseSensitive(sect_info, "author");
        if (cJSON_IsString(sect_info_author)) out_config->author = _strdup(sect_info_author->valuestring);
        
        // Version
        cJSON *sect_info_ver = cJSON_GetObjectItemCaseSensitive(sect_info, "version");
        if (cJSON_IsString(sect_info_ver)) out_config->version = _strdup(sect_info_ver->valuestring);
    }


    printf("Author: %s\n", out_config->author);
    printf("Version: %s\n", out_config->version);

    return ENGINE_SUCCESS;
}


// API functions -------------------------------------------------------- //

KYRA_ENGINE_API EngineResult engine_preconstruct(ConstStr config_filepath) {
    if (state) return ENGINE_PRECONSTRUCT_ERROR_STATE_ALREADY_INITIALISED;
    if (!config_filepath) return ENGINE_PRECONSTRUCT_ERROR_CONFIG_FILEPATH_NULL;
    
    printf("Preconstructing engine...\n");
    
    // Allocate and reset engine state
    state = malloc(sizeof(EngineState));
    memset(state, 0, sizeof(EngineState));

    // Configure the engine
    EngineResult config_result = _engine_configure(config_filepath, &state->config);
    if (config_result != ENGINE_SUCCESS) {
        // In case engine configuration failed...

        // Free the state and set it to NULL
        free(state);
        state = NULL;

        return config_result;
    }
    
    printf("Engine preconstructed.\n");

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
    
    // Deallocate engine configuration properties
    {
        free(state->config.author);
        free(state->config.version);
    }

    // Deallocate engine state and set to NULL
    free(state);
    state = NULL;

    printf("Engine destructed.\n");

    return ENGINE_SUCCESS;
}


