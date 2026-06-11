#include "kyra/core/logger/logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cJSON.h>

#include "kyra/core/containers/map/map.h"
#include "kyra/core/containers/string/string.h"
#include "kyra/core/hal/clock/wall/wall.h"
#include "kyra/core/hash/hash.h"
#include "kyra/core/misc/console/console.h"
#include "kyra/core/platform/filesystem/filesystem.h"
#include "kyra/core/memory/zone/memory_zone.h"
#include "kyra/core/modules/command/command_module.h"


// Internal structures --------------------------------------------- //

typedef struct Logger_Handle {
    String          id;
    
    LoggerFlags     flags;
    
    File            file;
    Bool            has_file;

} Logger;

typedef struct Logger_State {
    Map             logger_map;     // <Logger>
    String          out_directory;

    // For allocations/deallocation
    ByteSize        memory_size;

} LoggerState;

static LoggerState *state = NULL;


// Helper functions ------------------------------------------------ //

static ConstStr _logger_get_verbosity_string(const LoggerVerbosity verbosity) {
    switch (verbosity) {
        case LOGGER_VERBOSITY_TRACE:    return "TRACE";
        case LOGGER_VERBOSITY_DEBUG:    return "DEBUG";
        case LOGGER_VERBOSITY_INFO:     return "INFO";
        case LOGGER_VERBOSITY_WARNING:  return "WARNING";
        case LOGGER_VERBOSITY_ERROR:    return "ERROR";
        case LOGGER_VERBOSITY_FATAL:    return "FATAL";
        
        default:                        return "UNKNOWN";
    }
}

static ConstStr _logger_verbosity_to_string(LoggerVerbosity verbosity) {
    // Convert verbosity to string
    switch (verbosity) {
        case LOGGER_VERBOSITY_TRACE:    return "TRACE";
        case LOGGER_VERBOSITY_DEBUG:    return "DEBUG";
        case LOGGER_VERBOSITY_INFO:     return "INFO";
        case LOGGER_VERBOSITY_WARNING:  return "WARN";
        case LOGGER_VERBOSITY_ERROR:    return "ERROR";
        case LOGGER_VERBOSITY_FATAL:    return "FATAL";
        
        default:                        return "LOG";
    }
}

static LoggerFlags _logger_parse_flags(ConstStr flags_str) {
    if (!flags_str) return LOGGER_FLAG_ALL;

    // Parse flags string
    if (!strcmp(flags_str, "ALL"))     return LOGGER_FLAG_ALL;
    if (!strcmp(flags_str, "NONE"))    return LOGGER_FLAG_NONE;

    // Parse flags
    LoggerFlags flags = LOGGER_FLAG_NONE;
    Char buffer[KYRA_LINE_MAX_LENGTH] = {0};
    strncpy(buffer, flags_str, sizeof(buffer));

    // Tokenise flags string
    ConstStr token = strtok(buffer, "| ");
    while (token) {
        if (!strcmp(token, "TIMESTAMP"))        flags |= LOGGER_FLAG_TIMESTAMP;
        else if (!strcmp(token, "VERBOSITY"))   flags |= LOGGER_FLAG_VERBOSITY;
        else if (!strcmp(token, "FILE_LINE"))   flags |= LOGGER_FLAG_FILE_LINE;
        else if (!strcmp(token, "FUNCTION"))    flags |= LOGGER_FLAG_FUNCTION;
        
        token = strtok(NULL, "| ");
    }

    // If no flags were set, return all flags as default 
    return flags == LOGGER_FLAG_NONE ? LOGGER_FLAG_ALL : flags;
}

static void _logger_set_verbosity_text_colour(LoggerVerbosity verbosity) {
    switch (verbosity) {
        case LOGGER_VERBOSITY_TRACE:    console_set_foreground_rgb(0, 100, 255);   break;   // Blue
        case LOGGER_VERBOSITY_DEBUG:    console_set_foreground_rgb(120, 120, 120); break;   // Gray
        case LOGGER_VERBOSITY_INFO:     console_set_foreground_rgb(0, 255, 255);   break;   // Cyan
        case LOGGER_VERBOSITY_WARNING:  console_set_foreground_rgb(255, 255, 0);   break;   // Bright yellow
        case LOGGER_VERBOSITY_ERROR:    console_set_foreground_rgb(255, 0, 0);     break;   // Bright red
        case LOGGER_VERBOSITY_FATAL:    console_set_foreground_rgb(227, 7, 113);   break;   // Pink
        default:                        console_set_foreground_rgb(255, 255, 255); break;
    }
}

static void _logger_set_verbosity_colour(LoggerVerbosity verbosity) {
    switch (verbosity) {
        case LOGGER_VERBOSITY_TRACE:
        console_set_foreground_rgb(255, 255, 255);      // White
        console_set_background_rgb(0, 100, 255);        // Blue
        break;
        case LOGGER_VERBOSITY_DEBUG:
        console_set_foreground_rgb(255, 255, 255);      // White
        console_set_background_rgb(100, 100, 100);      // Gray
        break;
        case LOGGER_VERBOSITY_INFO:
        console_set_foreground_rgb(0, 0, 0);            // Black
        console_set_background_rgb(0, 255, 255);        // Cyan
        break;
        case LOGGER_VERBOSITY_WARNING:
        console_set_foreground_rgb(0, 0, 0);            // Black
        console_set_background_rgb(255, 255, 0);        // Yellow
        break;
        case LOGGER_VERBOSITY_ERROR:
        console_set_foreground_rgb(255, 255, 255);      // White
        console_set_background_rgb(255, 0, 0);          // Red
        break;
        case LOGGER_VERBOSITY_FATAL:
        console_set_foreground_rgb(255, 255, 255);      // White
        console_set_background_rgb(227, 7, 113);        // Pink
        break;
        default:
        console_set_foreground_rgb(255, 255, 255);      // White
        console_set_background_rgb(60, 60, 60);         // Gray
        break;
    }
}


// API functions --------------------------------------------------- //

KYRA_ENGINE_API LoggerResult logger_startup(ConstStr config_filepath) {
    if (state) return LOGGER_ERROR_ALREADY_INITIALISED;
    if (!config_filepath) return LOGGER_ERROR_CONFIG_FILEPATH_NULL;
    
    // Open config file
    File config_file = {0};
    if (platform_filesystem_file_open(config_filepath, FILESYSTEM_IO_MODE_READ, FILESYSTEM_FILE_MODE_BINARY, &config_file) != FILESYSTEM_SUCCESS)
        return LOGGER_ERROR_FAILED_TO_OPEN_CONFIG_FILE;

    Str buffer = NULL;
    ByteSize buffer_memsize = 0;
        
    // Read config file
    {
        // Get file size
        ByteSize size = 0;
        if (platform_filesystem_file_size(&config_file, &size) != FILESYSTEM_SUCCESS)
            return LOGGER_ERROR_FAILED_TO_GET_CONFIG_FILE_SIZE;

        // Allocate raw buffer
        if (memory_zone_allocate("loggers", size, (VoidPtr *)&buffer, &buffer_memsize) != MEMORY_ZONE_SUCCESS)
            return LOGGER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_CONFIG_RAW_BUFFER;

        // Read entire file data
        ByteSize read_bytes = 0;
        if (platform_filesystem_read_all(&config_file, &read_bytes, &buffer) != FILESYSTEM_SUCCESS)
            return LOGGER_ERROR_FAILED_TO_READ_CONFIG_FILE;
        
        // Null-terminate
        buffer[read_bytes] = '\0';

        // Close file
        platform_filesystem_file_close(&config_file);
    }

    cJSON *json = NULL;

    // Parse config file to JSON object
    {
        json = cJSON_Parse(buffer);
    
        // Deallocate raw buffer 
        if (memory_zone_deallocate("loggers", (VoidPtr)buffer, buffer_memsize) != MEMORY_ZONE_SUCCESS)
            return LOGGER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_CONFIG_RAW_BUFFER;
    
        if (!json) return LOGGER_ERROR_FAILED_TO_PARSE_CONFIG_BUFFER_TO_JSON;
    }

    // Locate log system section inside config json
    cJSON *log_system = cJSON_GetObjectItemCaseSensitive(json, "log_system");
    if (!log_system) {
        // Failed to locate...

        // Delete config JSON object
        cJSON_Delete(json);
        
        return LOGGER_ERROR_FAILED_TO_LOCATE_LOG_SYSTEM_SECTION_IN_CONFIG_JSON;
    }

    // Allocate logger state
    ByteSize mem_size = 0;
    if (memory_zone_allocate("loggers", sizeof(LoggerState), (VoidPtr *)&state, &mem_size) != MEMORY_ZONE_SUCCESS)
        return LOGGER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE;

    // Construct logger map
    if (container_map_construct(sizeof(Logger), &state->logger_map) != CONTAINER_SUCCESS)
        return LOGGER_ERROR_FAILED_TO_CONSTRUCT_LOGGER_MAP;

    // Initialise output directory string
    {
        cJSON *out_dir = cJSON_GetObjectItemCaseSensitive(log_system, "out_directory");
        ConstStr out_path = out_dir ? out_dir->valuestring : "logs";
        
        if (container_string_construct(out_path, &state->out_directory) != CONTAINER_SUCCESS)
            return LOGGER_ERROR_FAILED_TO_CONSTRUCT_OUT_DIRECTORY_STRING;
        
        // Check if output directory exists
        Bool exists = false;
        platform_filesystem_directory_exists(out_path, &exists);
        if (!exists) {
            // Doesn't exist...

            // Create output directory
            platform_filesystem_create_directory(out_path);
        }
    }

    // Register loggers 
    {
        cJSON *loggers = cJSON_GetObjectItemCaseSensitive(log_system, "loggers");

        if (cJSON_IsArray(loggers)) {
            cJSON *logger_item = NULL;

            cJSON_ArrayForEach(logger_item, loggers) {
                // For each logger item in 'loggers' section...

                // Get info
                cJSON *id = cJSON_GetObjectItemCaseSensitive(logger_item, "id");
                cJSON *file = cJSON_GetObjectItemCaseSensitive(logger_item, "file");
                cJSON *flags = cJSON_GetObjectItemCaseSensitive(logger_item, "flags");

                // Register if 'id' is valid
                if (cJSON_IsString(id)) {
                    logger_register(
                        id->valuestring,
                        file ? file->valuestring : NULL,
                        _logger_parse_flags(flags ? flags->valuestring : NULL)
                    );
                }
            }
        }
    }

    // Assign memory size
    state->memory_size = mem_size;

    // Delete config JSON object
    cJSON_Delete(json);

    return LOGGER_SUCCESS;
}

KYRA_ENGINE_API LoggerResult logger_shutdown(void) {
    if (!state) return LOGGER_ERROR_NOT_INITIALISED;
    
    // Destroy all logger handles
    ByteSize capacity = container_map_capacity(state->logger_map);
    for (ByteSize index = 0; index < capacity; ++index) {
        String key;
        Logger handle;

        if (container_map_at_index(state->logger_map, index, &key, (VoidPtr)&handle) == CONTAINER_SUCCESS) {
            // For every found logger handle...

            // Destroy 'id' string
            if (container_string_destruct(&handle.id) != CONTAINER_SUCCESS)
                return LOGGER_ERROR_FAILED_TO_DESTRUCT_ID_STRING_FOR_LOGGER;

            // Close output log file
            if (handle.has_file) {
                if (platform_filesystem_file_close(&handle.file) != FILESYSTEM_SUCCESS)
                    return LOGGER_ERROR_FAILED_TO_CLOSE_OUTPUT_LOG_FILE_FOR_LOGGER;
            }
        }
    }

    // Clean up logger state
    {
        // Destroy logger map
        if (container_map_destruct(&state->logger_map) != CONTAINER_SUCCESS)
            return LOGGER_ERROR_FAILED_TO_DESTRUCT_LOGGER_MAP;
        
        // Destroy 'id' string 
        if (container_string_destruct(&state->out_directory) != CONTAINER_SUCCESS)
            return LOGGER_ERROR_FAILED_TO_DESTRUCT_OUT_DIRECTORY_STRING;
    }

    // Deallocate state
    if (memory_zone_deallocate("loggers", (VoidPtr)state, state->memory_size) != MEMORY_ZONE_SUCCESS)
        return LOGGER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE;

    // Set to NULL
    state = NULL; 

    return LOGGER_SUCCESS;
}

KYRA_ENGINE_API LoggerResult logger_register(ConstStr id, ConstStr filename, const LoggerFlags flags) {
    if (!state) return LOGGER_ERROR_NOT_INITIALISED;
    
    Logger *reg_handle = NULL;
    if (container_map_search(state->logger_map, id, (VoidPtr)reg_handle) == CONTAINER_SUCCESS) {
        // Found logger with matching 'id'...

        // Update handle instead
        if (reg_handle) return logger_update(id, filename, flags);
    }

    // Create logger handle
    Logger handle = {0};
    {
        // Contruct 'id' string
        if (container_string_construct(id, &handle.id) != CONTAINER_SUCCESS)
            return LOGGER_ERROR_FAILED_TO_CONSTRUCT_ID_STRING_FOR_LOGGER;
        
        handle.flags = flags; // Set flags
        handle.has_file = false; // No file by default
    }

    // Open file, if provided
    if (filename) {
        // Construct full path
        Char full_path[KYRA_LINE_MAX_LENGTH] = {0};
        sprintf(full_path, "%s/%s", container_string_cstr(state->out_directory), filename);

        // Open file
        if (platform_filesystem_file_open(full_path, FILESYSTEM_IO_MODE_WRITE, FILESYSTEM_FILE_MODE_TEXT, &handle.file) == FILESYSTEM_SUCCESS)
            handle.has_file = true;
    }

    // Register handle to map
    if (container_map_insert(&state->logger_map, id, (VoidPtr)&handle) != CONTAINER_SUCCESS) {
        // Failed to do so...

        // Destroy 'id' string
        if (container_string_destruct(&handle.id) != CONTAINER_SUCCESS)
            return LOGGER_ERROR_FAILED_TO_DESTRUCT_ID_STRING_FOR_LOGGER;

        // Close output log file
        if (handle.has_file) {
            if (platform_filesystem_file_close(&handle.file) != FILESYSTEM_SUCCESS)
                return LOGGER_ERROR_FAILED_TO_CLOSE_OUTPUT_LOG_FILE_FOR_LOGGER;
        }

        return LOGGER_ERROR_FAILED_TO_REGISTER_LOGGER;
    }

    return LOGGER_SUCCESS;
}

KYRA_ENGINE_API LoggerResult logger_unregister(ConstStr id) {
    if (!state) return LOGGER_ERROR_NOT_INITIALISED;

    // Search for logger handle
    Logger handle;
    if (container_map_search(state->logger_map, id, (VoidPtr)&handle) == CONTAINER_SUCCESS) {
        // Found logger with matching 'id'...

        // Destroy 'id' string
        if (container_string_destruct(&handle.id) != CONTAINER_SUCCESS)
            return LOGGER_ERROR_FAILED_TO_DESTRUCT_ID_STRING_FOR_LOGGER;

        // Close output log file
        if (handle.has_file) {
            if (platform_filesystem_file_close(&handle.file) != FILESYSTEM_SUCCESS)
                return LOGGER_ERROR_FAILED_TO_CLOSE_OUTPUT_LOG_FILE_FOR_LOGGER;
        }

        // Un-register handle from map
        if (container_map_remove(&state->logger_map, id) != CONTAINER_SUCCESS)
            return LOGGER_ERROR_FAILED_TO_UNREGISTER_LOGGER;

        return LOGGER_SUCCESS;
    }

    return CONTAINER_MAP_ERROR_FAILED_TO_LOCATE_SLOT_FOR_KEY;
}

KYRA_ENGINE_API LoggerResult logger_update(ConstStr id, ConstStr new_filename, const LoggerFlags new_flags) {
    if (!state) return LOGGER_ERROR_NOT_INITIALISED;

    // Search for logger handle
    Logger *handle = NULL;
    if (container_map_search(state->logger_map, id, (VoidPtr)handle) == CONTAINER_SUCCESS) {
        // Found logger with matching 'id'...

        // Update output log file
        if (new_filename) {
            // Close old output log file
            if (handle->has_file) {
                if (platform_filesystem_file_close(&handle->file) != FILESYSTEM_SUCCESS)
                    return LOGGER_ERROR_FAILED_TO_CLOSE_OUTPUT_LOG_FILE_FOR_LOGGER;

                handle->has_file = false;
            }

            // Open new output log file
            {
                // Construct full path
                Char full_path[KYRA_LINE_MAX_LENGTH] = {0};
                sprintf(full_path, "%s/%s", container_string_cstr(state->out_directory), new_filename);

                // Open file
                if (platform_filesystem_file_open(full_path, FILESYSTEM_IO_MODE_WRITE, FILESYSTEM_FILE_MODE_TEXT, &handle->file) == FILESYSTEM_SUCCESS)
                    handle->has_file = true;
            }
        }

        // Update log flags
        handle->flags = new_flags;

        return LOGGER_SUCCESS;
    }

    return LOGGER_ERROR_FAILED_TO_UPDATE_LOGGER;
}

KYRA_ENGINE_API LoggerResult logger_print(
    ConstStr            id,

    LoggerVerbosity     verbosity,

    ConstStr            at_file,
    UInt32              at_line,
    ConstStr            at_function,

    ConstStr            format,
    ...
) {
    if (!state) return LOGGER_ERROR_NOT_INITIALISED;

    // Search for logger handle
    Logger handle;
    if (container_map_search(state->logger_map, id, &handle) != CONTAINER_SUCCESS)
        return LOGGER_ERROR_FAILED_TO_GET_LOGGER;


    // ----- Preparation stage ----- // 
    
    // Logger ID
    Char logger_id[64] = {0};
    strncpy(logger_id, id, sizeof(logger_id) - 1);
    
    // Verbosity string
    ConstStr verbosity_str = _logger_get_verbosity_string(verbosity);
    
    // Timestamp 
    Char log_time[64] = {0};
    WallClock now;
    clock_wall_now(&now);
    strftime(log_time, sizeof(log_time), "%F %T", &now.time_info);
    
    // Formatted message
    Char formatted_message[KYRA_LOG_MESSAGE_MAX_LENGTH] = {0};
    VaList args;
    va_start(args, format);
    vsnprintf(formatted_message, sizeof(formatted_message), format, args);
    va_end(args);
    fflush(stdout);

    // File
    Char file_name[64] = {0};
    platform_filesystem_extract_filename(at_file, sizeof(file_name), (Str)file_name);
    
    // Function
    Char function_name[64] = {0};
    strncpy(function_name, at_function, sizeof(function_name) - 1);
    
    // Padding
    Int32 padding = 4;
    
    // Widths
    Char line_str[16] = {0};
    sprintf(line_str, "%d", at_line);
    
    Int32 logger_id_width = container_string_size(handle.id) + padding;
    Int32 timestamp_width = strlen(log_time) + padding;
    Int32 verbosity_width = strlen(verbosity_str) + padding;
    Int32 file_line_width = (KYRA_BITFLAG_IF_SET(handle.flags, LOGGER_FLAG_FILE_LINE) ? (strlen(file_name) + strlen(line_str) + 1) : 0) + padding;
    Int32 function_width = strlen(function_name) + padding;
    
    // Total width for header
    Int32 total_width = logger_id_width + timestamp_width + verbosity_width + file_line_width + function_width;
    

    // ----- Print stage ----- //
    
    // Momentarily in-activate command line
    command_module_inactivate_command_line();

    // Print header
    {
        console_reset();
        
        // Logger ID
        console_set_foreground_rgb(255, 220, 230);      // Blush
        console_set_background_rgb(130, 0, 60);         // Black rose
        console_write("  %s  ", logger_id);
        console_reset();
        
        // Timestamp
        if (KYRA_BITFLAG_IF_SET(handle.flags, LOGGER_FLAG_TIMESTAMP)) {
            console_set_foreground_rgb(230, 200, 255);  // Lavender
            console_set_background_rgb(50, 20, 80);     // Night purple
            console_write("  %s  ", log_time);
            console_reset();
        }
        
        // File and Line
        if (KYRA_BITFLAG_IF_SET(handle.flags, LOGGER_FLAG_FILE_LINE)) {
            console_set_foreground_rgb(255, 200, 255);  // Soft lilac
            console_set_background_rgb(100, 30, 120);   // Deep orchid
            console_write("  %s:%d  ", file_name, at_line);
            console_reset();
        }
        
        // Function
        if (KYRA_BITFLAG_IF_SET(handle.flags, LOGGER_FLAG_FUNCTION)) {
            console_set_foreground_rgb(255, 255, 255);  // White
            console_set_background_rgb(120, 40, 100);   // Dark mauve
            console_write("  %s  ", function_name);
            console_reset();
        }
        
        // Verbosity
        _logger_set_verbosity_colour(verbosity);
        console_write("  %s  ", verbosity_str);
        console_reset();
        
        console_write_line("");
    }

    // Print message, which dynamically wraps
    {
        Int32 message_length = strlen(formatted_message);
        Int32 line_length = 0;
        Int32 line_start = 0;
        
        while (line_start < message_length) {
            Int32 line_end = line_start + total_width - padding;
            
            // If line end is greater than message length, set it to message length
            if (line_end > message_length) line_end = message_length;
            
            // If line end is not a space, search backwards for a space
            if ((line_end < message_length) && (formatted_message[line_end] != ' ')) {
                Int32 search_pos = line_end;
                while ((search_pos > line_start) && (formatted_message[search_pos] != ' ')) --search_pos;
                
                if (search_pos > line_start) line_end = search_pos;
            }
            
            // Force wrap around if there is no space
            if (line_end == line_start) line_end = line_start + (total_width - padding);
            
            // Print message
            {
                _logger_set_verbosity_text_colour(verbosity);
                console_write_line("  %-*.*s  ", total_width - padding, line_end - line_start, formatted_message + line_start);
                console_reset();
            }
            
            // Move to next line
            line_start = line_end;
            while ((line_start < message_length) && (formatted_message[line_start] == ' ')) ++line_start;
        }
    }

    // Build output to file
    Char out_to_file[KYRA_LOG_MESSAGE_MAX_LENGTH] = {0};
    {
        // Pointer to current position in out_to_file
        Char *ptr = out_to_file;
        
        // Remaining space in out_to_file
        ByteSize remaining = sizeof(out_to_file);
        
        // Number of bytes written
        Int32 written = 0;
        
        // Timestamp
        if (KYRA_BITFLAG_IF_SET(handle.flags, LOGGER_FLAG_TIMESTAMP)) {
            written = snprintf(ptr, remaining, "[%s] ", log_time);
            
            if (written > 0) { 
                ptr += (Int32)written; 
                remaining -= (ByteSize)written; 
            }
        }
        
        // Logger ID
        {
            written = snprintf(ptr, remaining, "[%s] ", logger_id);
            if (written > 0) { 
                ptr += (Int32)written; 
                remaining -= (ByteSize)written; 
            }
        }
        
        // File and Line
        if (KYRA_BITFLAG_IF_SET(handle.flags, LOGGER_FLAG_FILE_LINE)) {
            written = snprintf(ptr, remaining, "[%s:%d] ", file_name, at_line);
            if (written > 0) { 
                ptr += (Int32)written; 
                remaining -= (ByteSize)written; 
            }
        }
        
        // Function
        if (KYRA_BITFLAG_IF_SET(handle.flags, LOGGER_FLAG_FUNCTION)) {
            written = snprintf(ptr, remaining, "[%s] ", function_name);
            if (written > 0) { 
                ptr += (Int32)written; 
                remaining -= (ByteSize)written; 
            }
        }
        
        // Verbosity
        if (KYRA_BITFLAG_IF_SET(handle.flags, LOGGER_FLAG_VERBOSITY)) {
            written = snprintf(ptr, remaining, "[%s] ", verbosity_str);
            if (written > 0) { 
                ptr += (Int32)written; 
                remaining -= (ByteSize)written; 
            }
        }
        
        // Formatted message
        snprintf(ptr, remaining, ": %s\n", formatted_message);
        
    }
    
    // Output to file
    if (handle.has_file) platform_filesystem_write_data(&handle.file, strlen(out_to_file), out_to_file);
    
    // Re-activate command line
    command_module_activate_command_line();

    return LOGGER_SUCCESS;
}

KYRA_ENGINE_API ConstStr logger_result_to_string(const LoggerResult result) {
    switch (result) {
        case LOGGER_SUCCESS:                                                            return "LOGGER_SUCCESS";

        case LOGGER_ERROR_ALREADY_INITIALISED:                                          return "LOGGER_ERROR_ALREADY_INITIALISED";
        case LOGGER_ERROR_NOT_INITIALISED:                                              return "LOGGER_ERROR_NOT_INITIALISED";
        case LOGGER_ERROR_CONFIG_FILEPATH_NULL:                                         return "LOGGER_ERROR_CONFIG_FILEPATH_NULL";
        case LOGGER_ERROR_FAILED_TO_OPEN_CONFIG_FILE:                                   return "LOGGER_ERROR_FAILED_TO_OPEN_CONFIG_FILE";
        case LOGGER_ERROR_FAILED_TO_GET_CONFIG_FILE_SIZE:                               return "LOGGER_ERROR_FAILED_TO_GET_CONFIG_FILE_SIZE";
        case LOGGER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_CONFIG_RAW_BUFFER:              return "LOGGER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_CONFIG_RAW_BUFFER";
        case LOGGER_ERROR_FAILED_TO_READ_CONFIG_FILE:                                   return "LOGGER_ERROR_FAILED_TO_READ_CONFIG_FILE";
        case LOGGER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_CONFIG_RAW_BUFFER:             return "LOGGER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_CONFIG_RAW_BUFFER";
        case LOGGER_ERROR_FAILED_TO_PARSE_CONFIG_BUFFER_TO_JSON:                        return "LOGGER_ERROR_FAILED_TO_PARSE_CONFIG_BUFFER_TO_JSON";
        case LOGGER_ERROR_FAILED_TO_LOCATE_LOG_SYSTEM_SECTION_IN_CONFIG_JSON:           return "LOGGER_ERROR_FAILED_TO_LOCATE_LOG_SYSTEM_SECTION_IN_CONFIG_JSON";
        case LOGGER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE:                          return "LOGGER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE";
        case LOGGER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE:                         return "LOGGER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE";
        case LOGGER_ERROR_FAILED_TO_CONSTRUCT_LOGGER_MAP:                               return "LOGGER_ERROR_FAILED_TO_CONSTRUCT_LOGGER_MAP";
        case LOGGER_ERROR_FAILED_TO_DESTRUCT_LOGGER_MAP:                                return "LOGGER_ERROR_FAILED_TO_DESTRUCT_LOGGER_MAP";
        case LOGGER_ERROR_FAILED_TO_CONSTRUCT_OUT_DIRECTORY_STRING:                     return "LOGGER_ERROR_FAILED_TO_CONSTRUCT_OUT_DIRECTORY_STRING";
        case LOGGER_ERROR_FAILED_TO_DESTRUCT_OUT_DIRECTORY_STRING:                      return "LOGGER_ERROR_FAILED_TO_DESTRUCT_OUT_DIRECTORY_STRING";
        case LOGGER_ERROR_FAILED_TO_CONSTRUCT_ID_STRING_FOR_LOGGER:                     return "LOGGER_ERROR_FAILED_TO_CONSTRUCT_ID_STRING_FOR_LOGGER";
        case LOGGER_ERROR_FAILED_TO_DESTRUCT_ID_STRING_FOR_LOGGER:                      return "LOGGER_ERROR_FAILED_TO_DESTRUCT_ID_STRING_FOR_LOGGER";
        case LOGGER_ERROR_FAILED_TO_CLOSE_OUTPUT_LOG_FILE_FOR_LOGGER:                   return "LOGGER_ERROR_FAILED_TO_CLOSE_OUTPUT_LOG_FILE_FOR_LOGGER";
        case LOGGER_ERROR_FAILED_TO_REGISTER_LOGGER:                                    return "LOGGER_ERROR_FAILED_TO_REGISTER_LOGGER";
        case LOGGER_ERROR_FAILED_TO_UNREGISTER_LOGGER:                                  return "LOGGER_ERROR_FAILED_TO_UNREGISTER_LOGGER";
        case LOGGER_ERROR_FAILED_TO_UPDATE_LOGGER:                                      return "LOGGER_ERROR_FAILED_TO_UPDATE_LOGGER";
        case LOGGER_ERROR_FAILED_TO_GET_LOGGER:                                         return "LOGGER_ERROR_FAILED_TO_GET_LOGGER";
        
        default:                                                                        return "UNKNOWN_LOGGER_RESULT";
    }
}


