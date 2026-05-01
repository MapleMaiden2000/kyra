#include "kyra/core/engine/engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cJSON.h>

#include "kyra/core/platform/filesystem/filesystem.h"
#include "kyra/core/memory/manager/memory_manager.h"
#include "kyra/core/memory/zone/memory_zone.h"
#include "kyra/core/misc/console/console.h"


// Engine state --------------------------------------------------------- //

typedef struct Engine_State {
    EngineConfig    engine_config;
    MemoryConfig    memory_config;
} EngineState;

static EngineState *state = NULL;


// Helper functions ----------------------------------------------------- //

EngineResult _engine_parse_memory_size(ConstStr size_str, ByteSize *out_size) {
    if (!size_str) return ENGINE_HELPER_ERROR_SIZE_STRING_NULL;

    // Intepret value as float64
    // Locate position after the value and save to 'end'
    Str end = NULL;
    Flt64 value = strtod(size_str, &end);

    // Skip whitespace
    while (*end == ' ') ++end;

    Char unit[3] = {0};     // Extra character for null-terminator (_B + \0)
    strncpy(unit, end, 2);  // Copy _B

    // Convert to uppercase for uniformed parsing    
    for (ByteSize index = 0; index < 2; ++index)
        if (unit[index] >= 'a' && unit[index] <= 'z') unit[index] -= 32; 

    // Convert to bytes
    if (out_size) {
        if (!strcmp(unit, "GB"))        *out_size = MEMORY_GB_TO_BYTES(value);
        else if (!strcmp(unit, "MB"))   *out_size = MEMORY_MB_TO_BYTES(value);
        else if (!strcmp(unit, "KB"))   *out_size = MEMORY_KB_TO_BYTES(value);
        else if (!strcmp(unit, "B"))    *out_size = (ByteSize)value;
    }

    return ENGINE_SUCCESS;
}

EngineResult _engine_configure(ConstStr config_filepath) {
    if (!config_filepath) return ENGINE_HELPER_ERROR_CONFIG_FILEPATH_NULL;

    FilesystemResult fs_result = 0; 

    // Open configuration file
    File config_file = {0};
    fs_result = platform_filesystem_file_open(config_filepath, FILESYSTEM_IO_MODE_READ, FILESYSTEM_FILE_MODE_BINARY, &config_file);
    if (fs_result != FILESYSTEM_SUCCESS) {
        KYRA_PRINT_ERROR("Failed to open config file: %s (Error: %s)", config_filepath, platform_filesystem_result_to_string(fs_result));
        return ENGINE_HELPER_ERROR_FAILED_TO_OPEN_CONFIG_FILE;
    }

    // Get file size
    ByteSize file_size = 0;
    fs_result = platform_filesystem_file_size(&config_file, &file_size);
    if (fs_result != FILESYSTEM_SUCCESS) {
        KYRA_PRINT_ERROR("Failed to get size of file: %s (Error: %s)", config_filepath, platform_filesystem_result_to_string(fs_result));
        return ENGINE_HELPER_ERROR_FAILED_TO_GET_FILE_SIZE;
    }

    // Allocate buffer to contain file data
    Str buffer = malloc(file_size + 1); // +1 for null-terminator
    if (!buffer) {
        // If data buffer failed to allocate, close the config file
        fs_result = platform_filesystem_file_close(&config_file);
        if (fs_result != FILESYSTEM_SUCCESS) {
            KYRA_PRINT_ERROR("Failed to close file: %s (Error: %s)", config_filepath, platform_filesystem_result_to_string(fs_result));
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
        KYRA_PRINT_ERROR("Failed to close file: %s (Error: %s)", config_filepath, platform_filesystem_result_to_string(fs_result));
        return ENGINE_HELPER_ERROR_FAILED_TO_CLOSE_CONFIG_FILE;
    }

    // Parse to JSON
    cJSON *json = cJSON_Parse(buffer);    
    if (!json) {
        KYRA_PRINT_ERROR("Failed to parse to JSON.");
        return ENGINE_HELPER_ERROR_FAILED_TO_PARSE_TO_JSON;
    }

    // Free data buffer
    free(buffer);

    // --- Info section --- //
    {
        cJSON *info = cJSON_GetObjectItemCaseSensitive(json, "info");
        if (info) {
            // Author
            cJSON *author = cJSON_GetObjectItemCaseSensitive(info, "author");
            if (cJSON_IsString(author)) state->engine_config.author = _strdup(author->valuestring);
            
            // Version
            cJSON *version = cJSON_GetObjectItemCaseSensitive(info, "version");
            if (cJSON_IsString(version)) state->engine_config.version = _strdup(version->valuestring);
        }
    }

    // --- Memory section --- //
    {
        cJSON *memory = cJSON_GetObjectItemCaseSensitive(json, "memory");
        
        // Memory zones
        cJSON *memory_zones = cJSON_GetObjectItemCaseSensitive(memory, "zones");
        if (memory_zones) {
            // Identify number of memory zones and allocate accordingly
            state->memory_config.zone_count = (ByteSize)cJSON_GetArraySize(memory_zones);
            state->memory_config.zones = malloc(sizeof(MemoryZoneConfig) * state->memory_config.zone_count);
            
            // Interate through each item inside 'memory/zones'
            // Register name and capacity
            cJSON *zone_item = NULL;
            ByteSize index = 0;
            cJSON_ArrayForEach(zone_item, memory_zones) {
                MemoryZoneConfig *zone = &state->memory_config.zones[index++];
                
                // Zone name
                zone->name = _strdup(zone_item->string);

                // Zone capacity
                ByteSize capacity = 0;
                EngineResult parse_result = _engine_parse_memory_size(zone_item->valuestring, &capacity);
                if (parse_result != ENGINE_SUCCESS) {
                    // If failed to parse capacity...
                    
                    // Notify a warning
                    // Assign zone capacity as zero
                    KYRA_PRINT_WARN(
                        "Failed to parse capacity for memory zone: %s (Error: %s). Assigning capacity as zero...", 
                        zone->name, 
                        engine_result_to_string(parse_result)
                    );
                }
                zone->capacity = capacity;

                KYRA_PRINT_INFO("Registered memory zone: %s (bytes: %llu).", zone->name, zone->capacity);
            }
        }
    }

    // Delete JSON
    cJSON_Delete(json);

    return ENGINE_SUCCESS;
}


// API functions -------------------------------------------------------- //

KYRA_ENGINE_API EngineResult engine_preconstruct(ConstStr config_filepath) {
    if (state) return ENGINE_PRECONSTRUCT_ERROR_STATE_ALREADY_INITIALISED;
    if (!config_filepath) return ENGINE_PRECONSTRUCT_ERROR_CONFIG_FILEPATH_NULL;
    
    // Console startup
    console_startup();

    KYRA_PRINT_INFO("Preconstructing engine...");
    
    // Allocate and reset engine state
    state = malloc(sizeof(EngineState));
    memset(state, 0, sizeof(EngineState));

    // Configure the engine
    EngineResult config_result = _engine_configure(config_filepath);
    if (config_result != ENGINE_SUCCESS) {
        // In case engine configuration failed...

        // Free the state and set it to NULL
        free(state);
        state = NULL;

        return config_result;
    }
    
    // Memory manager startup
    MemoryManagerResult memory_manager_result = memory_manager_startup(&state->memory_config);
    if (memory_manager_result != MEMORY_MANAGER_SUCCESS) {
        KYRA_PRINT_ERROR("Memory manager startup failed (Error: %s).", memory_manager_result_to_string(memory_manager_result));
        
        // Call for engine destruction
        return engine_destruct();
    }

    KYRA_PRINT_INFO("Engine preconstructed.");

    return ENGINE_SUCCESS;
}

KYRA_ENGINE_API EngineResult engine_construct(void) {
    if (!state) return ENGINE_CONSTRUCT_ERROR_STATE_NOT_INITIALISED;

    KYRA_PRINT_INFO("Constructing engine...");
    
    KYRA_PRINT_INFO("Engine constructed.");

    return ENGINE_SUCCESS;
}

KYRA_ENGINE_API EngineResult engine_update(Flt32 delta_time) {
    if (!state) return ENGINE_UPDATE_ERROR_STATE_NOT_INITIALISED;
    
    KYRA_PRINT_INFO("Updating engine...");

    memory_manager_report();

    return ENGINE_SUCCESS;
}

KYRA_ENGINE_API EngineResult engine_destruct(void) {
    if (!state) return ENGINE_DESTRUCT_ERROR_STATE_NOT_INITIALISED;

    KYRA_PRINT_INFO("Destructing engine...");
    
    // Memory manager shutdown
    MemoryManagerResult memory_manager_result = memory_manager_shutdown();
    if (memory_manager_result != MEMORY_MANAGER_SUCCESS)
        KYRA_PRINT_ERROR("Memory manager shutdown failed (Error: %s).", memory_manager_result_to_string(memory_manager_result));

    // Deallocate memory configuration properties
    {
        if (state->memory_config.zones) free(state->memory_config.zones);
    }

    // Deallocate engine configuration properties
    {
        if (state->engine_config.author) free(state->engine_config.author);
        if (state->engine_config.version) free(state->engine_config.version);
    }

    // Deallocate engine state and set to NULL
    free(state);
    state = NULL;

    KYRA_PRINT_INFO("Engine destructed.");

    // Console shutdown
    console_shutdown();

    return ENGINE_SUCCESS;
}

KYRA_ENGINE_API ConstStr engine_result_to_string(const EngineResult result) {
    switch (result) {
        case ENGINE_SUCCESS:                                            return "ENGINE_SUCCESS";
        
        // Helper
        case ENGINE_HELPER_ERROR_CONFIG_FILEPATH_NULL:                  return "ENGINE_HELPER_ERROR_CONFIG_FILEPATH_NULL";
        case ENGINE_HELPER_ERROR_FAILED_TO_OPEN_CONFIG_FILE:            return "ENGINE_HELPER_ERROR_FAILED_TO_OPEN_CONFIG_FILE";
        case ENGINE_HELPER_ERROR_FAILED_TO_GET_FILE_SIZE:               return "ENGINE_HELPER_ERROR_FAILED_TO_GET_FILE_SIZE";
        case ENGINE_HELPER_ERROR_FAILED_TO_CLOSE_CONFIG_FILE:           return "ENGINE_HELPER_ERROR_FAILED_TO_CLOSE_CONFIG_FILE";
        case ENGINE_HELPER_ERROR_FAILED_TO_PARSE_TO_JSON:               return "ENGINE_HELPER_ERROR_FAILED_TO_PARSE_TO_JSON";
        case ENGINE_HELPER_ERROR_SIZE_STRING_NULL:                      return "ENGINE_HELPER_ERROR_SIZE_STRING_NULL";
        
        // Pre-construct
        case ENGINE_PRECONSTRUCT_ERROR_STATE_ALREADY_INITIALISED:       return "ENGINE_PRECONSTRUCT_ERROR_STATE_ALREADY_INITIALISED";
        case ENGINE_PRECONSTRUCT_ERROR_CONFIG_FILEPATH_NULL:            return "ENGINE_PRECONSTRUCT_ERROR_CONFIG_FILEPATH_NULL";
        
        // Construct
        case ENGINE_CONSTRUCT_ERROR_STATE_NOT_INITIALISED:              return "ENGINE_CONSTRUCT_ERROR_STATE_NOT_INITIALISED";
        
        // Update
        case ENGINE_UPDATE_ERROR_STATE_NOT_INITIALISED:                 return "ENGINE_UPDATE_ERROR_STATE_NOT_INITIALISED";
        
        // Destruct 
        case ENGINE_DESTRUCT_ERROR_STATE_NOT_INITIALISED:               return "ENGINE_DESTRUCT_ERROR_STATE_NOT_INITIALISED";
    
        default:                                                        return "UNKNOWN_ENGINE_RESULT";
    }
}



