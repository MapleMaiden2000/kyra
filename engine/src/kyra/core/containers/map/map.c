#include "kyra/core/containers/map/map.h"

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kyra/core/memory/zone/memory_zone.h"
#include "kyra/core/containers/string/string.h"
#include "kyra/core/hash/hash.h"


// Constants ------------------------------------------------------- //

#define TOMBSTONE_HASHED_KEY            0xdeadbeefdeadbeefull // Tombstone key for deleted entries
#define MAX_PROBE_DISTANCE(capacity)    ((capacity * 3) / 5) // Probes 60% of map capacity to reduce collisions     


// Internal structures --------------------------------------------- //

typedef struct Container_Map_Data_Item {
    String      key;
    HashedID    hashed_key;
    UIntPtr     addr_value;
} MapDataItem;

struct Container_Map {
    VoidPtr     pool;
    
    ByteSize    data_size;
    ByteSize    size;
    ByteSize    capacity;
    
    // For allocations/deallocations
    ByteSize    memory_size; 
};


// Helper functions ------------------------------------------------ //

// Performs secondary hash for double hashing to ensure non-zero return and good distribution
static KYRA_INLINE HashedID _container_map_secondary_hash(HashedID primary_hash, ByteSize capacity) {
    // For power-of-two capacity, h2 must be odd to visit all slots.
    return (primary_hash | 1);
}

// Rehashes an old slot into the map
static ContainerResult _container_map_rehash_insert(Map *map, const MapDataItem *old_slot) {
    if (!map || !(*map)) return CONTAINER_MAP_HELPER_ERROR_REF_MAP_NULL;
    if (!old_slot) return CONTAINER_MAP_HELPER_ERROR_OLD_SLOT_NULL;
    
    ByteSize capacity = (*map)->capacity;
    HashedID h1 = old_slot->hashed_key; // primary hash
    HashedID h2 = _container_map_secondary_hash(h1, capacity); // secondary hash
    ByteSize probe = 1;

    // Get initial index
    ByteSize index = h1 % capacity;

    // Get initial slot
    ByteSize item_size = sizeof(MapDataItem) + (*map)->data_size;
    MapDataItem *check_slot = (MapDataItem *)((BytePtr)(*map)->pool + (index * item_size));

    while (check_slot->hashed_key != 0 && check_slot->hashed_key != TOMBSTONE_HASHED_KEY) {
        // 'check_slot' is either empty or a tombstone...
        
        // Probe and get index
        index = (h1 + (probe * h2)) % capacity;

        // Get slot
        check_slot = (MapDataItem *)((BytePtr)(*map)->pool + (index * item_size));

        // Impcrement probe count
        ++probe;
    }

    // Get hashed key and address for the value
    check_slot->hashed_key = old_slot->hashed_key;
    check_slot->addr_value = (UIntPtr)((BytePtr)check_slot + sizeof(MapDataItem));

    // Copy over old slot key
    if (container_string_construct(container_string_get_cstr(old_slot->key), &check_slot->key) != CONTAINER_SUCCESS) {
        // Failed...

        return CONTAINER_MAP_HELPER_ERROR_FAILED_TO_COPY_OLD_SLOT_KEY;
    }

    // Copy over old slot value
    memcpy((VoidPtr)check_slot->addr_value, (VoidPtr)old_slot->addr_value, (*map)->data_size);
    
    return CONTAINER_SUCCESS;
}

static ContainerResult _container_map_resize(Map *map, const ByteSize new_capacity) {
    if (!map || !(*map)) return CONTAINER_MAP_HELPER_ERROR_REF_MAP_NULL;
    if (new_capacity < (*map)->size) return CONTAINER_MAP_HELPER_ERROR_NEW_CAPACITY_SHORTER_THAN_MAP_SIZE;

    Map old_map = *map;
    Map new_map = NULL;

    // Calculate allocation size
    ByteSize item_size = sizeof(MapDataItem) + old_map->data_size;
    ByteSize pool_size = item_size * new_capacity;
    ByteSize alloc_size = KYRA_APPLY_MEMORY_ALIGNMENT(sizeof(struct Container_Map) + pool_size, KYRA_MEMORY_ALIGNMENT_SIZE);
    ByteSize mem_size = 0;

    // Allocate new map
    if (memory_zone_allocate("containers", alloc_size, (VoidPtr *)&new_map, &mem_size) != MEMORY_ZONE_SUCCESS)
        return CONTAINER_MAP_HELPER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_NEW_MAP;

    // Set new map properties
    {
        new_map->data_size = old_map->data_size;
        new_map->size = 0; // Will be incremented during rehash...
        new_map->capacity = new_capacity;
        new_map->memory_size = mem_size;
        new_map->pool = (VoidPtr)KYRA_APPLY_MEMORY_ALIGNMENT((UIntPtr)new_map + sizeof(struct Container_Map), KYRA_MEMORY_ALIGNMENT_SIZE);
    }

    // Initialise new pool
    for (ByteSize i = 0; i < new_map->capacity; ++i) {
        MapDataItem *slot = (MapDataItem *)((BytePtr)new_map->pool + (i * item_size));
        slot->hashed_key = 0;
    }

    // Rehash all old slots
    for (ByteSize index = 0; index < old_map->capacity; ++index) {
        MapDataItem *old_slot = (MapDataItem *)((BytePtr)old_map->pool + (index * item_size));

        // For every old slot...

        // Check if occupied
        if (old_slot->hashed_key != TOMBSTONE_HASHED_KEY && old_slot->hashed_key != 0) {
            // Occupied...

            // Perform rehashing and insertion 
            if (_container_map_rehash_insert(&new_map, old_slot) != CONTAINER_SUCCESS) {
                // Failed...

                // Deallocate new map
                if (memory_zone_deallocate("containers", new_map, new_map->memory_size) != MEMORY_ZONE_SUCCESS)
                    return CONTAINER_MAP_HELPER_ERROR_FAILED_TO_DEALLOCATE_NEW_MAP;

                return CONTAINER_MAP_HELPER_ERROR_REHASH_INSERT_FAILED;
            }

            // Increment new map size
            new_map->size++;

            // Destroy old slot key
            container_string_destruct(&old_slot->key);
        }
    }

    // Deallocate memory of old map
    if (memory_zone_deallocate("containers", old_map, old_map->memory_size) != MEMORY_ZONE_SUCCESS)
        return CONTAINER_MAP_HELPER_ERROR_FAILED_TO_DEALLOCATE_OLD_MAP;

    // Save new map to ref
    *map = new_map;

    return  CONTAINER_SUCCESS;
}


// API functions --------------------------------------------------- //

KYRA_ENGINE_API ContainerResult container_map_construct(const ByteSize data_size, Map *out_map) {
    if (data_size == 0) return CONTAINER_MAP_ERROR_DATA_SIZE_ZERO;
    if (!out_map) return CONTAINER_MAP_ERROR_REF_OUT_MAP_NULL;

    // Calculate allocation size
    ByteSize item_size = sizeof(MapDataItem) + data_size;
    ByteSize pool_size = item_size * KYRA_CONTAINER_DEFAULT_CAPACITY;
    ByteSize alloc_size = KYRA_APPLY_MEMORY_ALIGNMENT(sizeof(struct Container_Map) + pool_size, KYRA_MEMORY_ALIGNMENT_SIZE);
    ByteSize mem_size = 0;

    // Allocate map
    if (memory_zone_allocate("containers", alloc_size, (VoidPtr *)out_map, &mem_size) != MEMORY_ZONE_SUCCESS)
        return CONTAINER_MAP_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_NEW_MAP;

    // Initialise map properties
    {
        (*out_map)->data_size = data_size;
        (*out_map)->size = 0;
        (*out_map)->capacity = KYRA_CONTAINER_DEFAULT_CAPACITY;
        (*out_map)->memory_size = mem_size;
        (*out_map)->pool = (VoidPtr)KYRA_APPLY_MEMORY_ALIGNMENT((UIntPtr)(*out_map) + sizeof(struct Container_Map), KYRA_MEMORY_ALIGNMENT_SIZE);
    }

    // Set all slots to empty
    for (ByteSize index = 0; index < (*out_map)->capacity; ++index) {
        MapDataItem *slot = (MapDataItem *)((BytePtr)(*out_map)->pool + (index * item_size));
        slot->hashed_key = 0;
    }

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_map_destruct(Map *map) {
    if (!map || !(*map)) return CONTAINER_MAP_ERROR_REF_MAP_NULL;

    ByteSize item_size = sizeof(MapDataItem) + (*map)->data_size;

    // Destroy all keys
    for (ByteSize index = 0; index < (*map)->capacity; ++index) {
        MapDataItem *slot = (MapDataItem *)((BytePtr)(*map)->pool + (index * item_size));
        
        // For every slot...

        // Check if slot is occupied
        if (slot->hashed_key != TOMBSTONE_HASHED_KEY && slot->hashed_key != 0) {
            // Slot is occupied...

            // Destroy slot key
            container_string_destruct(&slot->key);
        }
    }

    // Deallocate map
    if (memory_zone_deallocate("containers", (*map), (*map)->memory_size) != MEMORY_ZONE_SUCCESS) 
        return CONTAINER_MAP_ERROR_FAILED_TO_DEALLOCATE_MAP;

    // Set to NULL
    (*map) = NULL;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_map_insert(Map *map, ConstStr key, const VoidPtr value) {
    if (!map || !(*map)) return CONTAINER_MAP_ERROR_REF_MAP_NULL;
    if (!key) return CONTAINER_MAP_ERROR_KEY_NULL;
    if (!value) return CONTAINER_MAP_ERROR_VALUE_NULL;

    // Handle resizing when map is 75% full
    if ((*map)->size >= ((*map)->capacity * 3) / 4) {
        ByteSize new_capacity = (ByteSize)((Flt32)(*map)->capacity * KYRA_CONTAINER_RESIZE_RATIO);
        
        ContainerResult resize_result = _container_map_resize(map, new_capacity);
        if (resize_result != CONTAINER_SUCCESS) return resize_result;
    }

    // Get map properties
    ByteSize capacity = (*map)->capacity;
    ByteSize item_size = sizeof(MapDataItem) + (*map)->data_size;

    // Perform primary hash
    HashedID h1 = hash_str(key, HASH_MODE_XXH3);
    
    // Perform secondary hash
    HashedID h2 = _container_map_secondary_hash(h1, capacity);

    // Get initial index
    ByteSize index = h1 % capacity;
    
    // Get initial slot
    MapDataItem *check_slot = (MapDataItem *)((BytePtr)(*map)->pool + (index * item_size));

    // Probe for an open slot
    ByteSize probe = 1;
    ByteSize probe_distance = 0;
    while (check_slot->hashed_key != 0 && check_slot->hashed_key != TOMBSTONE_HASHED_KEY) {
        // If key already exists, consider this an update
        if (check_slot->hashed_key == h1) return container_map_update(map, key, value);
    
        // Otherwise...

        // Perform double-hashing
        index = (h1 + (probe * h2)) % capacity;
        check_slot = (MapDataItem *)((BytePtr)(*map)->pool + (index * item_size));

        if (++probe_distance > MAX_PROBE_DISTANCE(capacity)) {
            // Linear probing fallback
            Bool found = false;
            for (ByteSize i = 0; i < capacity; ++i) {
                MapDataItem *slot = (MapDataItem *)((BytePtr)(*map)->pool + (i * item_size));
                if (slot->hashed_key == TOMBSTONE_HASHED_KEY || slot->hashed_key == 0) {
                    // Found an open slot...
                    
                    // Update index and slot
                    index = i;
                    check_slot = slot;
                    
                    // Set as found
                    found = true;
                    break;
                }
            }

            // If not open slot was found, consider probing reached limit
            if (!found) return CONTAINER_MAP_ERROR_REACHED_PROBING_LIMIT;
            break;
        }

        ++probe;
    }

    // With open slot, insert key and value
    {
        Bool is_tombstone = (check_slot->hashed_key == TOMBSTONE_HASHED_KEY);
        
        // Construct slot key
        ContainerResult key_result = container_string_construct(key, &check_slot->key);
        if (key_result != CONTAINER_SUCCESS) return CONTAINER_MAP_ERROR_FAILED_TO_CONSTRUCT_SLOT_KEY;
        
        // Assign hashed key
        check_slot->hashed_key = h1;

        // Assign address for value
        check_slot->addr_value = (UIntPtr)((BytePtr)check_slot + sizeof(MapDataItem));
        
        // Copy value
        memcpy((VoidPtr)check_slot->addr_value, value, (*map)->data_size);
        
        if (!is_tombstone) (*map)->size++;
    }

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_map_remove(Map *map, ConstStr key) {
    if (!map || !(*map)) return CONTAINER_MAP_ERROR_REF_MAP_NULL;
    if (!key) return CONTAINER_MAP_ERROR_KEY_NULL;

    ByteSize capacity = (*map)->capacity;
    ByteSize item_size = sizeof(MapDataItem) + (*map)->data_size;

    // Perform primary hash
    HashedID h1 = hash_str(key, HASH_MODE_XXH3);
    
    // Perform secondary hash
    HashedID h2 = _container_map_secondary_hash(h1, capacity);

    // Get initial index
    ByteSize index = h1 % capacity;
    
    // Get initial slot
    MapDataItem *check_slot = (MapDataItem *)((BytePtr)(*map)->pool + (index * item_size));

    // Probe for an open slot
    ByteSize probe = 1;
    ByteSize probe_distance = 0;
    while (check_slot->hashed_key != 0) {
        if (check_slot->hashed_key == h1) {
            // Found matching slot...

            // Destroy slot key
            container_string_destruct(&check_slot->key);

            // Mark as tombstone
            check_slot->hashed_key = TOMBSTONE_HASHED_KEY;
            check_slot->addr_value = (UIntPtr)NULL;

            // Decrement map size
            (*map)->size--;

            return CONTAINER_SUCCESS;
        }

        // Otherwise, if found no matching slot...

        // Perform double-hashing for next probe
        index = (h1 + (probe * h2)) % capacity;
        check_slot = (MapDataItem *)((BytePtr)(*map)->pool + (index * item_size));

        if (++probe_distance > MAX_PROBE_DISTANCE(capacity)) return CONTAINER_MAP_ERROR_REACHED_PROBING_LIMIT;

        // Increment probe count
        ++probe;
    }

    return CONTAINER_MAP_ERROR_FAILED_TO_LOCATE_SLOT_FOR_KEY;
}

KYRA_ENGINE_API ContainerResult container_map_update(Map *map, ConstStr key, const VoidPtr new_value) {
    if (!map || !(*map)) return CONTAINER_MAP_ERROR_REF_MAP_NULL;
    if (!key) return CONTAINER_MAP_ERROR_KEY_NULL;
    if (!new_value) return CONTAINER_MAP_ERROR_NEW_VALUE_NULL;

    ByteSize capacity = (*map)->capacity;
    ByteSize item_size = sizeof(MapDataItem) + (*map)->data_size;

    // Perform primary hash
    HashedID h1 = hash_str(key, HASH_MODE_XXH3);
    
    // Perform secondary hash
    HashedID h2 = _container_map_secondary_hash(h1, capacity);

    // Get initial index
    ByteSize index = h1 % capacity;
    
    // Get initial slot
    MapDataItem *check_slot = (MapDataItem *)((BytePtr)(*map)->pool + (index * item_size));

    // Probe for an open slot
    ByteSize probe = 1;
    ByteSize probe_distance = 0;
    while (check_slot->hashed_key != 0) {
        if (check_slot->hashed_key == h1) {
            // Found matching slot...

            // Copy new value over to slot
            memcpy((VoidPtr)check_slot->addr_value, new_value, (*map)->data_size);

            return CONTAINER_SUCCESS;
        }

        // Otherwise, if found no matching slot...

        // Perform double-hashing for next probe
        index = (h1 + (probe * h2)) % capacity;
        check_slot = (MapDataItem *)((BytePtr)(*map)->pool + (index * item_size));
        
        if (++probe_distance > MAX_PROBE_DISTANCE(capacity)) return CONTAINER_MAP_ERROR_REACHED_PROBING_LIMIT;

        // Increment probe count
        ++probe;
    }

    return CONTAINER_MAP_ERROR_FAILED_TO_LOCATE_SLOT_FOR_KEY;
}

KYRA_ENGINE_API ContainerResult container_map_clear(Map *map) {
    if (!map || !(*map)) return CONTAINER_MAP_ERROR_REF_MAP_NULL;

    // Iterate over all slots
    for (ByteSize index = 0; index < (*map)->capacity; ++index) {
        MapDataItem *slot = (MapDataItem *)((BytePtr)(*map)->pool + (index * (*map)->data_size));

        // For every slot...

        // Check if slot is occupied
        if (slot->hashed_key != TOMBSTONE_HASHED_KEY && slot->hashed_key != 0) {
            // Occupied...

            // Destroy slot key
            container_string_destruct(&slot->key);
        }

        // Mark as empty
        slot->hashed_key = 0;
        slot->addr_value = (UIntPtr)NULL;
    }

    // Reset map size
    (*map)->size = 0;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_map_search(const Map map, ConstStr key, VoidPtr out_value) {
    if (!map) return CONTAINER_MAP_ERROR_MAP_NULL;
    if (!key) return CONTAINER_MAP_ERROR_KEY_NULL;

    ByteSize capacity = map->capacity;
    ByteSize item_size = sizeof(MapDataItem) + map->data_size;

    // Perform primary hash
    HashedID h1 = hash_str(key, HASH_MODE_XXH3);
    
    // Perform secondary hash
    HashedID h2 = _container_map_secondary_hash(h1, capacity);

    // Get initial index
    ByteSize index = h1 % capacity;
    
    // Get initial slot
    MapDataItem *check_slot = (MapDataItem *)((BytePtr)map->pool + (index * item_size));

    // Probe for an open slot
    ByteSize probe = 1;
    ByteSize probe_distance = 0;
    while (check_slot->hashed_key != 0) {
        if (check_slot->hashed_key == h1) {
            // Found matching slot...

            // Save slot value to ref
            if (out_value) memcpy(out_value, (VoidPtr)check_slot->addr_value, map->data_size);

            return CONTAINER_SUCCESS;
        }

        // Otherwise, if found no matching slot...

        // Perform double-hashing for next probe
        index = (h1 + (probe * h2)) % capacity;
        check_slot = (MapDataItem *)((BytePtr)map->pool + (index * item_size));
        
        if (++probe_distance > MAX_PROBE_DISTANCE(capacity)) return CONTAINER_MAP_ERROR_REACHED_PROBING_LIMIT;

        // Increment probe count
        ++probe;
    }

    return CONTAINER_MAP_ERROR_FAILED_TO_LOCATE_SLOT_FOR_KEY;
}

KYRA_ENGINE_API ContainerResult container_map_at_index(const Map map, const ByteSize index, String *out_key, VoidPtr out_value) {
    if (!map) return CONTAINER_MAP_ERROR_MAP_NULL;
    if (index >= map->capacity) return CONTAINER_MAP_ERROR_INDEX_OUT_OF_BOUNDS;

    // Get slot for specified index
    ByteSize item_size = sizeof(MapDataItem) + map->data_size;
    MapDataItem *slot = (MapDataItem *)((BytePtr)map->pool + (index * item_size));

    if (slot->hashed_key == 0 || slot->hashed_key == TOMBSTONE_HASHED_KEY) {
        // Slot is empty or tombstone...
        
        return CONTAINER_MAP_ERROR_FAILED_TO_LOCATE_SLOT_FOR_INDEX;
    }

    // Save to refs
    if (out_key) *out_key = slot->key;
    if (out_value) memcpy(out_value, (VoidPtr)slot->addr_value, map->data_size);

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API Bool container_map_contains(const Map map, ConstStr key) {
    if (!map || !key) return false;

    return (container_map_search(map, key, NULL) == CONTAINER_SUCCESS);
}

KYRA_ENGINE_API Bool container_map_is_empty(const Map map) {
    if (!map) return true;
    
    return (map->size == 0);
}

KYRA_ENGINE_API ByteSize container_map_get_data_size(const Map map) {
    if (!map) return 0;

    return map->data_size;
}

KYRA_ENGINE_API ByteSize container_map_get_size(const Map map) {
    if (!map) return 0;

    return map->size;
}

KYRA_ENGINE_API ByteSize container_map_get_capacity(const Map map) {
    if (!map) return 0;

    return map->capacity;
}

KYRA_ENGINE_API ConstStr container_map_result_to_string(const ContainerResult result) {

}







