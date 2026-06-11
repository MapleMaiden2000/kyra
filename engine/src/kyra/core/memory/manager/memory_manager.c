#include "kyra/core/memory/manager/memory_manager.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "kyra/core/misc/console/console.h"

static MemoryManager *memory_manager = NULL;


// Helper functions --------------------------------------------------------- //

static MemoryManagerResult _memory_manager_compute_size_classes(const ByteSize capacity, ByteSize *out_num_classes, MemoryZoneSizeClass **out_size_classes) {
    if (capacity == 0) return MEMORY_MANAGER_HELPER_ERROR_CAPACITY_ZERO;

    const Flt32 ratio = 1.618f;
    ByteSize current_size = sizeof(ByteSize);   // Start with minimum block size
    ByteSize index = 0;                         // Index for size classes

    // Count the number of size classes
    while (current_size <= capacity) {
        if (out_size_classes && *out_size_classes) {
            // Assign current size (aligned) to the size class
            (*out_size_classes)[index].size = KYRA_APPLY_MEMORY_ALIGNMENT(current_size, KYRA_MEMORY_ALIGNMENT_SIZE);
        }

        // Compute the next size class
        current_size = (ByteSize)((Flt32)current_size * ratio);

        // Move to next size class index
        ++index;
    }

    // For the last size class, assign the capacity
    if (out_size_classes && *out_size_classes) (*out_size_classes)[index].size = capacity;

    // Save to ref
    if (out_num_classes) *out_num_classes = ++index;

    return MEMORY_MANAGER_SUCCESS;
}

static void _memory_manager_format_bytes(const ByteSize size, Str out_buffer, const ByteSize buffer_size) {
    ConstStr suffixes[] = { "B", "KB", "MB", "GB" };
    ByteSize suffix_index = 0;
    Flt32 count = (Flt32)size;  // To be used if >= 1024 bytes (1 KB)

    // 'suffix_index' clamp is there to ensure it does not go out of bound
    while (count >= 1024.f && suffix_index < 3) {
        // Divide by 1024 until it cannot anymore
        count /= 1024.f;
        
        // Advance to next index for every division
        ++suffix_index;
    }

    if (suffix_index == 0) {
        // If < 1024 bytes (1 KB)...

        // Save to ref 
        snprintf(out_buffer, buffer_size, "%llu %s", size, suffixes[suffix_index]);
    }
    else {
        // Otherwise, if >= 1024 bytes (1 KB)...
        
        // Save to ref
        snprintf(out_buffer, buffer_size, "%.2f %s", count, suffixes[suffix_index]);
    }
}

static void _memory_manager_draw_progress_bar(const ByteSize used_memory, const ByteSize capacity, const ByteSize bar_width) {
    Flt32 percentage = (capacity == 0) ? 0.f : ((Flt32)used_memory / (Flt32)capacity) * 100.f;
    ByteSize num_blocks = (capacity == 0) ? 0 : (ByteSize)((Flt32)bar_width * (percentage / 100.f));

    // Determine colour based on threshold
    ConsoleColour colour = CONSOLE_COLOUR_BRIGHT_GREEN;
    if (percentage >= 75.f) colour = CONSOLE_COLOUR_BRIGHT_RED;
    if (percentage >= 50.f) colour = CONSOLE_COLOUR_BRIGHT_YELLOW;

    // Draw bar with colour
    {
        console_set_foreground(colour);
        
        Char output[KYRA_LINE_MAX_LENGTH];
        memset(output, 0, sizeof(output));

        output[0] = '[';
        for (ByteSize index = 1; index < bar_width + 1; ++index) {
            if (index < num_blocks) output[index] = '=';
            else output[index] = ' ';
        }
        output[bar_width] = ']';

        console_write(output);
        console_reset();
    }

    // Print percentage, used memory and capacity
    {
        Char str_used_memory[KYRA_LINE_MAX_LENGTH];
        Char str_capacity[KYRA_LINE_MAX_LENGTH];

        _memory_manager_format_bytes(used_memory, str_used_memory, sizeof(str_used_memory));
        _memory_manager_format_bytes(capacity, str_capacity, sizeof(str_capacity));

        console_write_line(" (%.2f%%) used memory: %s; capacity: %s", percentage, str_used_memory, str_capacity);        
    }
}


// API functions ------------------------------------------------------------ //

KYRA_ENGINE_API MemoryManagerResult memory_manager_startup(const MemoryConfig *config) {
    if (!config) return MEMORY_MANAGER_ERROR_MEMORY_CONFIG_NULL;
    if (memory_manager) return MEMORY_MANAGER_ERROR_STATE_ALREADY_INITIALISED;

    ByteSize total_capacity = 0;
    ByteSize total_size_classes = 0;

    // Iterate through each zone
    // Add to total capacity
    for (ByteSize index = 0; index < config->zone_count; ++index) {
        // Compute number of size classes
        ByteSize num_classes = 0;
        MemoryManagerResult compute_result = _memory_manager_compute_size_classes(config->zones[index].capacity, &num_classes, NULL);
        if (compute_result != MEMORY_MANAGER_SUCCESS) {
            KYRA_CONSOLE_PRINT_ERROR(
                "Failed to compute size classes for zone: %s (Error: %s).", 
                config->zones[index].name, 
                memory_manager_result_to_string(compute_result)
            );
            return compute_result;
        }

        // Add to totals
        total_size_classes += num_classes;
        total_capacity += config->zones[index].capacity;
    }

    // Compute total memory required (ensuring 16-byte alignment for each section)
    ByteSize manager_size = KYRA_APPLY_MEMORY_ALIGNMENT(sizeof(MemoryManager), KYRA_MEMORY_ALIGNMENT_SIZE);
    ByteSize zone_headers_size = KYRA_APPLY_MEMORY_ALIGNMENT(sizeof(MemoryZone) * config->zone_count, KYRA_MEMORY_ALIGNMENT_SIZE);
    ByteSize size_class_headers_size = KYRA_APPLY_MEMORY_ALIGNMENT(sizeof(MemoryZoneSizeClass) * total_size_classes, KYRA_MEMORY_ALIGNMENT_SIZE);
    ByteSize total_alloc_size = manager_size + zone_headers_size + size_class_headers_size + total_capacity;

    // Allocate one large block
    // Architecture: [memory_manager] [memory_zone_headers] [memory_size_class_headers] [memory_pool]
    UIntPtr block = (UIntPtr)calloc(1, total_alloc_size);
    if (!block) return MEMORY_MANAGER_ERROR_FAILED_TO_ALLOCATE_MEMORY_BLOCK;

    // Configure memory manager
    memory_manager = (MemoryManager *)block;
    memory_manager->zones = (MemoryZone *)(block + manager_size);
    memory_manager->zone_count = config->zone_count;
    memory_manager->capacity = total_capacity;
    memory_manager->pool = (VoidPtr)(block + manager_size + zone_headers_size + size_class_headers_size);

    // Configure memory zones
    UIntPtr addr_block = (UIntPtr)memory_manager->pool;
    UIntPtr addr_size_class = (UIntPtr)(block + manager_size + zone_headers_size);
    for (ByteSize index = 0; index < config->zone_count; ++index) {
        // For each memory zone...  
        MemoryZone *zone = &memory_manager->zones[index];
        
        // Configure properties
        zone->name = _strdup(config->zones[index].name);
        zone->capacity = config->zones[index].capacity;
        zone->used_memory = 0;
        zone->pool_offset = 0;
        zone->addr_start = addr_block;

        // Compute number of size classes
        zone->size_classes = (MemoryZoneSizeClass *)addr_size_class;
        MemoryManagerResult compute_result = _memory_manager_compute_size_classes(config->zones[index].capacity, &zone->num_classes, &zone->size_classes);
        if (compute_result != MEMORY_MANAGER_SUCCESS) {
            KYRA_CONSOLE_PRINT_ERROR(
                "Failed to compute size classes for zone: %s (Error: %s).", 
                config->zones[index].name, 
                memory_manager_result_to_string(compute_result)
            );
            
            // Deallocate memory block
            free((VoidPtr)block);
            block = 0;            
            
            return compute_result;
        }

        // Move to next memory zone
        addr_block += zone->capacity;
        addr_size_class += (sizeof(MemoryZoneSizeClass) * zone->num_classes);
    }

    return MEMORY_MANAGER_SUCCESS;
}

KYRA_ENGINE_API MemoryManagerResult memory_manager_shutdown(void) {
    if (!memory_manager) return MEMORY_MANAGER_ERROR_STATE_NOT_INITIALISED;
    
    for (ByteSize index = 0; index < memory_manager->zone_count; ++index) {
        // For each memory zone...
        MemoryZone *zone = &memory_manager->zones[index];

        // Deallocate name
        if (zone->name) free((VoidPtr)zone->name);
    }

    // Deallocate memory manager (aka. address of the entire memory block)
    free(memory_manager);
    memory_manager = NULL;

    return MEMORY_MANAGER_SUCCESS;
}

KYRA_ENGINE_API MemoryManagerResult memory_manager_get(MemoryManager **out_manager) {
    if (!memory_manager) return MEMORY_MANAGER_ERROR_STATE_NOT_INITIALISED;
    
    // Save to ref
    if (out_manager) *out_manager = memory_manager;
    
    return MEMORY_MANAGER_SUCCESS;
}

KYRA_ENGINE_API MemoryManagerResult memory_manager_report(void) {
    if (!memory_manager) return MEMORY_MANAGER_ERROR_STATE_NOT_INITIALISED;
    
    printf("===== Kyra Engine Memory Report =====\n");
    
    // Memory manager properties
    {
        printf("Total:\n");
        printf("Pool address: %p\n", memory_manager->pool);
        printf("Zone count: %u\n", memory_manager->zone_count);
        
        // Draw progress bar
        _memory_manager_draw_progress_bar(memory_manager->used_memory, memory_manager->capacity, 60);
    }

    // Memory zones
    {
        printf("\nZones:\n");
        for (ByteSize index = 0; index < memory_manager->zone_count; ++index) {
            MemoryZone *zone = &memory_manager->zones[index];
            
            // The '%-15s' ensures all progress bars start at the same vertical column
            printf("> %-15s ", zone->name); 
            
            // Draw progress bar
            _memory_manager_draw_progress_bar(zone->used_memory, zone->capacity, 40);
        }
    }

    printf("=====================================\n");

    return MEMORY_MANAGER_SUCCESS;
}


KYRA_ENGINE_API ConstStr memory_manager_result_to_string(const MemoryManagerResult result) {
    switch (result) {
        case MEMORY_MANAGER_SUCCESS:                                    return "MEMORY_MANAGER_SUCCESS";
        
        case MEMORY_MANAGER_ERROR_MEMORY_CONFIG_NULL:                   return "MEMORY_MANAGER_ERROR_MEMORY_CONFIG_NULL";
        case MEMORY_MANAGER_ERROR_STATE_ALREADY_INITIALISED:            return "MEMORY_MANAGER_ERROR_STATE_ALREADY_INITIALISED";
        case MEMORY_MANAGER_ERROR_STATE_NOT_INITIALISED:                return "MEMORY_MANAGER_ERROR_STATE_NOT_INITIALISED";
        case MEMORY_MANAGER_ERROR_FAILED_TO_ALLOCATE_MEMORY_BLOCK:      return "MEMORY_MANAGER_ERROR_FAILED_TO_ALLOCATE_MEMORY_BLOCK";
        
        case MEMORY_MANAGER_HELPER_ERROR_CAPACITY_ZERO:                 return "MEMORY_MANAGER_HELPER_ERROR_CAPACITY_ZERO";
        
        default:                                                        return "UNKNOWN_MEMORY_MANAGER_RESULT";
    }
}





