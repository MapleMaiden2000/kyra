#include "kyra/core/application/application.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cJSON.h>

#include "kyra/core/platform/filesystem/filesystem.h"
#include "kyra/core/misc/console/console.h"


// API functions -------------------------------------------------------- //

KYRA_ENGINE_API ApplicationResult application_configure(ConstStr config_filepath, ApplicationConfig *out_config) {
    if (!out_config) return APPLICATION_ERROR_REF_OUT_CONFIG_NULL;

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
        if (cJSON_IsString(sect_info_name)) out_config->name = _strdup(sect_info_name->valuestring);
    }

    KYRA_PRINT_INFO("Application name: %s", out_config->name);

    return APPLICATION_SUCCESS;
}


