#include "kyra/core/misc/timer/timer.h"

#include <math.h>
#include <string.h>

#include <cJSON.h>

#include "kyra/core/memory/zone/memory_zone.h"
#include "kyra/core/hal/clock/hi_res/hi_res.h"
#include "kyra/core/platform/filesystem/filesystem.h"
#include "kyra/core/logger/logger.h"
#include "kyra/core/hash/hash.h"


// Internal state ----------------------------------------------- //

typedef struct Gameplay_Timer {
    HashedID            hashed_id;

    Flt64               duration;
    Flt64               elapsed;

    Bool                loop;
    Bool                active;
    Bool                paused;
    Bool                use_real_time;      // If true, ignore global pause / time scaling

    DelegateFunction    callback;
    Sender              sender;
    Listener            listener;
    VoidPtr             user_data;

} GameplayTimer;

typedef struct Timer_State {
    HiResClock          clock;

    Flt64               real_time_seconds;
    Flt64               real_delta_time;

    Flt64               game_time_seconds;
    Flt64               game_delta_time;

    Flt32               time_scale;
    Bool                paused;

    GameplayTimer      *timer_registry;
    ByteSize            max_active_timers;

    // For allocations/deallocations
    ByteSize            memory_size;

} TimerState;

static TimerState *state = NULL;


// API functions ------------------------------------------------ //

KYRA_ENGINE_API TimerResult timer_startup(ConstStr config_filepath) {
    if (state) return TIMER_ERROR_ALREADY_INITIALISED;    
    if (!config_filepath) return TIMER_ERROR_CONFIG_FILEPATH_NULL;

    // Open config file
    File config_file = {0};
    if (platform_filesystem_file_open(config_filepath, FILESYSTEM_IO_MODE_READ, FILESYSTEM_FILE_MODE_BINARY, &config_file) != FILESYSTEM_SUCCESS)
        return TIMER_ERROR_FAILED_TO_OPEN_CONFIG_FILE;

    Str buffer = NULL;
    ByteSize buffer_memsize = 0;
        
    // Read config file
    {
        // Get file size
        ByteSize size = 0;
        if (platform_filesystem_file_size(&config_file, &size) != FILESYSTEM_SUCCESS) {
            // Close config file
            if (platform_filesystem_file_close(&config_file) != FILESYSTEM_SUCCESS)
                return TIMER_ERROR_FAILED_TO_CLOSE_CONFIG_FILE;
            
            return TIMER_ERROR_FAILED_TO_GET_CONFIG_FILE_SIZE;
        }

        // Allocate raw buffer
        if (memory_zone_allocate("runtime", size + 1, (VoidPtr *)&buffer, &buffer_memsize) != MEMORY_ZONE_SUCCESS) {
            // Close config file
            if (platform_filesystem_file_close(&config_file) != FILESYSTEM_SUCCESS)
                return TIMER_ERROR_FAILED_TO_CLOSE_CONFIG_FILE;
            
            return TIMER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_CONFIG_RAW_BUFFER;
        }

        // Read entire file data
        ByteSize read_bytes = 0;
        if (platform_filesystem_read_all(&config_file, &read_bytes, &buffer) != FILESYSTEM_SUCCESS) {
            // Close config file
            if (platform_filesystem_file_close(&config_file) != FILESYSTEM_SUCCESS)
                return TIMER_ERROR_FAILED_TO_CLOSE_CONFIG_FILE;

            // Deallocate raw buffer 
            if (memory_zone_deallocate("runtime", (VoidPtr)buffer, buffer_memsize) != MEMORY_ZONE_SUCCESS)
                return TIMER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_CONFIG_RAW_BUFFER;
            
            return TIMER_ERROR_FAILED_TO_READ_CONFIG_FILE;
        }
        
        // Null-terminate
        buffer[read_bytes] = '\0';

        // Close config file
        if (platform_filesystem_file_close(&config_file) != FILESYSTEM_SUCCESS)
            return TIMER_ERROR_FAILED_TO_CLOSE_CONFIG_FILE;
    }

    cJSON *json = NULL;

    // Parse config file to JSON object
    {
        json = cJSON_Parse(buffer);
    
        // Deallocate raw buffer 
        if (memory_zone_deallocate("runtime", (VoidPtr)buffer, buffer_memsize) != MEMORY_ZONE_SUCCESS)
            return TIMER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_CONFIG_RAW_BUFFER;
    
        if (!json) return TIMER_ERROR_FAILED_TO_PARSE_CONFIG_BUFFER_TO_JSON;
    }

    // Locate runtime section inside config json
    cJSON *runtime = cJSON_GetObjectItemCaseSensitive(json, "runtime");
    if (!runtime) {
        // Failed to locate...

        // Delete config JSON object
        cJSON_Delete(json);
        
        return TIMER_ERROR_FAILED_TO_LOCATE_RUNTIME_SECTION_IN_CONFIG_JSON;
    }

    // Initialise configuration properties
    ByteSize config_max_active_timers = 0; 
    {
        // Max active timers
        cJSON *max_active = cJSON_GetObjectItemCaseSensitive(runtime, "max_active_timers");
        if (cJSON_IsNumber(max_active)) {
            config_max_active_timers = max_active->valueint;
        }
    }

    // Allocate for state
    // Initialise state properties 
    ByteSize mem_size = 0;
    {
        ByteSize state_size = KYRA_APPLY_MEMORY_ALIGNMENT(sizeof(TimerState), KYRA_MEMORY_ALIGNMENT_SIZE);
        ByteSize timer_registry_size = sizeof(GameplayTimer) * config_max_active_timers;
        ByteSize alloc_size = KYRA_APPLY_MEMORY_ALIGNMENT(state_size + timer_registry_size, KYRA_MEMORY_ALIGNMENT_SIZE);

        if (memory_zone_allocate("runtime", alloc_size, (VoidPtr *)&state, &mem_size) != MEMORY_ZONE_SUCCESS)
            return TIMER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE;

        state->timer_registry = (GameplayTimer *)KYRA_APPLY_MEMORY_ALIGNMENT((UIntPtr)state + sizeof(TimerState), KYRA_MEMORY_ALIGNMENT_SIZE);
        memset(state->timer_registry, 0, timer_registry_size);

        state->max_active_timers = config_max_active_timers;
        state->time_scale = 1.0f;
        state->paused = false;
    }

    // Assign state memory size
    state->memory_size = mem_size;

    // Delete config JSON object
    cJSON_Delete(json);

    // Start the engine timer clock
    if (clock_hires_split(&state->clock) != CLOCK_SUCCESS)
        return TIMER_ERROR_FAILED_TO_START_TIMER_CLOCK;

    return TIMER_SUCCESS;
}

KYRA_ENGINE_API TimerResult timer_shutdown(void) {
    if (!state) return TIMER_ERROR_NOT_INITIALISED;

    // Deallocate state
    if (memory_zone_deallocate("runtime", state, state->memory_size) != MEMORY_ZONE_SUCCESS)
        return TIMER_ERROR_FAILED_TO_DEALLOCCATE_MEMORY_OF_STATE;

    // Set to NULL
    state = NULL;

    return TIMER_SUCCESS;
}

KYRA_ENGINE_API TimerResult timer_update(const Flt64 delta_time) {
    if (!state) return TIMER_ERROR_NOT_INITIALISED;

    // Update times
    {
        state->real_delta_time = delta_time;
        state->real_time_seconds += delta_time;

        state->game_delta_time = state->paused ? 0.0f : (delta_time * state->time_scale);
        state->game_time_seconds += state->game_delta_time;
    }

    // Update active timers
    for (ByteSize index = 0; index < state->max_active_timers; ++index) {
        GameplayTimer *timer = &state->timer_registry[index];
        if (!timer->active || timer->paused) continue;

        // For every active timer...

        // Determine which delta time to apply
        Flt64 dt = timer->use_real_time ? state->real_delta_time : state->game_delta_time;
        timer->elapsed += dt;

        if (timer->elapsed >= timer->duration) {
            // Timer reaches its assigned duration...

            // Trigger callback delegate
            if (timer->callback) timer->callback(timer->sender, timer->listener, timer->user_data);

            // If looping, keep the remainder to prevent timer from drifting
            if (timer->loop) timer->elapsed = fmod(timer->elapsed, timer->duration);

            // Otherwise...
            // Clean up the timer slot since it is done
            else memset(timer, 0, sizeof(GameplayTimer));
        }
    }

    return TIMER_SUCCESS;
}

KYRA_ENGINE_API TimerResult timer_set_time_scale(const Flt32 scale) {
    if (!state) return TIMER_ERROR_NOT_INITIALISED;

    state->time_scale = scale;

    return TIMER_SUCCESS;
}

KYRA_ENGINE_API TimerResult timer_set_paused(const Bool paused) {
    if (!state) return TIMER_ERROR_NOT_INITIALISED;

    state->paused = paused;

    return TIMER_SUCCESS;
}

KYRA_ENGINE_API TimerResult timer_construct_timer(
    ConstStr id,
    const Flt64 duration, 
    const Bool loop, 
    const Bool use_real_time, 
    const DelegateFunction callback, 
    const Sender sender, 
    const Listener listener, 
    const VoidPtr user_data, 
    TimerHandle *out_handle
) {
    if (!state) return TIMER_ERROR_NOT_INITIALISED;
    if (!id) return TIMER_ERROR_TIMER_ID_NULL;
    if (!out_handle) return TIMER_ERROR_REF_OUT_HANDLE_NULL;

    HashedID hashed = hash_str(id, HASH_MODE_XXH3);
    GameplayTimer *write_slot = NULL;

    // Search if timer with this ID is already registered...
    for (ByteSize index = 0; index < state->max_active_timers; ++index) {
        GameplayTimer *timer_slot = &state->timer_registry[index]; 
        
        if (timer_slot->active && timer_slot->hashed_id == hashed) {
            // Found registered timer with matching ID...

            // Assign registered timer address to writing slot 
            write_slot = timer_slot;

            break;
        }
    }

    // If slot remains NULL, meaning it is new...
    if (!write_slot) {
        // Find the first inactive slot
        for (ByteSize index = 0; index < state->max_active_timers; ++index) {
            GameplayTimer *timer_slot = &state->timer_registry[index]; 
            
            if (!timer_slot->active) {
                // Found inactive slot...

                // Assign address to writing slot
                write_slot = timer_slot;
                
                break;
            }
        }
    }

    // If slot remains NULL, meaning the registry is full...
    if (!write_slot) {
        // Notify user with warning
        KYRA_LOG_ENGINE_WARN("Timer registry is full."); 
        
        // Invalidate handle
        out_handle->hashed_id = INVALID_HASH;

        // Return 
        return TIMER_ERROR_TIMER_REGISTRY_FULL;
    }

    // Initialise timer properties
    {
        write_slot->hashed_id = hashed;
        write_slot->duration = duration;
        write_slot->elapsed = 0.0;
        write_slot->loop = loop;
        write_slot->active = true;
        write_slot->paused = false;
        write_slot->use_real_time = use_real_time;
        write_slot->callback = callback;
        write_slot->sender = sender;
        write_slot->listener = listener;
        write_slot->user_data = user_data;
    }

    // Save hashed id value to ref's 'hashed_id'
    out_handle->hashed_id = hashed;

    return TIMER_SUCCESS;
}

KYRA_ENGINE_API TimerResult timer_destruct_timer(TimerHandle *handle) {
    if (!state) return TIMER_ERROR_NOT_INITIALISED;
    if (!handle) return TIMER_ERROR_TIMER_HANDLE_NULL;
    if (handle->hashed_id == INVALID_HASH) return TIMER_ERROR_TIMER_HANDLE_HASH_ID_INVALID;

    for (ByteSize index = 0; index < state->max_active_timers; ++index) {
        GameplayTimer *timer_slot = &state->timer_registry[index];

        if (timer_slot->active && timer_slot->hashed_id == handle->hashed_id) {
            // Found timer slot with matching ID...

            // Reset registry slot
            memset(timer_slot, 0, sizeof(GameplayTimer));

            // Invalidate handle
            handle->hashed_id = INVALID_HASH;

            return TIMER_SUCCESS;
        }
    }

    return TIMER_ERROR_FAILED_TO_LOCATE_TIMER_SLOT_FOR_HANDLE_HASHED_ID;
}

KYRA_ENGINE_API TimerResult timer_pause(const TimerHandle handle) {
    if (!state) return TIMER_ERROR_NOT_INITIALISED;
    if (handle.hashed_id == INVALID_HASH) return TIMER_ERROR_TIMER_HANDLE_HASH_ID_INVALID;

    for (ByteSize index = 0; index < state->max_active_timers; ++index) {
        GameplayTimer *timer_slot = &state->timer_registry[index];

        if (timer_slot->active && timer_slot->hashed_id == handle.hashed_id) {
            // Found timer slot with matching ID...
            
            // Pause the timer
            timer_slot->paused = true;

            return TIMER_SUCCESS;
        }
    }

    return TIMER_ERROR_FAILED_TO_LOCATE_TIMER_SLOT_FOR_HANDLE_HASHED_ID;
}

KYRA_ENGINE_API TimerResult timer_resume(const TimerHandle handle) {
    if (!state) return TIMER_ERROR_NOT_INITIALISED;
    if (handle.hashed_id == INVALID_HASH) return TIMER_ERROR_TIMER_HANDLE_HASH_ID_INVALID;

    for (ByteSize index = 0; index < state->max_active_timers; ++index) {
        GameplayTimer *timer_slot = &state->timer_registry[index];

        if (timer_slot->active && timer_slot->hashed_id == handle.hashed_id) {
            // Found timer slot with matching ID...
            
            // Resume the timer
            timer_slot->paused = false;

            return TIMER_SUCCESS;
        }
    }

    return TIMER_ERROR_FAILED_TO_LOCATE_TIMER_SLOT_FOR_HANDLE_HASHED_ID;
}

KYRA_ENGINE_API TimerResult timer_reset(const TimerHandle handle) {
    if (!state) return TIMER_ERROR_NOT_INITIALISED;
    if (handle.hashed_id == INVALID_HASH) return TIMER_ERROR_TIMER_HANDLE_HASH_ID_INVALID;

    for (ByteSize index = 0; index < state->max_active_timers; ++index) {
        GameplayTimer *timer_slot = &state->timer_registry[index];

        if (timer_slot->active && timer_slot->hashed_id == handle.hashed_id) {
            // Found timer slot with matching ID...
            
            // Reset elapsed time
            timer_slot->elapsed = 0.0;

            return TIMER_SUCCESS;
        }
    }

    return TIMER_ERROR_FAILED_TO_LOCATE_TIMER_SLOT_FOR_HANDLE_HASHED_ID;
}

KYRA_ENGINE_API TimerResult timer_get_remaining(const TimerHandle handle, Flt64 *out_remaining) {
    if (!state) return TIMER_ERROR_NOT_INITIALISED;
    if (handle.hashed_id == INVALID_HASH) return TIMER_ERROR_TIMER_HANDLE_HASH_ID_INVALID;

    for (ByteSize index = 0; index < state->max_active_timers; ++index) {
        GameplayTimer *timer_slot = &state->timer_registry[index];

        if (timer_slot->active && timer_slot->hashed_id == handle.hashed_id) {
            // Found timer slot with matching ID...
            
            // Calculate remaining time
            Flt64 remaining = timer_slot->duration - timer_slot->elapsed;

            // Save timer remaining time to ref
            if (out_remaining) *out_remaining = (remaining > 0.0f) ? remaining : 0.0;

            return TIMER_SUCCESS;
        }
    }

    return TIMER_ERROR_FAILED_TO_LOCATE_TIMER_SLOT_FOR_HANDLE_HASHED_ID;
}

KYRA_ENGINE_API HiResClock timer_get_clock(void) {
    if (!state) return (HiResClock) { 0 };

    return state->clock;
}

KYRA_ENGINE_API Flt64 timer_get_real_delta_time(void) {
    if (!state) return 0.0;
    
    return state->real_delta_time;
}

KYRA_ENGINE_API Flt64 timer_get_game_delta_time(void) {
    if (!state) return 0.0;
    
    return state->game_delta_time;
}

KYRA_ENGINE_API Flt64 timer_get_real_time_second(void) {
    if (!state) return 0.0;
    
    return state->real_time_seconds;
}

KYRA_ENGINE_API Flt64 timer_get_game_time_second(void) {
    if (!state) return 0.0;
    
    return state->game_time_seconds;
}

KYRA_ENGINE_API Flt32 timer_get_time_scale(void) {
    if (!state) return 0.0;
    
    return state->time_scale;
}

KYRA_ENGINE_API Bool timer_is_paused(void) {
    if (!state) return false;

    return state->paused;
}

KYRA_ENGINE_API ConstStr timer_result_to_string(const TimerResult result) {
    switch (result) {
        case TIMER_SUCCESS:                                                     return "TIMER_SUCCESS";

        case TIMER_ERROR_NOT_INITIALISED:                                       return "TIMER_ERROR_NOT_INITIALISED";
        case TIMER_ERROR_ALREADY_INITIALISED:                                   return "TIMER_ERROR_ALREADY_INITIALISED";
        case TIMER_ERROR_CONFIG_FILEPATH_NULL:                                  return "TIMER_ERROR_CONFIG_FILEPATH_NULL";
        case TIMER_ERROR_TIMER_ID_NULL:                                         return "TIMER_ERROR_TIMER_ID_NULL";
        case TIMER_ERROR_REF_OUT_HANDLE_NULL:                                   return "TIMER_ERROR_REF_OUT_HANDLE_NULL";
        case TIMER_ERROR_TIMER_HANDLE_NULL:                                     return "TIMER_ERROR_TIMER_HANDLE_NULL";
        case TIMER_ERROR_TIMER_HANDLE_HASH_ID_INVALID:                          return "TIMER_ERROR_TIMER_HANDLE_HASH_ID_INVALID";
        case TIMER_ERROR_TIMER_REGISTRY_FULL:                                   return "TIMER_ERROR_TIMER_REGISTRY_FULL";
        case TIMER_ERROR_FAILED_TO_OPEN_CONFIG_FILE:                            return "TIMER_ERROR_FAILED_TO_OPEN_CONFIG_FILE";
        case TIMER_ERROR_FAILED_TO_CLOSE_CONFIG_FILE:                           return "TIMER_ERROR_FAILED_TO_CLOSE_CONFIG_FILE";
        case TIMER_ERROR_FAILED_TO_GET_CONFIG_FILE_SIZE:                        return "TIMER_ERROR_FAILED_TO_GET_CONFIG_FILE_SIZE";
        case TIMER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_CONFIG_RAW_BUFFER:       return "TIMER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_CONFIG_RAW_BUFFER";
        case TIMER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_CONFIG_RAW_BUFFER:      return "TIMER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_CONFIG_RAW_BUFFER";
        case TIMER_ERROR_FAILED_TO_READ_CONFIG_FILE:                            return "TIMER_ERROR_FAILED_TO_READ_CONFIG_FILE";
        case TIMER_ERROR_FAILED_TO_PARSE_CONFIG_BUFFER_TO_JSON:                 return "TIMER_ERROR_FAILED_TO_PARSE_CONFIG_BUFFER_TO_JSON";
        case TIMER_ERROR_FAILED_TO_LOCATE_RUNTIME_SECTION_IN_CONFIG_JSON:       return "TIMER_ERROR_FAILED_TO_LOCATE_RUNTIME_SECTION_IN_CONFIG_JSON";
        case TIMER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE:                   return "TIMER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE";
        case TIMER_ERROR_FAILED_TO_DEALLOCCATE_MEMORY_OF_STATE:                 return "TIMER_ERROR_FAILED_TO_DEALLOCCATE_MEMORY_OF_STATE";
        case TIMER_ERROR_FAILED_TO_START_TIMER_CLOCK:                           return "TIMER_ERROR_FAILED_TO_START_TIMER_CLOCK";
        case TIMER_ERROR_FAILED_TO_LOCATE_TIMER_SLOT_FOR_HANDLE_HASHED_ID:      return "TIMER_ERROR_FAILED_TO_LOCATE_TIMER_SLOT_FOR_HANDLE_HASHED_ID";

        default:                                                                return "UNKNOWN_TIMER_RESULT";
    }
}


