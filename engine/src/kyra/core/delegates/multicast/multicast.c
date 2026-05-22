#include "kyra/core/delegates/multicast/multicast.h"

#include <string.h>

#include "kyra/core/containers/array/array.h"
#include "kyra/core/containers/map/map.h"
#include "kyra/core/memory/zone/memory_zone.h"


// Internal state -------------------------------------------------- //

typedef struct Delegate_Multicast_State {
	Map				delegate_map;       // <ConstStr, MulticastDelegate>
    Array           active_delegates;   // <MulticastDelegate>

    // For allocations/deallocations
    ByteSize        memory_size;

} MulticastDelegateState;

static MulticastDelegateState *state = NULL;


// API functions --------------------------------------------------- //

KYRA_ENGINE_API DelegateResult delegate_multicast_startup(void) {
    if (state != NULL) return DELEGATE_MULTICAST_ERROR_ALREADY_INITIALISED;
    
    // Allocate for state
    ByteSize mem_size = 0;
    if (memory_zone_allocate("delegates", sizeof(MulticastDelegateState), (VoidPtr *)&state, &mem_size) != MEMORY_ZONE_SUCCESS)
        return DELEGATE_MULTICAST_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE;

    // Configurate state properties
    {
        // Delegate map
        if (container_map_construct(sizeof(MulticastDelegate), &state->delegate_map) != CONTAINER_SUCCESS) {
            // Failed to construct...
            
            // Deallocate state
            if (memory_zone_deallocate("delegates", (VoidPtr)state, mem_size) != MEMORY_ZONE_SUCCESS) 
                return DELEGATE_MULTICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE;
            
            // Set to NULL
            state = NULL;

            return DELEGATE_MULTICAST_ERROR_FAILED_TO_CONSTRUCT_DELEGATE_MAP;
        }

        // List of active delegates
        // Used as 'book-keeping' (for faster delegate cleanup during shutdown)
        if (container_array_construct(sizeof(MulticastDelegate), &state->active_delegates) != CONTAINER_SUCCESS) {
            // Failed to construct...

            // Destruct delegate map
            if (container_map_destruct(&state->delegate_map) != CONTAINER_SUCCESS)
                return DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_MAP;

            // Deallocate state
            if (memory_zone_deallocate("delegates", (VoidPtr)state, mem_size) != MEMORY_ZONE_SUCCESS) 
                return DELEGATE_MULTICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE;
            
            // Set to NULL
            state = NULL;

            return DELEGATE_MULTICAST_ERROR_FAILED_TO_CONSTRUCT_ACTIVE_DELEGATES_ARRAY;
        }

        // Memory size
        state->memory_size = mem_size;
    }

    return DELEGATE_SUCCESS;
}

KYRA_ENGINE_API DelegateResult delegate_multicast_shutdown(void) {
    if (!state) return DELEGATE_MULTICAST_ERROR_NOT_INITIALISED;
    
    // Destruct all active delegates
    {
        ByteSize size = container_array_size(state->active_delegates);
        for (ByteSize index = 0; index < size; ++index) {
            MulticastDelegate delegate = *(MulticastDelegate *)container_array_get_at(state->active_delegates, index);

            // For every delegate...

            if (delegate) {
                // Destruct arrays of listeners and callbacks
                {
                    if (container_array_destruct(&delegate->listeners) != CONTAINER_SUCCESS)
                        return DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_LISTENERS_ARRAY;

                    if (container_array_destruct(&delegate->callbacks) != CONTAINER_SUCCESS)
                        return DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_CALLBACKS_ARRAY;
                }

                // Deallocate delegate
                if (memory_zone_deallocate("delegates", (VoidPtr)delegate, delegate->memory_size) != MEMORY_ZONE_SUCCESS)
                    return DELEGATE_MULTICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_DELEGATE;
            }
        }

        // Destruct active delegate array
        if (container_array_destruct(&state->active_delegates) != CONTAINER_SUCCESS)
            return DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_ACTIVE_DELEGATES_ARRAY;
    }

    // Destruct delegate map
    if (container_map_destruct(&state->delegate_map) != CONTAINER_SUCCESS)
        return DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_MAP;

    // Deallocate state
    if (memory_zone_deallocate("delegates", (VoidPtr)state, state->memory_size) != MEMORY_ZONE_SUCCESS)
        return DELEGATE_MULTICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE;

    // Set to NULL
    state = NULL;

    return DELEGATE_SUCCESS;
}

KYRA_ENGINE_API DelegateResult delegate_multicast_register(ConstStr id, const Listener listener, const DelegateFunction callback) {
    if (!state) return DELEGATE_MULTICAST_ERROR_NOT_INITIALISED;
    if (!id) return DELEGATE_MULTICAST_ERROR_DELEGATE_ID_NULL;
    if (!callback) return DELEGATE_MULTICAST_ERROR_DELEGATE_CALLBACK_NULL;
    
    MulticastDelegate delegate = NULL;

    ContainerResult search_result = container_map_search(state->delegate_map, id, (VoidPtr)&delegate);
    if (search_result != CONTAINER_SUCCESS) {
        // Failed to locate delegate, indicating delegate of id not registered...

        // Allocate for delegate
        ByteSize mem_size = 0;
        if (memory_zone_allocate("delegates", sizeof(struct Multicast_Delegate), (VoidPtr)&delegate, &mem_size) != MEMORY_ZONE_SUCCESS)
            return DELEGATE_MULTICAST_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_DELEGATE;

        // Construct arrays of listeners and callbacks 
        {
            ContainerResult listeners_result = container_array_construct(sizeof(Listener), &delegate->listeners); 
            ContainerResult callbacks_result = container_array_construct(sizeof(DelegateFunction), &delegate->callbacks);
            if (listeners_result != CONTAINER_SUCCESS || callbacks_result != CONTAINER_SUCCESS) {
                // Failed to construct either...
                
                // Deallocate delegate
                if (memory_zone_deallocate("delegates", (VoidPtr)delegate, mem_size) != MEMORY_ZONE_SUCCESS)
                    return DELEGATE_MULTICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_DELEGATE;
                
                return DELEGATE_MULTICAST_ERROR_FAILED_TO_CONSTRUCT_ARRAYS_FOR_DELEGATE;
            }
        }

        // Assign memory size and invoke depth
        delegate->memory_size = mem_size;
        delegate->invoke_depth = 0;

        // Register delegate to map
        {
            ContainerResult register_result = container_map_insert(&state->delegate_map, id, (VoidPtr)&delegate); 
            if (register_result != CONTAINER_SUCCESS) {
                // Failed to do so...

                // Destruct arrays of listeners and callbacks
                {
                    if (container_array_destruct(&delegate->listeners) != CONTAINER_SUCCESS)
                        return DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_LISTENERS_ARRAY;

                    if (container_array_destruct(&delegate->callbacks) != CONTAINER_SUCCESS)
                        return DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_CALLBACKS_ARRAY;
                }

                // Deallocate delegate
                if (memory_zone_deallocate("delegates", (VoidPtr)delegate, delegate->memory_size) != MEMORY_ZONE_SUCCESS)
                    return DELEGATE_MULTICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_DELEGATE;

                // Set to NULL
                delegate = NULL;

                return DELEGATE_MULTICAST_ERROR_FAILED_TO_REGISTER_DELEGATE;
            }
        }

        // Register to active delegates array
        container_array_push(&state->active_delegates, (VoidPtr)&delegate);
    }

    // Add listener and callback to delegate
    {
        container_array_push(&delegate->listeners, (VoidPtr)&listener);
        container_array_push(&delegate->callbacks, (VoidPtr)&callback);
    }

    return DELEGATE_SUCCESS;
}

KYRA_ENGINE_API DelegateResult delegate_multicast_unregister(ConstStr id, const Listener listener) {
    if (!state) return DELEGATE_MULTICAST_ERROR_NOT_INITIALISED;
    if (!id) return DELEGATE_MULTICAST_ERROR_DELEGATE_ID_NULL;
    if (!listener) return DELEGATE_MULTICAST_ERROR_DELEGATE_LISTENER_NULL;
    
    MulticastDelegate delegate = NULL;

    ContainerResult search_result = container_map_search(state->delegate_map, id, (VoidPtr)&delegate);
    if (search_result != CONTAINER_SUCCESS) return DELEGATE_MULTICAST_ERROR_FAILED_TO_LOCATE_DELEGATE_FOR_ID;

    // Find and remove listener
    {
        ByteSize size = container_array_size(delegate->listeners);
        for (ByteSize index = 0; index < size; ++index) {
            Listener ls = *(Listener *)container_array_get_at(delegate->listeners, index);

            // For every listener...
            
            // Check if matched
            if (ls == listener) {
                // Matched specified listener...

                if (delegate->invoke_depth > 0) {
                    DelegateFunction empty_callback =  NULL;
                    container_array_update_at(&delegate->callbacks, index, &empty_callback);
                }
                else {
                    // Remove from both arrays
                    container_array_remove_at(&delegate->listeners, index);
                    container_array_remove_at(&delegate->callbacks, index);

                    // If no listeners remain, clean up the entire delegate
                    ByteSize remain = container_array_size(delegate->listeners);
                    if (remain == 0) {
                        // Remove from active list
                        container_array_remove(&state->active_delegates, (VoidPtr)&delegate, false);

                        // Remove from map
                        container_map_remove(&state->delegate_map, id);

                        // Destruct arrays of listeners and callbacks
                        {
                            if (container_array_destruct(&delegate->listeners) != CONTAINER_SUCCESS)
                                return DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_LISTENERS_ARRAY;

                            if (container_array_destruct(&delegate->callbacks) != CONTAINER_SUCCESS)
                                return DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_CALLBACKS_ARRAY;
                        }

                        // Deallocate delegate
                        if (memory_zone_deallocate("delegates", (VoidPtr)delegate, delegate->memory_size) != MEMORY_ZONE_SUCCESS)
                            return DELEGATE_MULTICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_DELEGATE;
                    }
                }

                // Return after first match is done
                return DELEGATE_SUCCESS;
            }
        }
    }

    return DELEGATE_MULTICAST_ERROR_FAILED_TO_LOCATE_LISTENER_FOR_ID;
}

KYRA_ENGINE_API DelegateResult delegate_multicast_invoke(ConstStr id, const Sender sender, const VoidPtr data) {
    if (!state) return DELEGATE_MULTICAST_ERROR_NOT_INITIALISED;
    if (!id) return DELEGATE_MULTICAST_ERROR_DELEGATE_ID_NULL;
    if (!sender) return DELEGATE_MULTICAST_ERROR_DELEGATE_SENDER_NULL;

    MulticastDelegate delegate = NULL;

    ContainerResult search_result = container_map_search(state->delegate_map, id, (VoidPtr)&delegate);
    if (search_result != CONTAINER_SUCCESS) return DELEGATE_MULTICAST_ERROR_FAILED_TO_LOCATE_DELEGATE_FOR_ID;

    // Enter dispatch scope
    delegate->invoke_depth++;

    // Dispatched events
    ByteSize size = container_array_size(delegate->listeners);
    for (ByteSize index = 0; index < size; ++index) {
        // For every element...

        // Get listener and callback
        Listener listener = *(Listener *)container_array_get_at(delegate->listeners, index);
        DelegateFunction callback = *(DelegateFunction *)container_array_get_at(delegate->callbacks, index);

        // Invoke callback function
        if (callback) callback(sender, listener, data);
    }

    // Exit dispatched scope
    delegate->invoke_depth--;

    if (delegate->invoke_depth == 0) {
        // All nested dispatches are done...

        // Perform deferred cleanup pass
        ByteSize current_size = container_array_size(delegate->listeners);
        for (Int64 index = (Int64)current_size - 1; index >= 0; --index) {
            DelegateFunction callback = *(DelegateFunction *)container_array_get_at(delegate->callbacks, index);
            
            // For every callback...

            if (callback == NULL) {
                // Flagged as NULL...
                
                // Remove it from both arrays
                container_array_remove_at(&delegate->listeners, index);
                container_array_remove_at(&delegate->callbacks, index);
            }
        }

        // If the arrays are now empty, destroy the entire delegate
        if (container_array_size(delegate->listeners) == 0) {
            // Remove from active list
                container_array_remove(&state->active_delegates, (VoidPtr)&delegate, false);

                // Remove from map
                container_map_remove(&state->delegate_map, id);

                // Destruct arrays of listeners and callbacks
                {
                    if (container_array_destruct(&delegate->listeners) != CONTAINER_SUCCESS)
                        return DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_LISTENERS_ARRAY;

                    if (container_array_destruct(&delegate->callbacks) != CONTAINER_SUCCESS)
                        return DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_CALLBACKS_ARRAY;
                }

                // Deallocate delegate
                if (memory_zone_deallocate("delegates", (VoidPtr)delegate, delegate->memory_size) != MEMORY_ZONE_SUCCESS)
                    return DELEGATE_MULTICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_DELEGATE;
        }
    }

    return DELEGATE_SUCCESS;
}

KYRA_ENGINE_API ConstStr delegate_multicast_result_to_string(const DelegateResult result) {
    switch (result) {
        case DELEGATE_SUCCESS:                                                              return "DELEGATE_SUCCESS";

        case DELEGATE_MULTICAST_ERROR_ALREADY_INITIALISED:                                  return "DELEGATE_MULTICAST_ERROR_ALREADY_INITIALISED";
        case DELEGATE_MULTICAST_ERROR_NOT_INITIALISED:                                      return "DELEGATE_MULTICAST_ERROR_NOT_INITIALISED";
        case DELEGATE_MULTICAST_ERROR_DELEGATE_ID_NULL:                                     return "DELEGATE_MULTICAST_ERROR_DELEGATE_ID_NULL";
        case DELEGATE_MULTICAST_ERROR_DELEGATE_CALLBACK_NULL:                               return "DELEGATE_MULTICAST_ERROR_DELEGATE_CALLBACK_NULL";
        case DELEGATE_MULTICAST_ERROR_DELEGATE_LISTENER_NULL:                               return "DELEGATE_MULTICAST_ERROR_DELEGATE_LISTENER_NULL";
        case DELEGATE_MULTICAST_ERROR_DELEGATE_SENDER_NULL:                                 return "DELEGATE_MULTICAST_ERROR_DELEGATE_SENDER_NULL";
        case DELEGATE_MULTICAST_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE:                  return "DELEGATE_MULTICAST_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE";
        case DELEGATE_MULTICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE:                 return "DELEGATE_MULTICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE";
        case DELEGATE_MULTICAST_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_DELEGATE:               return "DELEGATE_MULTICAST_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_DELEGATE";
        case DELEGATE_MULTICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_DELEGATE:              return "DELEGATE_MULTICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_DELEGATE";
        case DELEGATE_MULTICAST_ERROR_FAILED_TO_CONSTRUCT_DELEGATE_MAP:                     return "DELEGATE_MULTICAST_ERROR_FAILED_TO_CONSTRUCT_DELEGATE_MAP";
        case DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_MAP:                      return "DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_MAP";
        case DELEGATE_MULTICAST_ERROR_FAILED_TO_CONSTRUCT_ACTIVE_DELEGATES_ARRAY:           return "DELEGATE_MULTICAST_ERROR_FAILED_TO_CONSTRUCT_ACTIVE_DELEGATES_ARRAY";
        case DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_ACTIVE_DELEGATES_ARRAY:            return "DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_ACTIVE_DELEGATES_ARRAY";
        case DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_LISTENERS_ARRAY:          return "DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_LISTENERS_ARRAY";
        case DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_CALLBACKS_ARRAY:          return "DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_CALLBACKS_ARRAY";
        case DELEGATE_MULTICAST_ERROR_FAILED_TO_CONSTRUCT_ARRAYS_FOR_DELEGATE:              return "DELEGATE_MULTICAST_ERROR_FAILED_TO_CONSTRUCT_ARRAYS_FOR_DELEGATE";
        case DELEGATE_MULTICAST_ERROR_FAILED_TO_REGISTER_DELEGATE:                          return "DELEGATE_MULTICAST_ERROR_FAILED_TO_REGISTER_DELEGATE";
        case DELEGATE_MULTICAST_ERROR_FAILED_TO_LOCATE_DELEGATE_FOR_ID:                     return "DELEGATE_MULTICAST_ERROR_FAILED_TO_LOCATE_DELEGATE_FOR_ID";
        case DELEGATE_MULTICAST_ERROR_FAILED_TO_LOCATE_LISTENER_FOR_ID:                     return "DELEGATE_MULTICAST_ERROR_FAILED_TO_LOCATE_LISTENER_FOR_ID";
    
        default:                                                                            return "UNKNOWN_DELEGATE_RESULT";
    }
}


