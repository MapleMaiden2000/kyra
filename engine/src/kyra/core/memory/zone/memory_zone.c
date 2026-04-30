#include "kyra/core/memory/zone/memory_zone.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "kyra/core/memory/manager/memory_manager.h"


// Helper functions --------------------------------------------------------- //

// Computes log2 of a size to identify the closest size class
static KYRA_INLINE UInt8 _memory_zone_log2_size(const ByteSize size) {
    ByteSize log2 = 0, val = size;
    while (val >>= 1) ++log2;
    return log2;
}

static MemoryZoneResult _memory_zone_size_class_index(const MemoryZone *zone, const ByteSize size, ByteSize *out_index) {
    if (!zone) return MEMORY_ZONE_HELPER_ERROR_ZONE_NULL;
    if (size == 0) return MEMORY_ZONE_HELPER_ERROR_SIZE_ZERO;

    // Align the size
    ByteSize aligned_size = KYRA_APPLY_MEMORY_ALIGNMENT(size, KYRA_MEMORY_ALIGNMENT_SIZE);

    // Locate the index of the size class (via log2)
    const Flt32 log2_ratio = 0.694f;    // log2(1.618) approx
    const ByteSize log2_size = _memory_zone_log2_size(aligned_size);
    ByteSize index = (ByteSize)((Flt32)log2_size / log2_ratio);

    // If aligned size < current size class
    // Adjust the index until aligned size > current size class
    if (aligned_size < zone->size_classes[index].size)
        while (index > 0 && aligned_size < zone->size_classes[index].size) --index;

    // Adjust the index if aligned size > size class 
    if (aligned_size > zone->size_classes[index].size) ++index;

    // Clamp the index to valid range
    index = (index >= zone->num_classes) ? zone->num_classes - 1 : index;
    
    // Save to ref
    if (out_index) *out_index = index;

    return MEMORY_ZONE_SUCCESS;
}


// API functions ------------------------------------------------------------ //

KYRA_ENGINE_API MemoryZoneResult memory_zone_get(ConstStr name, MemoryZone **out_zone) {
    if (!name) return MEMORY_ZONE_ERROR_ZONE_NAME_NULL;

    // Get memory manager
    MemoryManager *manager = NULL;
    {
        MemoryManagerResult get_result = memory_manager_get(&manager);
        if (get_result != MEMORY_MANAGER_SUCCESS) {
            printf("Error: Failed to get memory manager (Error: %s).\n", memory_manager_result_to_string(get_result));
            return MEMORY_ZONE_ERROR_FAILED_TO_GET_MEMORY_MANAGER;
        }
    }

    for (ByteSize index = 0; index < manager->zone_count; ++index) {
        // For each memory zone...
        if (!strcmp(name, manager->zones[index].name)) {
            // Memory zone name matched...


            // Save to ref
            if (out_zone) *out_zone = &manager->zones[index];

            return MEMORY_ZONE_SUCCESS;
        }
    }
    
    return MEMORY_ZONE_ERROR_FAILED_TO_LOCATE_ZONE;
}

KYRA_ENGINE_API MemoryZoneResult memory_zone_clear(ConstStr name) {
    if (!name) return MEMORY_ZONE_ERROR_ZONE_NAME_NULL;

    // Get memory manager
    MemoryManager *manager = NULL;
    {
        MemoryManagerResult get_result = memory_manager_get(&manager);
        if (get_result != MEMORY_MANAGER_SUCCESS) {
            printf("Error: Failed to get memory manager (Error: %s).\n", memory_manager_result_to_string(get_result));
            return MEMORY_ZONE_ERROR_FAILED_TO_GET_MEMORY_MANAGER;
        }
    }

    // Get memory zone
    MemoryZone *zone = NULL;
    {
        MemoryZoneResult get_result = memory_zone_get(name, &zone);
        if (get_result != MEMORY_ZONE_SUCCESS) return get_result;
    }

    for (ByteSize index = 0; index < zone->num_classes; ++index) {
        // For each size class...

        // Reset 'free_list_head'
        zone->size_classes[index].free_list_head = NULL;
    }
    
    // Update 'used_memory' for manager
    manager->used_memory -= zone->used_memory;

    // Reset 'used memory' for zone
    zone->used_memory = 0;

    return MEMORY_ZONE_SUCCESS;
}

KYRA_ENGINE_API MemoryZoneResult memory_zone_allocate(ConstStr name, const ByteSize size, VoidPtr *out_addr, ByteSize *out_alloc_size) {
    if (!name) return MEMORY_ZONE_ERROR_ZONE_NAME_NULL;
    if (size == 0) return MEMORY_ZONE_ERROR_SIZE_ZERO;

    // Get memory manager
    MemoryManager *manager = NULL;
    {
        MemoryManagerResult get_result = memory_manager_get(&manager);
        if (get_result != MEMORY_MANAGER_SUCCESS) {
            printf("Error: Failed to get memory manager (Error: %s).\n", memory_manager_result_to_string(get_result));
            return MEMORY_ZONE_ERROR_FAILED_TO_GET_MEMORY_MANAGER;
        }
    }

    // Get memory zone
    MemoryZone *zone = NULL;
    {
        MemoryZoneResult get_result = memory_zone_get(name, &zone);
        if (get_result != MEMORY_ZONE_SUCCESS) return get_result;
    }

    // Locate size class
    ByteSize index = 0;
    MemoryZoneResult index_result = _memory_zone_size_class_index(zone, size, &index);
    if (index_result != MEMORY_ZONE_SUCCESS) {
        printf("Error: Failed to get size class index (Error: %s).\n", memory_manager_result_to_string(index_result));
        return MEMORY_ZONE_ERROR_FAILED_TO_GET_SIZE_CLASS_INDEX;
    }
    MemoryZoneSizeClass *size_class = &zone->size_classes[index];

    // Allocate from stack of free blocks if available
    if (size_class->free_list_head != NULL) {
        // Get the free block, aka. head of the free list for this size class 
        VoidPtr free_block = size_class->free_list_head;

        // Read the pointer stored inside that block to find the next free block
        VoidPtr *next_free_node = (VoidPtr *)free_block;

        // Update the head to point to the next block
        size_class->free_list_head = *next_free_node;

        // Save to refs
        if (out_addr)           *out_addr = free_block;
        if (out_alloc_size)     *out_alloc_size = size_class->size;

        // Update the 'used_memory' for zone and manager
        zone->used_memory += size_class->size;
        manager->used_memory += size_class->size;

        return MEMORY_ZONE_SUCCESS;
    }

    // Check if there is enough memory to allocate
    if (zone->used_memory + size_class->size > zone->capacity) return MEMORY_ZONE_ERROR_INSUFFICIENT_MEMORY_TO_ALLOCATE;

    // Allocate from memory pool
    if (out_addr)           *out_addr = (VoidPtr)(zone->addr_start + zone->used_memory);
    if (out_alloc_size)     *out_alloc_size = size_class->size;

    // Update 'used memory' for zone and manager
    zone->used_memory += size_class->size;
    manager->used_memory += size_class->size;

    return MEMORY_ZONE_SUCCESS;
}

KYRA_ENGINE_API MemoryZoneResult memory_zone_deallocate(ConstStr name, const VoidPtr addr, const ByteSize size) {
    if (!name) return MEMORY_ZONE_ERROR_ZONE_NAME_NULL;
    if (!addr) return MEMORY_ZONE_ERROR_INVALID_ADDRESS;
    if (size == 0) return MEMORY_ZONE_ERROR_SIZE_ZERO;

    const Flt32 ratio = 1.618f;

    // Get memory manager
    MemoryManager *manager = NULL;
    {
        MemoryManagerResult get_result = memory_manager_get(&manager);
        if (get_result != MEMORY_MANAGER_SUCCESS) {
            printf("Error: Failed to get memory manager (Error: %s).\n", memory_manager_result_to_string(get_result));
            return MEMORY_ZONE_ERROR_FAILED_TO_GET_MEMORY_MANAGER;
        }
    }

    // Get memory zone
    MemoryZone *zone = NULL;
    {
        MemoryZoneResult get_result = memory_zone_get(name, &zone);
        if (get_result != MEMORY_ZONE_SUCCESS) return get_result;
    }

    // Locate size class
    ByteSize index = 0;
    MemoryZoneResult index_result = _memory_zone_size_class_index(zone, size, &index);
    if (index_result != MEMORY_ZONE_SUCCESS) {
        printf("Error: Failed to get size class index (Error: %s).\n", memory_manager_result_to_string(index_result));
        return MEMORY_ZONE_ERROR_FAILED_TO_GET_SIZE_CLASS_INDEX;
    }
    MemoryZoneSizeClass *size_class = &zone->size_classes[index];

    // Make freed address to be pointer of itself as the 'freed address' node
    VoidPtr *next_free_node = (VoidPtr *)addr;

    // Write the current free list head directly into the freed block memory 
    *next_free_node = size_class->free_list_head;

    // The freed address is now new head of the free list
    size_class->free_list_head = addr;

    // Update 'used memory' for both zone and manager
    zone->used_memory -= size_class->size;
    manager->used_memory -= size_class->size;

    return MEMORY_ZONE_SUCCESS;
}

KYRA_ENGINE_API ConstStr memory_zone_result_to_string(const MemoryZoneResult result) {
    switch (result) {
        case MEMORY_ZONE_SUCCESS:                                       return "MEMORY_ZONE_SUCCESS";

        case MEMORY_ZONE_ERROR_ZONE_NAME_NULL:                          return "MEMORY_ZONE_ERROR_ZONE_NAME_NULL";
        case MEMORY_ZONE_ERROR_SIZE_ZERO:                               return "MEMORY_ZONE_ERROR_SIZE_ZERO";
        case MEMORY_ZONE_ERROR_INVALID_ADDRESS:                         return "MEMORY_ZONE_ERROR_INVALID_ADDRESS";
        case MEMORY_ZONE_ERROR_FAILED_TO_GET_MEMORY_MANAGER:            return "MEMORY_ZONE_ERROR_FAILED_TO_GET_MEMORY_MANAGER";
        case MEMORY_ZONE_ERROR_FAILED_TO_LOCATE_ZONE:                   return "MEMORY_ZONE_ERROR_FAILED_TO_LOCATE_ZONE";
        case MEMORY_ZONE_ERROR_FAILED_TO_GET_SIZE_CLASS_INDEX:          return "MEMORY_ZONE_ERROR_FAILED_TO_GET_SIZE_CLASS_INDEX";
        case MEMORY_ZONE_ERROR_FAILED_TO_RESIZE_SIZE_CLASS:             return "MEMORY_ZONE_ERROR_FAILED_TO_RESIZE_SIZE_CLASS";
        case MEMORY_ZONE_ERROR_INSUFFICIENT_MEMORY_TO_ALLOCATE:         return "MEMORY_ZONE_ERROR_INSUFFICIENT_MEMORY_TO_ALLOCATE";
        
        case MEMORY_ZONE_HELPER_ERROR_ZONE_NULL:                        return "MEMORY_ZONE_HELPER_ERROR_ZONE_NULL";
        case MEMORY_ZONE_HELPER_ERROR_SIZE_ZERO:                        return "MEMORY_ZONE_HELPER_ERROR_SIZE_ZERO";

        default:                                                        return "UNKNOWN_MEMORY_ZONE_RESULT";
    }
}


