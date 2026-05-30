#include "kyra/core/application/application.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cJSON.h>

#include "kyra/core/platform/filesystem/filesystem.h"
#include "kyra/core/misc/console/console.h"
#include "kyra/core/containers/string/string.h"


// API functions -------------------------------------------------------- //

KYRA_ENGINE_API ApplicationResult application_configure(ConstStr config_filepath, Application *out_app) {
    if (!out_app) return APPLICATION_ERROR_REF_OUT_APPLICATION_NULL;

    FilesystemResult fs_result = 0; 

    // Open configuration file
    File config_file = {0};
    fs_result = platform_filesystem_file_open(config_filepath, FILESYSTEM_IO_MODE_READ, FILESYSTEM_FILE_MODE_BINARY, &config_file);
    if (fs_result != FILESYSTEM_SUCCESS) {
        KYRA_PRINT_ERROR("Failed to open config file: %s (Error: %s)", config_filepath, platform_filesystem_result_to_string(fs_result));
        
        return APPLICATION_ERROR_FAILED_TO_OPEN_CONFIG_FILE;
    }

    // Get file size
    ByteSize file_size = 0;
    fs_result = platform_filesystem_file_size(&config_file, &file_size);
    if (fs_result != FILESYSTEM_SUCCESS) {
        KYRA_PRINT_ERROR("Failed to get size of file: %s (Error: %s)", config_filepath, platform_filesystem_result_to_string(fs_result));
        
        return APPLICATION_ERROR_FAILED_TO_GET_FILE_SIZE;
    }

    // Allocate buffer to contain file data
    Str buffer = malloc(file_size + 1); // +1 for null-terminator
    if (!buffer) {
        // If data buffer failed to allocate, close the config file
        fs_result = platform_filesystem_file_close(&config_file);
        if (fs_result != FILESYSTEM_SUCCESS) {
            KYRA_PRINT_ERROR("Failed to close file: %s (Error: %s)", config_filepath, platform_filesystem_result_to_string(fs_result));
            
            return APPLICATION_ERROR_FAILED_TO_CLOSE_CONFIG_FILE;
        }
    }

    // Read entire file
    ByteSize bytes_read = 0;
    fs_result = platform_filesystem_read_all(&config_file, &bytes_read, &buffer);
    if (fs_result != FILESYSTEM_SUCCESS) {
        free(buffer);
        KYRA_PRINT_ERROR("Failed to read file: %s (Error: %s)", config_filepath, platform_filesystem_result_to_string(fs_result));
        
        return APPLICATION_ERROR_FAILED_TO_CLOSE_CONFIG_FILE;
    }

    // Null-terminate
    buffer[bytes_read] = '\0';

    // Close the config file
    fs_result = platform_filesystem_file_close(&config_file);
    if (fs_result != FILESYSTEM_SUCCESS) {
        free(buffer);
        KYRA_PRINT_ERROR("Failed to close file: %s (Error: %s)", config_filepath, platform_filesystem_result_to_string(fs_result));
        
        return APPLICATION_ERROR_FAILED_TO_CLOSE_CONFIG_FILE;
    }

    // Parse to JSON
    cJSON *json = cJSON_Parse(buffer);    
    if (!json) {
        KYRA_PRINT_ERROR("Failed to parse to JSON.");
        return APPLICATION_ERROR_FAILED_TO_PARSE_TO_JSON;
    }

    // Free data buffer
    free(buffer);

    // --- Info section --- //
    
    cJSON *sect_info = cJSON_GetObjectItemCaseSensitive(json, "info");
    if (sect_info) {
        // Application name
        cJSON *sect_info_name = cJSON_GetObjectItemCaseSensitive(sect_info, "name");
        if (cJSON_IsString(sect_info_name)) {
            if (container_string_construct(sect_info_name->valuestring, &out_app->name) != CONTAINER_SUCCESS)
                return APPLICATION_ERROR_FAILED_TO_CONSTRUCT_APPLICATION_NAME_STRING;
        }
    }

    return APPLICATION_SUCCESS;
}

KYRA_ENGINE_API ApplicationResult application_request_shutdown(Application *app) {
    // Request application shutdown
    if (!app) return APPLICATION_ERROR_REF_APPLICATION_NULL;

    app->is_running = false;

    return APPLICATION_SUCCESS;
}

KYRA_ENGINE_API ConstStr application_result_to_string(const ApplicationResult result) {
    switch (result) {
        case APPLICATION_SUCCESS:                                               return "APPLICATION_SUCCESS";

        case APPLICATION_ERROR_REF_OUT_APPLICATION_NULL:                        return "APPLICATION_ERROR_REF_OUT_APPLICATION_NULL"; 
        case APPLICATION_ERROR_REF_APPLICATION_NULL:                            return "APPLICATION_ERROR_REF_APPLICATION_NULL"; 
        case APPLICATION_ERROR_FAILED_TO_OPEN_CONFIG_FILE:                      return "APPLICATION_ERROR_FAILED_TO_OPEN_CONFIG_FILE"; 
        case APPLICATION_ERROR_FAILED_TO_GET_FILE_SIZE:                         return "APPLICATION_ERROR_FAILED_TO_GET_FILE_SIZE"; 
        case APPLICATION_ERROR_FAILED_TO_CLOSE_CONFIG_FILE:                     return "APPLICATION_ERROR_FAILED_TO_CLOSE_CONFIG_FILE"; 
        case APPLICATION_ERROR_FAILED_TO_PARSE_TO_JSON:                         return "APPLICATION_ERROR_FAILED_TO_PARSE_TO_JSON"; 
        case APPLICATION_ERROR_FAILED_TO_CONSTRUCT_APPLICATION_NAME_STRING:     return "APPLICATION_ERROR_FAILED_TO_CONSTRUCT_APPLICATION_NAME_STRING"; 
    
        default:                                                                return "UNKNOWN_APPLICATION_RESULT";
    }
}


