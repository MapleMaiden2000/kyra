#include "kyra/core/modules/command/command_module.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <cJSON.h>

#if KYRA_PLATFORM_WINDOWS
    #include <windows.h>
    #include <conio.h>
#endif    

#include "kyra/core/containers/map/map.h"
#include "kyra/core/containers/string/string.h"
#include "kyra/core/hash/hash.h"
#include "kyra/core/misc/console/console.h"
#include "kyra/core/memory/zone/memory_zone.h"
#include "kyra/core/delegates/unicast/unicast.h"
#include "kyra/core/containers/string/string.h"
#include "kyra/core/platform/filesystem/filesystem.h"


// Internal structure ------------------------------------------ //

typedef struct Command_Module_State {
    Map         registry;      // <root_name: ConstStr, Map<action_name: ConstStr, action: CommandAction>>

    Str         input_buffer;
    ByteSize    input_buffer_size;
    ByteSize    input_buffer_memory_size;
    
    Str         prompt_header;
    ByteSize    prompt_header_size;
    ByteSize    prompt_header_memory_size;

    UInt32      cursor_position;

    Bool        command_line_active;
    
    Str         token_buffer;
    ByteSize    token_buffer_memory_size;

    ConstStr   *token_list;
    ByteSize    token_list_memory_size;

    Str         history;
    ByteSize    history_size;
    Int32       history_view_index;
    ByteSize    history_memory_size;    

    Str         temp_buffer;
    ByteSize    temp_buffer_size;
    ByteSize    temp_buffer_memory_size;

    Int32       history_limit;
    Int32       undo_limit;
    Bool        echo_prompt;
    ByteSize    max_arguments;
    ByteSize    token_max_length;

    // For allocations/deallocations
    ByteSize    memory_size;

} CommandModuleState;

static CommandModuleState *state = NULL;


// Helper functions -------------------------------------------- //

static UInt32 _command_module_tokenise(ConstStr input, ConstStr *out_tokens, const UInt32 max_tokens) {
    UInt32 count = 0;
    ConstStr itr = input;
    
    // Iterate until we run out of input/tokens
    while ((*itr != '\0') && (count < max_tokens)) {
        // Skip whitespace
        {
            while (*itr && (*itr == ' ' || *itr == '\t')) ++itr;
            if (*itr == '\0') break;
        }

        Str token_buf = (Str)((UIntPtr)(state->token_buffer) + (state->token_max_length * count));

        ByteSize itr_token = 0;

        // Handle quoted strings
        if (*itr == '\"') {
            // Skip opening quote
            ++itr;

            // Copy entire content
            // Until we reach closing quote or end of string
            while ((*itr != '\0') && (*itr != '\"') && (itr_token < state->token_max_length - 1))
                token_buf[itr_token++] = *itr++;

            // Skip closing quote
            if (*itr == '\"') ++itr;
        }

        // Otherwise...
        // Standard token
        else {
            // Copy until whitespace or end of string
            while ((*itr != '\0') && (*itr != ' ') && (*itr != '\t') && (itr_token < state->token_max_length - 1))
                token_buf[itr_token++] = *itr++;
        }

        // Null-terminate token buffer
        token_buf[itr_token] = '\0';
    
        // Store token to ref
        out_tokens[count] = token_buf;

        // Increment token count to advance
        ++count;
    }

    return count;
}


// API functions ----------------------------------------------- //

KYRA_ENGINE_API CommandModuleResult command_module_startup(ConstStr config_filepath) {
    if (state) return COMMAND_MODULE_ERROR_ALREADY_INITIALISED;
    if (!config_filepath) return COMMAND_MODULE_ERROR_CONFIG_FILEPATH_NULL;

    // Allocate for state
    ByteSize mem_size = 0;
    if (memory_zone_allocate("command", sizeof(CommandModuleState), (VoidPtr *)&state, &mem_size) != MEMORY_ZONE_SUCCESS)
        return COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE;

    // Zero out state
    memset(state, 0, sizeof(CommandModuleState));

    // Configure
    {
        // Open config file
        File config_file = {0};
        if (platform_filesystem_file_open(config_filepath, FILESYSTEM_IO_MODE_READ, FILESYSTEM_FILE_MODE_BINARY, &config_file) != FILESYSTEM_SUCCESS)
            return COMMAND_MODULE_ERROR_FAILED_TO_OPEN_CONFIG_FILE;

        // Get file size
        ByteSize size = 0;
        platform_filesystem_file_size(&config_file, &size);

        Str buffer = NULL;
        ByteSize buffer_memsize = 0;

        // Allocate raw buffer
        if (memory_zone_allocate("command", size, (VoidPtr *)&buffer, &buffer_memsize) != MEMORY_ZONE_SUCCESS)
            return COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_CONFIG_RAW_BUFFER;

        // Read entire file data
        ByteSize read_bytes = 0;
        if (platform_filesystem_read_all(&config_file, &read_bytes, &buffer) != FILESYSTEM_SUCCESS)
            return COMMAND_MODULE_ERROR_FAILED_TO_READ_CONFIG_FILE;
        
        // Null-terminate
        buffer[read_bytes] = '\0';

        // Close file
        if (platform_filesystem_file_close(&config_file) != FILESYSTEM_SUCCESS)
            return COMMAND_MODULE_ERROR_FAILED_TO_CLOSE_CONFIG_FILE;

        // Parse to JSON
        cJSON *json = cJSON_Parse(buffer);
        if (!json) return COMMAND_MODULE_ERROR_FAILED_TO_PARSE_CONFIG_BUFFER_TO_JSON;

        // Deallocate raw buffer 
        if (memory_zone_deallocate("command", (VoidPtr)buffer, buffer_memsize) != MEMORY_ZONE_SUCCESS)
            return COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_CONFIG_RAW_BUFFER;
    
        cJSON *command = cJSON_GetObjectItemCaseSensitive(json, "command");
        if (command) {
            // History limit
            cJSON *history_limit = cJSON_GetObjectItemCaseSensitive(command, "history_limit");
            if (history_limit && cJSON_IsNumber(history_limit)) state->history_limit = history_limit->valueint;
            
            // Undo limit
            cJSON *undo_limit = cJSON_GetObjectItemCaseSensitive(command, "undo_limit");
            if (undo_limit && cJSON_IsNumber(undo_limit)) state->undo_limit = undo_limit->valueint;
        
            // Echo prompt
            cJSON *echo_prompt = cJSON_GetObjectItemCaseSensitive(command, "echo_prompt");
            if (echo_prompt) state->echo_prompt = cJSON_IsTrue(echo_prompt);

            // Max arguments
            cJSON *max_args = cJSON_GetObjectItemCaseSensitive(command, "max_arguments");
            if (max_args && cJSON_IsNumber(max_args)) state->max_arguments = max_args->valueint;
            
            // Token length
            cJSON *token_len = cJSON_GetObjectItemCaseSensitive(command, "token_max_length");
            if (token_len && cJSON_IsNumber(token_len)) state->token_max_length = token_len->valueint;
        }
        
        // Delete config JSON object
        cJSON_Delete(json);
    }

    // Construct registry
    if (container_map_construct(sizeof(Map), &state->registry) != CONTAINER_SUCCESS)
        return COMMAND_MODULE_ERROR_FAILED_TO_CONSTRUCT_REGISTRY;

    // Input buffer
    {
        // Allocate memory
        if (memory_zone_allocate("command", KYRA_LINE_MAX_LENGTH, (VoidPtr *)&state->input_buffer, &state->input_buffer_memory_size) != MEMORY_ZONE_SUCCESS)
            return COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_INPUT_BUFFER;

        // Zero out
        memset(state->input_buffer, 0, state->input_buffer_memory_size);
    }

    // Prompt header
    {
        // Allocate memory
        if (memory_zone_allocate("command", KYRA_SHORT_LINE_MAX_LENGTH, (VoidPtr *)&state->prompt_header, &state->prompt_header_memory_size) != MEMORY_ZONE_SUCCESS)
            return COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_PROMPT_HEADER;

        // Zero out
        memset(state->prompt_header, 0, state->prompt_header_memory_size);
    }

    // Token buffer
    {
        // Allocate memory
        if (memory_zone_allocate("command", state->max_arguments * state->token_max_length, (VoidPtr *)&state->token_buffer, &state->token_buffer_memory_size) != MEMORY_ZONE_SUCCESS)
            return COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_TOKEN_BUFFER;

        // Zero out
        memset(state->token_buffer, 0, state->token_buffer_memory_size);
    }

    // Token list
    {
        // Allocate memory
        if (memory_zone_allocate("command", state->max_arguments * sizeof(ConstStr), (VoidPtr *)&state->token_list, &state->token_list_memory_size) != MEMORY_ZONE_SUCCESS)
            return COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_TOKEN_LIST;

        // Zero out
        memset(state->token_buffer, 0, state->token_list_memory_size);
    }

    // History
    {
        // Allocate memory
        if (memory_zone_allocate("command", state->history_limit * KYRA_LINE_MAX_LENGTH, (VoidPtr *)&state->history, &state->history_memory_size) != MEMORY_ZONE_SUCCESS)
            return COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_HISTORY;

        // Zero out
        memset(state->history, 0, state->history_memory_size);
    }

    // Temp buffer
    {
        // Allocate memory
        if (memory_zone_allocate("command", KYRA_LINE_MAX_LENGTH, (VoidPtr *)&state->temp_buffer, &state->temp_buffer_memory_size) != MEMORY_ZONE_SUCCESS)
            return COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_TEMP_BUFFER;

        // Zero out
        memset(state->temp_buffer, 0, state->temp_buffer_memory_size);
    }

    // Assign state memory size
    state->memory_size = mem_size;

    return COMMAND_MODULE_SUCCESS;
}

KYRA_ENGINE_API CommandModuleResult command_module_shutdown(void) {
    if (!state) return COMMAND_MODULE_ERROR_NOT_INITIALISED;

    // Clean up command line
    {
        // Return to beginning of line 
        fprintf(stdout, "\r");

        // Clear line
        console_cursor_clear(CONSOLE_CURSOR_MODE_TO_END);

        // Inactivate command line
        command_module_inactivate_command_line();
        
        fflush(stdout);
    }
    
    // Clean up command module state
    {
        // Temp buffer
        if (memory_zone_deallocate("command", state->temp_buffer, state->temp_buffer_memory_size) != MEMORY_ZONE_SUCCESS)
            return COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_TEMP_BUFFER;

        // History
        if (memory_zone_deallocate("command", state->history, state->history_memory_size) != MEMORY_ZONE_SUCCESS)
            return COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_HISTORY;

        // Token list
        if (memory_zone_deallocate("command", state->token_list, state->token_list_memory_size) != MEMORY_ZONE_SUCCESS)
            return COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_TOKEN_LIST;

        // Token buffer
        if (memory_zone_deallocate("command", state->token_buffer, state->token_buffer_memory_size) != MEMORY_ZONE_SUCCESS)
            return COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_TOKEN_BUFFER;

        // Prompt header
        if (memory_zone_deallocate("command", state->prompt_header, state->prompt_header_memory_size) != MEMORY_ZONE_SUCCESS)
        return COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_PROMPT_HEADER;
        
        // Input buffer
        if (memory_zone_deallocate("command", state->input_buffer, state->input_buffer_memory_size) != MEMORY_ZONE_SUCCESS)
            return COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_INPUT_BUFFER;

        // Registry actions map
        if (container_map_size(state->registry) > 0) {
            // Registry is not empty...

            for (ByteSize index = 0; index < container_map_capacity(state->registry); ++index) {
                Map action_map = NULL;
                
                if (container_map_at_index(state->registry, index, NULL, (VoidPtr)&action_map) == CONTAINER_SUCCESS) {
                    // Actions map at this index is registered...

                    for (ByteSize am_index = 0; am_index < container_map_capacity(action_map); ++am_index) {
                        CommandAction cmd_action;

                        if (container_map_at_index(action_map, am_index, NULL, (VoidPtr)&cmd_action) == CONTAINER_SUCCESS) {
                            // Command action at this index is registered...

                            if (cmd_action.desc && container_string_destruct(&cmd_action.desc) != CONTAINER_SUCCESS)
                                return COMMAND_MODULE_ERROR_FAILED_TO_DESTRUCT_COMMAND_ACTION_DESCRIPTION_STRING;

                            if (cmd_action.usage && container_string_destruct(&cmd_action.usage) != CONTAINER_SUCCESS)
                                return COMMAND_MODULE_ERROR_FAILED_TO_DESTRUCT_COMMAND_ACTION_USAGE_STRING;
                        }
                    }

                    // Destruct actions map
                    if (container_map_destruct(&action_map) != CONTAINER_SUCCESS)
                        return COMMAND_MODULE_ERROR_FAILED_TO_DESTRUCT_ACTION_MAP_FOR_ROOT;
                }
            }
        }

        // Registry
        if (container_map_destruct(&state->registry) != CONTAINER_SUCCESS)
            return COMMAND_MODULE_ERROR_FAILED_TO_DESTRUCT_REGISTRY;
    }

    // Deallocate state
    if (memory_zone_deallocate("command", (VoidPtr)state, state->memory_size) != MEMORY_ZONE_SUCCESS)
        return COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE;

    // Set to NULL
    state = NULL; 

    return COMMAND_MODULE_SUCCESS;
}

KYRA_ENGINE_API CommandModuleResult command_module_setup_command_line(ConstStr header) {
    if (!state) return COMMAND_MODULE_ERROR_NOT_INITIALISED;

    // Initialise prompt header
    state->prompt_header_size = snprintf(state->prompt_header, KYRA_SHORT_LINE_MAX_LENGTH, "%s > ", header);

    // Activate command line
    state->command_line_active = true;

    // Update command line to reflect change
    CommandModuleResult update_result = command_module_update_command_line();
    if (update_result != COMMAND_MODULE_SUCCESS) return update_result;

    return COMMAND_MODULE_SUCCESS;
}

KYRA_ENGINE_API CommandModuleResult command_module_activate_command_line(void) {
    if (!state) return COMMAND_MODULE_ERROR_NOT_INITIALISED;
    
    // Can't activate the already activated
    if (state->command_line_active) return COMMAND_MODULE_SUCCESS;

    // Activate command line
    state->command_line_active = true;

    // Update command line to reflect change
    CommandModuleResult update_result = command_module_update_command_line();
    if (update_result != COMMAND_MODULE_SUCCESS) return update_result;

    return COMMAND_MODULE_SUCCESS;
}

KYRA_ENGINE_API CommandModuleResult command_module_inactivate_command_line(void) {
    if (!state) return COMMAND_MODULE_ERROR_NOT_INITIALISED;
    
    // Can't in-activate the already in-activated
    if (!state->command_line_active) return COMMAND_MODULE_SUCCESS;

    // Return to beginning of line
    // Clear line
    {
        fprintf(stdout, "\r");
        console_cursor_clear(CONSOLE_CURSOR_MODE_TO_END);

        fflush(stdout);
    }

    // Inactivate command line
    state->command_line_active = false;

    return COMMAND_MODULE_SUCCESS;
}

KYRA_ENGINE_API CommandModuleResult command_module_update_command_line(void) {
    if (!state) return COMMAND_MODULE_ERROR_NOT_INITIALISED;

    // Return to beginning of line
    // Clear line
    {
        fprintf(stdout, "\r");
        console_cursor_clear(CONSOLE_CURSOR_MODE_TO_END);
    }

    // Print prompt
    {
        console_set_foreground(CONSOLE_COLOUR_BRIGHT_CYAN);
        fprintf(stdout, "%s", state->prompt_header);
        console_reset();
    }

    // Print input buffer
    fprintf(stdout, "%s", state->input_buffer);

    // Move cursor to absolute position
    // Position: sizeof prompt_header + cursor_position + 1
    console_cursor_move_absolute((Int32)(state->prompt_header_size + state->cursor_position + 1));

    return COMMAND_MODULE_SUCCESS;
}

KYRA_ENGINE_API CommandModuleResult command_module_register_schema(ConstStr schema_filepath) {
    if (!state) return COMMAND_MODULE_ERROR_NOT_INITIALISED;
    if (!schema_filepath) return COMMAND_MODULE_ERROR_SCHEMA_FILEPATH_NULL;

    // Open schema file
    File schema_file = {0};
    if (platform_filesystem_file_open(schema_filepath, FILESYSTEM_IO_MODE_READ, FILESYSTEM_FILE_MODE_BINARY, &schema_file) != FILESYSTEM_SUCCESS)
        return COMMAND_MODULE_ERROR_FAILED_TO_OPEN_SCHEMA_FILE;

    // Get file size
    ByteSize size = 0;
    platform_filesystem_file_size(&schema_file, &size);

    Str buffer = NULL;
    ByteSize buffer_memsize = 0;

    // Allocate raw buffer
    if (memory_zone_allocate("command", size, (VoidPtr *)&buffer, &buffer_memsize) != MEMORY_ZONE_SUCCESS)
        return COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_SCHEMA_RAW_BUFFER;

    // Read entire file data
    ByteSize read_bytes = 0;
    if (platform_filesystem_read_all(&schema_file, &read_bytes, &buffer) != FILESYSTEM_SUCCESS)
        return COMMAND_MODULE_ERROR_FAILED_TO_READ_SCHEMA_FILE;
    
    // Null-terminate
    buffer[read_bytes] = '\0';

    // Close file
    if (platform_filesystem_file_close(&schema_file) != FILESYSTEM_SUCCESS)
        return COMMAND_MODULE_ERROR_FAILED_TO_CLOSE_SCHEMA_FILE;
        
    // Parse to JSON
    cJSON *json = cJSON_Parse(buffer);
    if (!json) return COMMAND_MODULE_ERROR_FAILED_TO_PARSE_SCHEMA_BUFFER_TO_JSON;

    // Deallocate raw buffer 
    if (memory_zone_deallocate("command", (VoidPtr)buffer, buffer_memsize) != MEMORY_ZONE_SUCCESS)
        return COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_SCHEMA_RAW_BUFFER;

    cJSON *root = cJSON_GetObjectItemCaseSensitive(json, "root");
    ConstStr root_name = root->valuestring;
    
    if (container_map_contains(state->registry, root_name)) {
        // Root name is registered to registry...
        
        // Return
        return COMMAND_MODULE_SUCCESS;
    }
    
    // Construct actions map (Map<action_name: ConstStr, action: CommandAction>)
    Map action_map;
    if (container_map_construct(sizeof(CommandAction), &action_map) != CONTAINER_SUCCESS)
        return COMMAND_MODULE_ERROR_FAILED_TO_CONSTRUCT_ACTION_MAP_FOR_ROOT;

    // Parse actions
    {
        cJSON *actions = cJSON_GetObjectItemCaseSensitive(json, "actions");
        
        cJSON *action;
        cJSON_ArrayForEach(action, actions) {
            // For every action...

            // Action name
            ConstStr action_name = action->string;

            CommandAction cmd_action;

            // Number of arguments
            cJSON *num_args = cJSON_GetObjectItemCaseSensitive(action, "num_args");
            if (cJSON_IsNumber(num_args)) cmd_action.num_args = num_args->valueint;
            
            // Description
            cJSON *desc = cJSON_GetObjectItemCaseSensitive(action, "desc");
            if (cJSON_IsString(desc)) {
                if (container_string_construct(desc->valuestring, &cmd_action.desc) != CONTAINER_SUCCESS)
                    return COMMAND_MODULE_ERROR_FAILED_TO_CONSTRUCT_COMMAND_ACTION_DESCRIPTION_STRING;
            }

            // Usage
            cJSON *usage = cJSON_GetObjectItemCaseSensitive(action, "usage");
            if (cJSON_IsString(usage)) {
                if (container_string_construct(usage->valuestring, &cmd_action.usage) != CONTAINER_SUCCESS)
                    return COMMAND_MODULE_ERROR_FAILED_TO_CONSTRUCT_COMMAND_ACTION_USAGE_STRING;
            }

            // Register to map
            if (container_map_insert(&action_map, action_name, (VoidPtr)&cmd_action) != CONTAINER_SUCCESS)
                return COMMAND_MODULE_ERROR_FAILED_TO_REGISTER_COMMAND_ACTION;
        }
    }

    // Register actions map to registry
    if (container_map_insert(&state->registry, root_name, (VoidPtr)&action_map) != CONTAINER_SUCCESS)
        return COMMAND_MODULE_ERROR_FAILED_TO_REGISTER_ACTION_MAP_TO_REGISTRY;

    return COMMAND_MODULE_SUCCESS;
}

KYRA_ENGINE_API CommandModuleResult command_module_register_action(ConstStr root, ConstStr action, const Listener listener, const DelegateFunction callback) {
    if (!state) return COMMAND_MODULE_ERROR_NOT_INITIALISED;
    if (!root) return COMMAND_MODULE_ERROR_ROOT_NAME_NULL;
    if (!action) return COMMAND_MODULE_ERROR_ACTION_NAME_NULL;

    // Omit '/'
    ConstStr root_name = (root[0]) == '/' ? &root[1] : root;

    if (!container_map_contains(state->registry, root_name)) {
        KYRA_CONSOLE_PRINT_ERROR("Command Module: Failed to locate root '/%s'.", root_name);
        KYRA_CONSOLE_PRINT_ERROR("You may need to register schema beforehand (via 'command_module_register_schema').");
    
        return COMMAND_MODULE_ERROR_FAILED_TO_LOCATE_ROOT_IN_REGISTRY;
    }

    // Locate actions map for root name
    Map action_map = NULL;
    if (container_map_search(state->registry, root_name, (VoidPtr)&action_map) != CONTAINER_SUCCESS) {
        KYRA_CONSOLE_PRINT_ERROR("Failed to locate action map for '/%s'.", root_name);
        KYRA_CONSOLE_PRINT_ERROR("You may need to register schema beforehand (via 'command_module_register_schema').");
    
        return COMMAND_MODULE_ERROR_FAILED_TO_LOCATE_ACTION_MAP_FOR_ROOT;
    }

    // Craft delegate ID (from root name and action name)
    Char delegate_id[KYRA_SHORT_LINE_MAX_LENGTH];
    snprintf(delegate_id, sizeof(delegate_id), "kyra_command_%s_%s", root_name, action);

    // Register delegate by ID with listener and callback
    if (delegate_unicast_register(delegate_id, listener, callback) != DELEGATE_SUCCESS)
        return COMMAND_MODULE_ERROR_FAILED_TO_REGISTER_DELEGATE_FOR_ACTION;

    return COMMAND_MODULE_SUCCESS;
}

KYRA_ENGINE_API CommandModuleResult command_module_update_action(ConstStr root, ConstStr action, const Listener listener, const DelegateFunction callback) {
    if (!state) return COMMAND_MODULE_ERROR_NOT_INITIALISED;
    if (!root) return COMMAND_MODULE_ERROR_ROOT_NAME_NULL;
    if (!action) return COMMAND_MODULE_ERROR_ACTION_NAME_NULL;

    // Omit '/'
    ConstStr root_name = (root[0]) == '/' ? &root[1] : root;

    if (!container_map_contains(state->registry, root_name)) {
        KYRA_CONSOLE_PRINT_ERROR("Command Module: Failed to locate root '/%s'.", root_name);
        KYRA_CONSOLE_PRINT_ERROR("You may need to register schema beforehand (via 'command_module_register_schema').");
    
        return COMMAND_MODULE_ERROR_FAILED_TO_LOCATE_ROOT_IN_REGISTRY;
    }

    // Locate actions map for root name
    Map action_map = NULL;
    if (container_map_search(state->registry, root_name, (VoidPtr)&action_map) != CONTAINER_SUCCESS) {
        KYRA_CONSOLE_PRINT_ERROR("Failed to locate action map for '/%s'.", root_name);
        KYRA_CONSOLE_PRINT_ERROR("You may need to register schema beforehand (via 'command_module_register_schema').");
    
        return COMMAND_MODULE_ERROR_FAILED_TO_LOCATE_ACTION_MAP_FOR_ROOT;
    }

    // Craft delegate ID (from root name and action name)
    Char delegate_id[KYRA_SHORT_LINE_MAX_LENGTH];
    snprintf(delegate_id, sizeof(delegate_id), "kyra_command_%s_%s", root_name, action);

    // Update listener and callback
    if (delegate_unicast_update(delegate_id, listener, callback) != DELEGATE_SUCCESS)
        return COMMAND_MODULE_ERROR_FAILED_TO_UPDATE_DELEGATE_FOR_ACTION;

    return COMMAND_MODULE_SUCCESS;
}

KYRA_ENGINE_API CommandModuleResult command_module_dispatch(const String command, const Sender sender) {
    if (!state) return COMMAND_MODULE_ERROR_NOT_INITIALISED;
    if (!command) return COMMAND_MODULE_ERROR_COMMAND_NULL;
    if (container_string_size(command) == 0) return COMMAND_MODULE_ERROR_COMMAND_EMPTY;

    ConstStr cmd_data = container_string_cstr(command); 
    ConstStr cmd_start = (cmd_data[0] == '/') ? &cmd_data[1] : cmd_data;

    UInt32 total_tokens = _command_module_tokenise(cmd_start, state->token_list, state->max_arguments);

    // Return if no token to process
    if (total_tokens == 0) COMMAND_MODULE_SUCCESS;

    // Get root and action names
    ConstStr root_name = state->token_list[0];
    ConstStr action_name = state->token_list[1];

    if (!container_map_contains(state->registry, root_name)) {
        KYRA_CONSOLE_PRINT_ERROR("Failed to locate root '/%s'.", root_name);
        KYRA_CONSOLE_PRINT_ERROR("You may need to register schema beforehand (via 'command_module_register_schema').");
    
        return COMMAND_MODULE_ERROR_FAILED_TO_LOCATE_ROOT_IN_REGISTRY;
    }

    // Get actions map for root name
    Map action_map = NULL;
    if (container_map_search(state->registry, root_name, (VoidPtr)&action_map) != CONTAINER_SUCCESS) {
        KYRA_CONSOLE_PRINT_ERROR("Failed to locate action map for '/%s'.", root_name);
        KYRA_CONSOLE_PRINT_ERROR("You may need to register schema beforehand (via 'command_module_register_schema').");
    
        return COMMAND_MODULE_ERROR_FAILED_TO_LOCATE_ACTION_MAP_FOR_ROOT;
    }

    // Get command action by action name
    CommandAction cmd_action;
    if (container_map_search(action_map, action_name, (VoidPtr)&cmd_action) != CONTAINER_SUCCESS) {
        KYRA_CONSOLE_PRINT_ERROR("Failed to locate '%s' in actions map for '/%s'.", action_name, root_name);
        KYRA_CONSOLE_PRINT_ERROR("You may need to register schema beforehand (via 'command_module_register_schema').");
    
        return COMMAND_MODULE_ERROR_FAILED_TO_LOCATE_COMMAND_ACTION;
    }

    UInt32 root_action_tokens = 2;
    UInt32 num_args = total_tokens - root_action_tokens;
    
    if (num_args < cmd_action.num_args) {
        KYRA_CONSOLE_PRINT_ERROR("Not enough arguments for '/%s %s'.", root_name, action_name);
        KYRA_CONSOLE_PRINT_ERROR("Usage: %s", container_string_cstr(cmd_action.usage));

        return COMMAND_MODULE_ERROR_NOT_ENOUGH_ARGUMENTS_FOR_COMMAND_DISPATCH;
    }

    CommandContext context;
    context.root_hash = hash_str(root_name, HASH_MODE_XXH3);
    context.action_hash = hash_str(action_name, HASH_MODE_XXH3);
    context.args = (num_args > 0) ? &state->token_list[2] : NULL;
    context.num_args = num_args;

    // Craft delegate ID (from root name and action name)
    Char delegate_id[KYRA_SHORT_LINE_MAX_LENGTH];
    snprintf(delegate_id, sizeof(delegate_id), "kyra_command_%s_%s", root_name, action_name);

    // Invoke delegate
    if (delegate_unicast_invoke(delegate_id, sender, (VoidPtr)&context) != DELEGATE_SUCCESS)
        return COMMAND_MODULE_ERROR_FAILED_TO_INVOKE_DELEGATE_FOR_ACTION;

    return COMMAND_MODULE_SUCCESS;
}

KYRA_ENGINE_API CommandModuleResult command_module_poll_input(String *out_string) {
    if (!state) return COMMAND_MODULE_ERROR_NOT_INITIALISED;
    if (!out_string) return COMMAND_MODULE_ERROR_REF_OUT_STRING_NULL;

    #if KYRA_PLATFORM_WINDOWS
        // For Windows...

        if (_kbhit()) {
            // Get input
            Int32 key = _getch();
            Char ch = (Char)key;

            if (key == 0 || key == 0xe0) {
                // Detected first byte indicating a special key...

                // Read second byte
                key = _getch();

                switch (key) {
                    // -- UP ARROW -- //
                    case 72:
                        if ((state->history_limit > 0) && (state->history_size > 0)) {
                            // Entering history list...

                            // Save current input buffer to temp buffer
                            if ((state->input_buffer_size > 0) && (state->history_view_index == -1)) {
                                strncpy(state->temp_buffer, state->input_buffer, KYRA_LINE_MAX_LENGTH);
                                
                                // Update temp buffer size
                                state->temp_buffer_size = state->input_buffer_size;
                            }

                            // Advance history view index
                            if (state->history_view_index < (Int32)(state->history_size - 1)) ++state->history_view_index;
                            
                            if (state->history_view_index >= 0) {
                                // Copy element buffer to input buffer
                                Str elem = (Str)((UIntPtr)state->history + (state->history_view_index * KYRA_LINE_MAX_LENGTH));
                                strncpy(state->input_buffer, elem, KYRA_LINE_MAX_LENGTH);
                                
                                // Update input buffer and cursor state
                                {
                                    state->input_buffer_size = strlen(state->input_buffer);
                                    state->cursor_position = state->input_buffer_size;
                                }

                                // Update command line to reflect change
                                CommandModuleResult update_result = command_module_update_command_line();
                                if (update_result != COMMAND_MODULE_SUCCESS) return update_result;
                            }
                        }

                        break;
                    
                    // -- DOWN ARROW -- //
                    case 80:
                        if (state->history_limit > 0) {
                            // Roll back history view index
                            if (state->history_view_index > -1) --state->history_view_index;
                            
                            // Leaving history list...
                            
                            // Restore input buffer from temp buffer
                            if (state->history_view_index == -1) {
                                strncpy(state->input_buffer, state->temp_buffer, KYRA_LINE_MAX_LENGTH);
                                
                                // Update input buffer size and cursor state
                                {
                                    state->input_buffer_size = state->temp_buffer_size;
                                    state->cursor_position = state->input_buffer_size; 
                                }
                            }

                            // Otherwise...
                            else {
                                // Still in history list...

                                // Copy element buffer to input buffer
                                Str elem = (Str)((UIntPtr)state->history + (state->history_view_index * KYRA_LINE_MAX_LENGTH));
                                strncpy(state->input_buffer, elem, KYRA_LINE_MAX_LENGTH);

                                // Update input buffer and cursor state
                                {
                                    state->input_buffer_size = strlen(state->input_buffer);
                                    state->cursor_position = state->input_buffer_size;
                                }
                            }

                            // Update command line to reflect change
                            CommandModuleResult update_result = command_module_update_command_line();
                            if (update_result != COMMAND_MODULE_SUCCESS) return update_result;
                        }

                        break;

                    // -- LEFT ARROW -- //
                    case 75:
                        if (state->cursor_position > 0) {
                            // Roll back cursor position
                            --state->cursor_position;
                            
                            // Update command line to reflect change
                            CommandModuleResult update_result = command_module_update_command_line();
                            if (update_result != COMMAND_MODULE_SUCCESS) return update_result;
                        }

                        break;

                    // -- RIGHT ARROW -- //
                    case 77:
                        if (state->cursor_position < state->input_buffer_size) {
                            // Advance cursor position
                            ++state->cursor_position;

                            // Update command line to reflect change
                            CommandModuleResult update_result = command_module_update_command_line();
                            if (update_result != COMMAND_MODULE_SUCCESS) return update_result;
                        }

                        break;
                    }

                return COMMAND_MODULE_POLL_STATUS_PENDING;
            }
            else {
                // Otherwise, standard key...

                switch (ch) {
                    // -- ENTER -- //
                    case '\r':
                    case '\n':
                        // Null terminate
                        state->input_buffer[state->input_buffer_size] = '\0';

                        // Echo prompt (if specified)
                        if (state->echo_prompt) {
                            // Return to beginning of line
                            fprintf(stdout, "\r");

                            // Clear line
                            console_cursor_clear(CONSOLE_CURSOR_MODE_TO_END);

                            // Echo print prompt
                            KYRA_CONSOLE_PRINT_FG(CONSOLE_COLOUR_GREEN, "%s%s", state->prompt_header, state->input_buffer);
                            
                            fflush(stdout);
                        }

                        // Handle history
                        if ((state->input_buffer_size > 0) && (state->history_limit > 0)) {
                            // Shift trailing 'elements' to the right
                            ByteSize shift_size = (state->history_size < state->history_limit) ? (state->history_limit - 1) * KYRA_LINE_MAX_LENGTH : state->history_size * KYRA_LINE_MAX_LENGTH;
                            memmove((Str)((UIntPtr)state->history + KYRA_LINE_MAX_LENGTH), state->history, shift_size);

                            // Save input buffer 'snapshot' to history list
                            strncpy(state->history, state->input_buffer, KYRA_LINE_MAX_LENGTH);                                

                            // Reset history view index
                            state->history_view_index = -1;

                            // Increment history list size until reached history limit
                            if (state->history_size < state->history_limit) ++state->history_size;

                            // Reset temp buffer
                            {
                                memset(state->temp_buffer, 0, state->temp_buffer_size);
                                state->temp_buffer_size = 0;
                            }
                        }

                        // Construct input string
                        // Save to ref (for command processing)
                        if (container_string_construct(state->input_buffer, out_string) != CONTAINER_SUCCESS)
                            return COMMAND_MODULE_ERROR_FAILED_TO_CONSTRUCT_INPUT_BUFFER_STRING;

                        // Reset input buffer and cursor state
                        {
                            memset(state->input_buffer, 0, state->input_buffer_size);
                            state->input_buffer_size = 0;
                            
                            state->cursor_position = 0;
                        }

                        KYRA_CONSOLE_PRINT_INFO("limit: %llu, size: %llu", state->history_limit, state->history_size);

                        // Return success to signify command line being registered
                        return COMMAND_MODULE_SUCCESS;

                    // -- BACKSPACE -- //
                    case '\b':
                        // Backspace only works if cursor not at beginning of line
                        if (state->cursor_position > 0) {
                            if (state->history_view_index > -1) {
                                // Inside history list...

                                // Copy element buffer to input buffer
                                Str elem = (Str)((UIntPtr)state->history + (state->history_view_index * KYRA_LINE_MAX_LENGTH));
                                strncpy(state->input_buffer, elem, KYRA_LINE_MAX_LENGTH);

                                // Update input buffer
                                state->input_buffer_size = strlen(state->input_buffer);
                            
                                // Leave history list immediately
                                state->history_view_index = -1;
                            }

                            // Shift all trailing character to the left
                            ByteSize shift_size = state->input_buffer_size - state->cursor_position + 1;
                            memmove(&state->input_buffer[state->cursor_position - 1], &state->input_buffer[state->cursor_position], shift_size);

                            // Decrement input buffer size
                            // Null terminate
                            state->input_buffer[--state->input_buffer_size] = '\0';

                            // Decrement cursor position
                            --state->cursor_position;

                            // Update command line to reflect change
                            CommandModuleResult update_result = command_module_update_command_line();
                            if (update_result != COMMAND_MODULE_SUCCESS) return update_result;
                        }

                        break;

                    // -- NON-SPECIAL CHARACTERS -- //
                    default:
                        // Covering ASCII range for printable characters
                        if ((ch >= 32 && ch <= 126) && (state->input_buffer_size < KYRA_LINE_MAX_LENGTH - 1)) {
                            if (state->history_view_index > -1) {
                                // Inside history list...

                                // Copy element buffer to input buffer
                                Str elem = (Str)((UIntPtr)state->history + (state->history_view_index * KYRA_LINE_MAX_LENGTH));
                                strncpy(state->input_buffer, elem, KYRA_LINE_MAX_LENGTH);

                                // Update input buffer
                                state->input_buffer_size = strlen(state->input_buffer);
                            
                                // Leave history list immediately
                                state->history_view_index = -1;
                            }
                            
                            // Handle character insertion
                            if (state->cursor_position < state->input_buffer_size) {
                                // Shift all trailing characters to the right
                                ByteSize shift_size = state->input_buffer_size - state->cursor_position + 1;
                                memmove(&state->input_buffer[state->cursor_position + 1], &state->input_buffer[state->cursor_position], shift_size);
                            }

                            // Assign pressed key character
                            // Advance cursor position
                            state->input_buffer[state->cursor_position++] = ch;

                            // Increment input buffer size
                            // Null terminate
                            state->input_buffer[++state->input_buffer_size] = '\0';

                            // Update command line to reflect change
                            CommandModuleResult update_result = command_module_update_command_line();
                            if (update_result != COMMAND_MODULE_SUCCESS) return update_result;
                        }

                        break;
                }
            }
        }
        
    #endif

    return COMMAND_MODULE_POLL_STATUS_PENDING;
}

KYRA_ENGINE_API Bool command_module_command_line_active(void) {
    if (!state) return false;

    return state->command_line_active;
}

KYRA_ENGINE_API ConstStr command_module_result_to_string(const CommandModuleResult result) {
    switch (result) {
        case COMMAND_MODULE_SUCCESS:                                                        return "COMMAND_MODULE_SUCCESS";

        case COMMAND_MODULE_ERROR_ALREADY_INITIALISED:                                      return "COMMAND_MODULE_ERROR_ALREADY_INITIALISED";
        case COMMAND_MODULE_ERROR_NOT_INITIALISED:                                          return "COMMAND_MODULE_ERROR_NOT_INITIALISED";
        case COMMAND_MODULE_ERROR_CONFIG_FILEPATH_NULL:                                     return "COMMAND_MODULE_ERROR_CONFIG_FILEPATH_NULL";
        case COMMAND_MODULE_ERROR_SCHEMA_FILEPATH_NULL:                                     return "COMMAND_MODULE_ERROR_SCHEMA_FILEPATH_NULL";
        case COMMAND_MODULE_ERROR_ROOT_NAME_NULL:                                           return "COMMAND_MODULE_ERROR_ROOT_NAME_NULL";
        case COMMAND_MODULE_ERROR_ACTION_NAME_NULL:                                         return "COMMAND_MODULE_ERROR_ACTION_NAME_NULL";
        case COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE:                      return "COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE";
        case COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE:                     return "COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE";
        case COMMAND_MODULE_ERROR_FAILED_TO_OPEN_CONFIG_FILE:                               return "COMMAND_MODULE_ERROR_FAILED_TO_OPEN_CONFIG_FILE";
        case COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_CONFIG_RAW_BUFFER:          return "COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_CONFIG_RAW_BUFFER";
        case COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_CONFIG_RAW_BUFFER:         return "COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_CONFIG_RAW_BUFFER";
        case COMMAND_MODULE_ERROR_FAILED_TO_READ_CONFIG_FILE:                               return "COMMAND_MODULE_ERROR_FAILED_TO_READ_CONFIG_FILE";
        case COMMAND_MODULE_ERROR_FAILED_TO_CLOSE_CONFIG_FILE:                              return "COMMAND_MODULE_ERROR_FAILED_TO_CLOSE_CONFIG_FILE";
        case COMMAND_MODULE_ERROR_FAILED_TO_PARSE_CONFIG_BUFFER_TO_JSON:                    return "COMMAND_MODULE_ERROR_FAILED_TO_PARSE_CONFIG_BUFFER_TO_JSON";
        case COMMAND_MODULE_ERROR_FAILED_TO_CONSTRUCT_REGISTRY:                             return "COMMAND_MODULE_ERROR_FAILED_TO_CONSTRUCT_REGISTRY";
        case COMMAND_MODULE_ERROR_FAILED_TO_DESTRUCT_REGISTRY:                              return "COMMAND_MODULE_ERROR_FAILED_TO_DESTRUCT_REGISTRY";
        case COMMAND_MODULE_ERROR_FAILED_TO_OPEN_SCHEMA_FILE:                               return "COMMAND_MODULE_ERROR_FAILED_TO_OPEN_SCHEMA_FILE";
        case COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_SCHEMA_RAW_BUFFER:          return "COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_SCHEMA_RAW_BUFFER";
        case COMMAND_MODULE_ERROR_FAILED_TO_READ_SCHEMA_FILE:                               return "COMMAND_MODULE_ERROR_FAILED_TO_READ_SCHEMA_FILE";
        case COMMAND_MODULE_ERROR_FAILED_TO_CLOSE_SCHEMA_FILE:                              return "COMMAND_MODULE_ERROR_FAILED_TO_CLOSE_SCHEMA_FILE";
        case COMMAND_MODULE_ERROR_FAILED_TO_PARSE_SCHEMA_BUFFER_TO_JSON:                    return "COMMAND_MODULE_ERROR_FAILED_TO_PARSE_SCHEMA_BUFFER_TO_JSON";
        case COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_SCHEMA_RAW_BUFFER:         return "COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_SCHEMA_RAW_BUFFER";
        case COMMAND_MODULE_ERROR_FAILED_TO_CONSTRUCT_ACTION_MAP_FOR_ROOT:                  return "COMMAND_MODULE_ERROR_FAILED_TO_CONSTRUCT_ACTION_MAP_FOR_ROOT";
        case COMMAND_MODULE_ERROR_FAILED_TO_DESTRUCT_ACTION_MAP_FOR_ROOT:                   return "COMMAND_MODULE_ERROR_FAILED_TO_DESTRUCT_ACTION_MAP_FOR_ROOT";
        case COMMAND_MODULE_ERROR_FAILED_TO_CONSTRUCT_COMMAND_ACTION_DESCRIPTION_STRING:    return "COMMAND_MODULE_ERROR_FAILED_TO_CONSTRUCT_COMMAND_ACTION_DESCRIPTION_STRING";
        case COMMAND_MODULE_ERROR_FAILED_TO_DESTRUCT_COMMAND_ACTION_DESCRIPTION_STRING:     return "COMMAND_MODULE_ERROR_FAILED_TO_DESTRUCT_COMMAND_ACTION_DESCRIPTION_STRING";
        case COMMAND_MODULE_ERROR_FAILED_TO_CONSTRUCT_COMMAND_ACTION_USAGE_STRING:          return "COMMAND_MODULE_ERROR_FAILED_TO_CONSTRUCT_COMMAND_ACTION_USAGE_STRING";
        case COMMAND_MODULE_ERROR_FAILED_TO_DESTRUCT_COMMAND_ACTION_USAGE_STRING:           return "COMMAND_MODULE_ERROR_FAILED_TO_DESTRUCT_COMMAND_ACTION_USAGE_STRING";
        case COMMAND_MODULE_ERROR_FAILED_TO_REGISTER_COMMAND_ACTION:                        return "COMMAND_MODULE_ERROR_FAILED_TO_REGISTER_COMMAND_ACTION";
        case COMMAND_MODULE_ERROR_FAILED_TO_REGISTER_ACTION_MAP_TO_REGISTRY:                return "COMMAND_MODULE_ERROR_FAILED_TO_REGISTER_ACTION_MAP_TO_REGISTRY";
        case COMMAND_MODULE_ERROR_FAILED_TO_LOCATE_ROOT_IN_REGISTRY:                        return "COMMAND_MODULE_ERROR_FAILED_TO_LOCATE_ROOT_IN_REGISTRY";
        case COMMAND_MODULE_ERROR_FAILED_TO_LOCATE_ACTION_MAP_FOR_ROOT:                     return "COMMAND_MODULE_ERROR_FAILED_TO_LOCATE_ACTION_MAP_FOR_ROOT";
        case COMMAND_MODULE_ERROR_FAILED_TO_REGISTER_DELEGATE_FOR_ACTION:                   return "COMMAND_MODULE_ERROR_FAILED_TO_REGISTER_DELEGATE_FOR_ACTION";
        case COMMAND_MODULE_ERROR_FAILED_TO_UPDATE_DELEGATE_FOR_ACTION:                     return "COMMAND_MODULE_ERROR_FAILED_TO_UPDATE_DELEGATE_FOR_ACTION";   
        case COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_INPUT_BUFFER:               return "COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_INPUT_BUFFER";
        case COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_INPUT_BUFFER:              return "COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_INPUT_BUFFER";
        case COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_PROMPT_HEADER:              return "COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_PROMPT_HEADER";
        case COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_PROMPT_HEADER:             return "COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_PROMPT_HEADER";
        case COMMAND_MODULE_ERROR_FAILED_TO_CONSTRUCT_INPUT_BUFFER_STRING:                  return "COMMAND_MODULE_ERROR_FAILED_TO_CONSTRUCT_INPUT_BUFFER_STRING";
        case COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_TOKEN_BUFFER:               return "COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_TOKEN_BUFFER";
        case COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_TOKEN_BUFFER:              return "COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_TOKEN_BUFFER";
        case COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_TOKEN_LIST:                 return "COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_TOKEN_LIST";
        case COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_TOKEN_LIST:                return "COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_TOKEN_LIST";
        case COMMAND_MODULE_ERROR_FAILED_TO_LOCATE_COMMAND_ACTION:                          return "COMMAND_MODULE_ERROR_FAILED_TO_LOCATE_COMMAND_ACTION";
        case COMMAND_MODULE_ERROR_NOT_ENOUGH_ARGUMENTS_FOR_COMMAND_DISPATCH:                return "COMMAND_MODULE_ERROR_NOT_ENOUGH_ARGUMENTS_FOR_COMMAND_DISPATCH";
        case COMMAND_MODULE_ERROR_FAILED_TO_INVOKE_DELEGATE_FOR_ACTION:                     return "COMMAND_MODULE_ERROR_FAILED_TO_INVOKE_DELEGATE_FOR_ACTION";
        case COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_HISTORY:                    return "COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_HISTORY";
        case COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_HISTORY:                   return "COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_HISTORY";
        case COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_TEMP_BUFFER:                return "COMMAND_MODULE_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_TEMP_BUFFER";
        case COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_TEMP_BUFFER:               return "COMMAND_MODULE_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_TEMP_BUFFER";

        default:                                                                            return "UNKNOWN_COMMAND_MODULE_RESULT"; 
    }
}

