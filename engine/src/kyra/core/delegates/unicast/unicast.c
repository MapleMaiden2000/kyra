#include "kyra/core/delegates/unicast/unicast.h"

#include <string.h>

#include "kyra/core/containers/array/array.h"
#include "kyra/core/containers/map/map.h"
#include "kyra/core/memory/zone/memory_zone.h"


// Internal state -------------------------------------------------- //

typedef struct Delegate_Unicast_State {
	Map				delegate_map;   // <ConstStr, UnicastDelegate>

    // For allocations/deallocations
    ByteSize        memory_size;

} UnicastDelegateState;

static UnicastDelegateState *state = NULL;


// API functions --------------------------------------------------- //

KYRA_ENGINE_API DelegateResult delegate_unicast_startup(void) {
    if (state != NULL) return DELEGATE_UNICAST_ERROR_ALREADY_INITIALISED;

    // Allocate state
    ByteSize mem_size = 0;
    if (memory_zone_allocate("delegates", sizeof(UnicastDelegateState), (VoidPtr *)&state, &mem_size) != MEMORY_ZONE_SUCCESS)
        return DELEGATE_UNICAST_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE;

    // Configurate state properties
    {
        // Delegate map
        if (container_map_construct(sizeof(UnicastDelegate), &state->delegate_map) != CONTAINER_SUCCESS) {
            // Failed to construct...
            
            // Deallocate state
            if (memory_zone_deallocate("delegates", (VoidPtr)state, mem_size) != MEMORY_ZONE_SUCCESS) 
                return DELEGATE_UNICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE;
            
            // Set to NULL
            state = NULL;

            return DELEGATE_UNICAST_ERROR_FAILED_TO_CONSTRUCT_DELEGATE_MAP;
        }

        // Memory size
        state->memory_size = mem_size;
    }

    return DELEGATE_SUCCESS;
}

KYRA_ENGINE_API DelegateResult delegate_unicast_shutdown(void) {
    if (!state) return DELEGATE_UNICAST_ERROR_NOT_INITIALISED;

    // Deallocate registered delegates
    {
        ByteSize capacity = container_map_capacity(state->delegate_map);
        for (ByteSize index = 0; index < capacity; ++index) {
            UnicastDelegate delegate = NULL;
            
            if (container_map_at_index(state->delegate_map, index, NULL, (VoidPtr)&delegate) == CONTAINER_SUCCESS) {
                // Delegate at this index is registered...
                
                if (delegate) {
                    // Deallocate delegate
                    if (memory_zone_deallocate("delegates", (VoidPtr)delegate, delegate->memory_size) != MEMORY_ZONE_SUCCESS)
                        return DELEGATE_UNICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_DELEGATE;
                }
            }
        }
    }

    // Destruct delegate map
    if (container_map_destruct(&state->delegate_map) != CONTAINER_SUCCESS)
        return DELEGATE_UNICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_MAP;

    // Deallocate state
    if (memory_zone_deallocate("delegates", (VoidPtr)state, state->memory_size) != MEMORY_ZONE_SUCCESS)
        return DELEGATE_UNICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE;

    // Set to NULL
    state = NULL;

    return DELEGATE_SUCCESS;
}

KYRA_ENGINE_API DelegateResult delegate_unicast_register(ConstStr id, const Listener listener, const DelegateFunction callback) {
    if (!state) return DELEGATE_UNICAST_ERROR_NOT_INITIALISED;
    if (!id) return DELEGATE_UNICAST_ERROR_DELEGATE_ID_NULL;
    if (!callback) return DELEGATE_UNICAST_ERROR_DELEGATE_CALLBACK_NULL;

    UnicastDelegate delegate = NULL;

    // Search for delegate
    ContainerResult search_result = container_map_search(state->delegate_map, id, (VoidPtr)&delegate);

    if (search_result == CONTAINER_SUCCESS) {
        // Delegate is already registered...
        
        if (delegate) {
            // Update delegate listener and callback
            delegate->listener = listener;
            delegate->callback = callback;
        }

        return DELEGATE_SUCCESS;
    }

    // Allocate for delegate
    ByteSize mem_size = 0;
    if (memory_zone_allocate("delegates", sizeof(struct Unicast_Delegate), (VoidPtr *)&delegate, &mem_size) != MEMORY_ZONE_SUCCESS)
        return DELEGATE_UNICAST_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_DELEGATE;
    
    // Configurate delegate properties
    {
        delegate->listener = listener;
        delegate->callback = callback;
        delegate->memory_size = mem_size;
    }

    // Register delegate to map
    if (container_map_insert(&state->delegate_map, id, (VoidPtr)&delegate) != CONTAINER_SUCCESS) {
        // Failed to do so...

        // Deallocate delegate
        if (memory_zone_deallocate("delegates", (VoidPtr)delegate, mem_size) != MEMORY_ZONE_SUCCESS)
            return DELEGATE_UNICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_DELEGATE;
    
        return DELEGATE_UNICAST_ERROR_FAILED_TO_REGISTER_DELEGATE;    
    }

    return DELEGATE_SUCCESS;
}

KYRA_ENGINE_API DelegateResult delegate_unicast_unregister(ConstStr id) {
    if (!state) return DELEGATE_UNICAST_ERROR_NOT_INITIALISED;
    if (!id) return DELEGATE_UNICAST_ERROR_DELEGATE_ID_NULL;

    UnicastDelegate delegate = NULL;

    // Search for delegate
    ContainerResult search_result = container_map_search(state->delegate_map, id, (VoidPtr)&delegate);
    if (search_result != CONTAINER_SUCCESS) return DELEGATE_UNICAST_ERROR_FAILED_TO_LOCATE_DELEGATE_FOR_ID;

    // Return if delegate is not registered
    if (!delegate) return DELEGATE_SUCCESS;

    // Un-register delegate from map
    if (container_map_remove(&state->delegate_map, id) != CONTAINER_SUCCESS)
        return DELEGATE_UNICAST_ERROR_FAILED_TO_UNREGISTER_DELEGATE;

    // Deallocate delegate
    if (memory_zone_deallocate("delegates", (VoidPtr)delegate, delegate->memory_size) != MEMORY_ZONE_SUCCESS)
        return DELEGATE_UNICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_DELEGATE;

    return DELEGATE_SUCCESS;
}

KYRA_ENGINE_API DelegateResult delegate_unicast_set_callback(ConstStr id, const DelegateFunction callback) {
    if (!state) return DELEGATE_UNICAST_ERROR_NOT_INITIALISED;
    if (!id) return DELEGATE_UNICAST_ERROR_DELEGATE_ID_NULL;
    if (!callback) return DELEGATE_UNICAST_ERROR_DELEGATE_CALLBACK_NULL;

    UnicastDelegate delegate = NULL;

    // Search for delegate
    ContainerResult search_result = container_map_search(state->delegate_map, id, (VoidPtr)&delegate);
    if (search_result != CONTAINER_SUCCESS) return DELEGATE_UNICAST_ERROR_FAILED_TO_LOCATE_DELEGATE_FOR_ID;

    // Return error if delegate is not registered
    if (!delegate) return DELEGATE_UNICAST_ERROR_DELEGATE_NOT_REGISTERED;

    // Set new callback
    delegate->callback = callback;

    return DELEGATE_SUCCESS;
}

KYRA_ENGINE_API DelegateResult delegate_unicast_invoke(ConstStr id, const Sender sender, const VoidPtr data) {
    if (!state) return DELEGATE_UNICAST_ERROR_NOT_INITIALISED;
    if (!id) return DELEGATE_UNICAST_ERROR_DELEGATE_ID_NULL;
    if (!sender) return DELEGATE_UNICAST_ERROR_DELEGATE_SENDER_NULL;

    UnicastDelegate delegate = NULL;

    // Search for delegate
    ContainerResult search_result = container_map_search(state->delegate_map, id, (VoidPtr)&delegate);
    if (search_result != CONTAINER_SUCCESS) return DELEGATE_UNICAST_ERROR_FAILED_TO_LOCATE_DELEGATE_FOR_ID;

    // Return error if delegate is not registered
    if (!delegate) return DELEGATE_UNICAST_ERROR_DELEGATE_NOT_REGISTERED;

    // Return error if delegate has no callback
    if (!delegate->callback) return DELEGATE_UNICAST_ERROR_DELEGATE_HAS_NO_CALLBACK;

    // Invoke delegate callback
    delegate->callback(sender, delegate->listener, data);

    return DELEGATE_SUCCESS;
}

KYRA_ENGINE_API ConstStr delegate_unicast_result_to_string(const DelegateResult result) {
    switch (result) {
        case DELEGATE_SUCCESS:                                                          return "DELEGATE_SUCCESS";

        case DELEGATE_UNICAST_ERROR_ALREADY_INITIALISED:                                return "DELEGATE_UNICAST_ERROR_ALREADY_INITIALISED";
        case DELEGATE_UNICAST_ERROR_NOT_INITIALISED:                                    return "DELEGATE_UNICAST_ERROR_NOT_INITIALISED";
        case DELEGATE_UNICAST_ERROR_DELEGATE_ID_NULL:                                   return "DELEGATE_UNICAST_ERROR_DELEGATE_ID_NULL";
        case DELEGATE_UNICAST_ERROR_DELEGATE_CALLBACK_NULL:                             return "DELEGATE_UNICAST_ERROR_DELEGATE_CALLBACK_NULL";
        case DELEGATE_UNICAST_ERROR_DELEGATE_NOT_REGISTERED:                            return "DELEGATE_UNICAST_ERROR_DELEGATE_NOT_REGISTERED";
        case DELEGATE_UNICAST_ERROR_DELEGATE_SENDER_NULL:                               return "DELEGATE_UNICAST_ERROR_DELEGATE_SENDER_NULL";
        case DELEGATE_UNICAST_ERROR_DELEGATE_HAS_NO_CALLBACK:                           return "DELEGATE_UNICAST_ERROR_DELEGATE_HAS_NO_CALLBACK";
        case DELEGATE_UNICAST_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE:                return "DELEGATE_UNICAST_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE";
        case DELEGATE_UNICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE:               return "DELEGATE_UNICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE";
        case DELEGATE_UNICAST_ERROR_FAILED_TO_CONSTRUCT_DELEGATE_MAP:                   return "DELEGATE_UNICAST_ERROR_FAILED_TO_CONSTRUCT_DELEGATE_MAP";
        case DELEGATE_UNICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_MAP:                    return "DELEGATE_UNICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_MAP";
        case DELEGATE_UNICAST_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_DELEGATE:             return "DELEGATE_UNICAST_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_DELEGATE";
        case DELEGATE_UNICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_DELEGATE:            return "DELEGATE_UNICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_DELEGATE";
        case DELEGATE_UNICAST_ERROR_FAILED_TO_REGISTER_DELEGATE:                        return "DELEGATE_UNICAST_ERROR_FAILED_TO_REGISTER_DELEGATE";
        case DELEGATE_UNICAST_ERROR_FAILED_TO_UNREGISTER_DELEGATE:                      return "DELEGATE_UNICAST_ERROR_FAILED_TO_UNREGISTER_DELEGATE";
        case DELEGATE_UNICAST_ERROR_FAILED_TO_LOCATE_DELEGATE_FOR_ID:                   return "DELEGATE_UNICAST_ERROR_FAILED_TO_LOCATE_DELEGATE_FOR_ID";
    
        default:                                                                        return "UNKNOWN_DELEGATE_RESULT";
    }
}

