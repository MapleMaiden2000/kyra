#include "kyra/core/containers/string/string.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "kyra/core/memory/zone/memory_zone.h"


// For SIMD optimisations
#if defined(__AVX2__) || defined(__SSE__)
    #include <immintrin.h>
#endif

// For Windows-specific functions
#if KYRA_PLATFORM_WINDOWS
    #include <windows.h>
#endif


// String container structure --------------------------------------------- //

struct Container_String {
    Str         data;
    ByteSize    size;
    ByteSize    capacity;
    
    // For allocations/deallocations
    ByteSize    memory_size;
};


// Helper functions ------------------------------------------------------- //

static ContainerResult _container_string_resize(String *string, const ByteSize new_capacity) {
    if (!string) return CONTAINER_STRING_HELPER_ERROR_REF_STRING_NULL;
    if (new_capacity == 0) return CONTAINER_STRING_HELPER_ERROR_NEW_CAPACITY_ZERO;
    
    ByteSize new_alloc_size = sizeof(struct Container_String) + new_capacity;

    String old_str = *string;
    String new_str = NULL;

    // Allocate memory for new string
    ByteSize new_mem_size = 0;
    if (memory_zone_allocate("string", new_alloc_size, (VoidPtr *)&new_str, &new_mem_size) != MEMORY_ZONE_SUCCESS)
        return CONTAINER_STRING_HELPER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_NEW_STRING;

    // Copy data from old string to new string
    new_str->data = (Str)KYRA_APPLY_MEMORY_ALIGNMENT((UIntPtr)new_str + sizeof(struct Container_String), KYRA_MEMORY_ALIGNMENT_SIZE);
    memcpy(new_str->data, old_str->data, old_str->size);
    new_str->data[old_str->size] = '\0';

    // Initialise remaining properties
    new_str->size = old_str->size;
    new_str->capacity = new_capacity;
    new_str->memory_size = new_mem_size;

    // Deallocate memory of old string
    if (memory_zone_deallocate("string", (VoidPtr)old_str, old_str->memory_size) != MEMORY_ZONE_SUCCESS)
        return CONTAINER_STRING_HELPER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_OLD_STRING;

    // Save to ref
    *string = new_str;

    return CONTAINER_SUCCESS;
}


// API functions ---------------------------------------------------------- //

KYRA_ENGINE_API ContainerResult container_string_construct(ConstStr value, String *out_string) {
    if (!value) return CONTAINER_STRING_ERROR_VALUE_NULL;
    if (!out_string) return CONTAINER_STRING_ERROR_REF_OUT_STRING_NULL;
    
    // Get value length
    ByteSize size = strlen(value);

    // Compute capacity of string (using aligned size)
    // Multiply by resize factor to reduce resizings
    ByteSize capacity = KYRA_APPLY_MEMORY_ALIGNMENT((ByteSize)((Flt32)size * KYRA_CONTAINER_RESIZE_RATIO), KYRA_MEMORY_ALIGNMENT_SIZE);

    // Allocate memory for string
    ByteSize alloc_size = sizeof(struct Container_String) + capacity;
    ByteSize mem_size = 0;
    if (memory_zone_allocate("string", alloc_size, (VoidPtr *)out_string, &mem_size) != MEMORY_ZONE_SUCCESS)
        return CONTAINER_STRING_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STRING;

    // Copy value to string
    (*out_string)->data = (Str)KYRA_APPLY_MEMORY_ALIGNMENT((UIntPtr)(*out_string) + sizeof(struct Container_String), KYRA_MEMORY_ALIGNMENT_SIZE);
    memcpy((*out_string)->data, value, size);
    (*out_string)->data[size] = '\0';

    // Initialise remaining properties
    (*out_string)->size = size;
    (*out_string)->capacity = capacity;
    (*out_string)->memory_size = mem_size;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_construct_empty(String *out_string) {
    return container_string_construct("", out_string);
}

KYRA_ENGINE_API ContainerResult container_string_construct_reserved(const ByteSize capacity, String *out_string) {
    if (!out_string) return CONTAINER_STRING_ERROR_REF_OUT_STRING_NULL;
    if (capacity == 0) return CONTAINER_STRING_ERROR_CAPACITY_ZERO;

    // Capacity here would not be aligned, as they should return as requested

    // Allocate memory for string
    ByteSize alloc_size = sizeof(struct Container_String) + capacity;
    ByteSize mem_size = 0;
    if (memory_zone_allocate("string", alloc_size, (VoidPtr *)out_string, &mem_size) != MEMORY_ZONE_SUCCESS)
        return CONTAINER_STRING_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STRING;

    // Initialise properties
    (*out_string)->data = (Str)KYRA_APPLY_MEMORY_ALIGNMENT((UIntPtr)(*out_string) + sizeof(struct Container_String), KYRA_MEMORY_ALIGNMENT_SIZE);
    (*out_string)->data[0] = '\0';
    (*out_string)->size = 0;
    (*out_string)->capacity = capacity;
    (*out_string)->memory_size = mem_size;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_construct_from_chars(const Char c, const ByteSize count, String *out_string) {
    if (!out_string) return CONTAINER_STRING_ERROR_REF_OUT_STRING_NULL;

    // Count being zero means string would be empty
    if (count == 0) return container_string_construct_empty(out_string);

    // Compute capacity of string (using aligned size)
    // Multiply by resize factor to reduce resizings
    ByteSize capacity = KYRA_APPLY_MEMORY_ALIGNMENT((ByteSize)((Flt32)count * KYRA_CONTAINER_RESIZE_RATIO), KYRA_MEMORY_ALIGNMENT_SIZE);

    // Allocate memory for string
    ByteSize alloc_size = sizeof(struct Container_String) + capacity;
    ByteSize mem_size = 0;
    if (memory_zone_allocate("string", alloc_size, (VoidPtr *)out_string, &mem_size) != MEMORY_ZONE_SUCCESS)
        return CONTAINER_STRING_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STRING;

    // Populate chars
    (*out_string)->data = (Str)KYRA_APPLY_MEMORY_ALIGNMENT((UIntPtr)(*out_string) + sizeof(struct Container_String), KYRA_MEMORY_ALIGNMENT_SIZE);
    memset((*out_string)->data, c, count);

    // Initialise remaining properties
    (*out_string)->size = count;
    (*out_string)->capacity = capacity;
    (*out_string)->memory_size = mem_size;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_construct_formatted(String *out_string, ConstStr format, ...) {
    if (!out_string) return CONTAINER_STRING_ERROR_REF_OUT_STRING_NULL;
    if (!format) return CONTAINER_STRING_ERROR_FORMAT_NULL;
    
    VaList args;

    // Get required size
    va_start(args, format);
    ByteSize size = vsnprintf(NULL, 0, format, args);
    va_end(args);

    // Compute capacity of string (using aligned size)
    // Multiply by resize factor to reduce resizings
    ByteSize capacity = KYRA_APPLY_MEMORY_ALIGNMENT((ByteSize)((Flt32)size * KYRA_CONTAINER_RESIZE_RATIO), KYRA_MEMORY_ALIGNMENT_SIZE);

    // Allocate memory for string
    ByteSize alloc_size = sizeof(struct Container_String) + capacity;
    ByteSize mem_size = 0;
    if (memory_zone_allocate("string", alloc_size, (VoidPtr *)out_string, &mem_size) != MEMORY_ZONE_SUCCESS)
        return CONTAINER_STRING_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STRING;

    // Format string value
    (*out_string)->data = (Str)KYRA_APPLY_MEMORY_ALIGNMENT((UIntPtr)(*out_string) + sizeof(struct Container_String), KYRA_MEMORY_ALIGNMENT_SIZE);
    va_start(args, format);
    vsnprintf((*out_string)->data, capacity, format, args);
    va_end(args);

    // Initialise remaining properties
    (*out_string)->size = size;
    (*out_string)->capacity = capacity;
    (*out_string)->memory_size = mem_size;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_destruct(String *string) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;
    if (!(*string)->memory_size) return CONTAINER_STRING_ERROR_REF_STRING_NOT_VALID;

    // Deallocate memory for string
    if (memory_zone_deallocate("string", (VoidPtr)(*string), (*string)->memory_size) != MEMORY_ZONE_SUCCESS)
        return CONTAINER_STRING_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STRING;

    // Set string to NULL
    (*string) = NULL;
}

KYRA_ENGINE_API ContainerResult container_string_clear(String *string) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;
    
    // Clear string
    memset((*string)->data, 0, (*string)->size);

    // Reset string size
    (*string)->size = 0;
    
    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_set(String *string, ConstStr value) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;
    if (!value) return CONTAINER_STRING_ERROR_VALUE_NULL;
    
    ByteSize new_size = strlen(value);

    if (new_size >= (*string)->capacity) {
        // New size is larger than current string capacity...

        // Calculate new capacity
        ByteSize new_capacity = KYRA_APPLY_MEMORY_ALIGNMENT((ByteSize)((Flt32)new_size * KYRA_CONTAINER_RESIZE_RATIO), KYRA_MEMORY_ALIGNMENT_SIZE);
        
        // Resize
        ContainerResult resize_result = _container_string_resize(string, new_capacity);
        if (resize_result != CONTAINER_SUCCESS) return resize_result;
    }

    // Update string data
    memcpy((*string)->data, value, new_size);
    (*string)->data[new_size] = '\0';

    // Update string size
    (*string)->size = new_size;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_append(String *string, ConstStr value) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;
    if (!value) return CONTAINER_STRING_ERROR_VALUE_NULL;

    ByteSize string_size = (*string)->size;
    ByteSize value_size = strlen(value);

    // Calculate new size
    ByteSize new_size = string_size + value_size;

    if (new_size >= (*string)->capacity) {
        // New size is larger than current string capacity...

        // Calculate new capacity
        ByteSize new_capacity = KYRA_APPLY_MEMORY_ALIGNMENT((ByteSize)((Flt32)new_size * KYRA_CONTAINER_RESIZE_RATIO), KYRA_MEMORY_ALIGNMENT_SIZE);
        
        // Resize
        ContainerResult resize_result = _container_string_resize(string, new_capacity);
        if (resize_result != CONTAINER_SUCCESS) return resize_result;
    }

    // Append value to end of string
    memcpy((*string)->data + string_size, value, value_size);
    (*string)->data[new_size] = '\0';

    // Update string size
    (*string)->size = new_size;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_append_chars(String *string, const Char c, const ByteSize count) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;

    // Return if there is zero character needed to append
    if (count == 0) return CONTAINER_SUCCESS;

    ByteSize string_size = (*string)->size;
    
    // Calculate new size
    ByteSize new_size = string_size + count;

    if (new_size >= (*string)->capacity) {
        // New size is larger than current string capacity...

        // Calculate new capacity
        ByteSize new_capacity = KYRA_APPLY_MEMORY_ALIGNMENT((ByteSize)((Flt32)new_size * KYRA_CONTAINER_RESIZE_RATIO), KYRA_MEMORY_ALIGNMENT_SIZE);
        
        // Resize
        ContainerResult resize_result = _container_string_resize(string, new_capacity);
        if (resize_result != CONTAINER_SUCCESS) return resize_result;
    }

    // Append characters to end of string
    memset((*string)->data + string_size, c, count);
    (*string)->data[new_size] = '\0';

    // Update string size
    (*string)->size = new_size;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_append_formatted(String *string, ConstStr format, ...) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;
    if (!format) return CONTAINER_STRING_ERROR_FORMAT_NULL;

    VaList args;
    ByteSize string_size = (*string)->size;
    
    // Calculate new size
    va_start(args, format);
    ByteSize new_size = vsnprintf(NULL, 0, format, args);
    va_end(args);

    if (new_size >= (*string)->capacity) {
        // New size is larger than current string capacity...

        // Calculate new capacity
        ByteSize new_capacity = KYRA_APPLY_MEMORY_ALIGNMENT((ByteSize)((Flt32)new_size * KYRA_CONTAINER_RESIZE_RATIO), KYRA_MEMORY_ALIGNMENT_SIZE);
        
        // Resize
        ContainerResult resize_result = _container_string_resize(string, new_capacity);
        if (resize_result != CONTAINER_SUCCESS) return resize_result;
    }

    // Append formatted value to end of string
    va_start(args, format);
    vsnprintf((*string)->data + string_size, new_size + 1, format, args);
    va_end(args);

    // Update string size
    (*string)->size = new_size;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_detach(String *string) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;

    // Detach last character from end of string
    // Decrement string size
    (*string)->data[--(*string)->size] = '\0';

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_detach_ranged(String *string, const ByteSize range) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;

    // Return if there is range for removal is zero
    if (range == 0) return CONTAINER_SUCCESS;

    // Remove 'range' characters from end of string
    (*string)->size = (*string)->size > range ? (*string)->size - range : 0;
    (*string)->data[(*string)->size] = '\0';

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_insert(String *string, const ByteSize index, ConstStr substr) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;
    if (!substr) return CONTAINER_STRING_ERROR_SUBSTRING_NULL;
    if (index > (*string)->size) return CONTAINER_STRING_ERROR_INDEX_OUT_OF_BOUNDS;
    
    ByteSize string_size = (*string)->size;
    ByteSize substring_size = strlen(substr);

    // Calculate new size
    ByteSize new_size = string_size + substring_size;

    if (new_size >= (*string)->capacity) {
        // New size is larger than current string capacity...

        // Calculate new capacity
        ByteSize new_capacity = KYRA_APPLY_MEMORY_ALIGNMENT((ByteSize)((Flt32)new_size * KYRA_CONTAINER_RESIZE_RATIO), KYRA_MEMORY_ALIGNMENT_SIZE);
        
        // Resize
        ContainerResult resize_result = _container_string_resize(string, new_capacity);
        if (resize_result != CONTAINER_SUCCESS) return resize_result;
    }

    // Shift 'substring_size' characters (from specified index) to the right to make room for substring
    memmove((*string)->data + index + substring_size, (*string)->data + index, (*string)->size - index);

    // Copy substring into the gap
    memcpy((*string)->data + index, substr, substring_size);

    // Null-terminate string
    (*string)->data[new_size] = '\0';

    // Update string size
    (*string)->size = new_size;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_insert_chars(String *string, const ByteSize index, const Char c, const ByteSize count) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;
    if (index > (*string)->size) return CONTAINER_STRING_ERROR_INDEX_OUT_OF_BOUNDS;

    // Return if there is number of character to insert is zero
    if (count == 0) return CONTAINER_SUCCESS;

    ByteSize string_size = (*string)->size;

    // Calculate new size
    ByteSize new_size = string_size + count;

    if (new_size >= (*string)->capacity) {
        // New size is larger than current string capacity...

        // Calculate new capacity
        ByteSize new_capacity = KYRA_APPLY_MEMORY_ALIGNMENT((ByteSize)((Flt32)new_size * KYRA_CONTAINER_RESIZE_RATIO), KYRA_MEMORY_ALIGNMENT_SIZE);
        
        // Resize
        ContainerResult resize_result = _container_string_resize(string, new_capacity);
        if (resize_result != CONTAINER_SUCCESS) return resize_result;
    }

    // Shift 'count' characters to the right to make room for characters
    memmove((*string)->data + index + count, (*string)->data + index, (*string)->size - index);
    
    // Copy characters into the gap
    memset((*string)->data + index, c, count);
    
    // Null-terminate string
    (*string)->data[new_size] = '\0';
    
    // Update string size
    (*string)->size = new_size;
    
    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_insert_formatted(String *string, const ByteSize index, ConstStr format, ...) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;
    if (index > (*string)->size) return CONTAINER_STRING_ERROR_INDEX_OUT_OF_BOUNDS;
    if (!format) return CONTAINER_STRING_ERROR_FORMAT_NULL;

    ByteSize string_size = (*string)->size; 

    VaList args;
    va_start(args, format);
    ByteSize formatted_size = vsnprintf(NULL, 0, format, args);
    va_end(args);

    // Calculate new size
    ByteSize new_size = string_size + formatted_size; 

    if (new_size >= (*string)->capacity) {
        // New size is larger than current string capacity...

        // Calculate new capacity
        ByteSize new_capacity = KYRA_APPLY_MEMORY_ALIGNMENT((ByteSize)((Flt32)new_size * KYRA_CONTAINER_RESIZE_RATIO), KYRA_MEMORY_ALIGNMENT_SIZE);
        
        // Resize
        ContainerResult resize_result = _container_string_resize(string, new_capacity);
        if (resize_result != CONTAINER_SUCCESS) return resize_result;
    }

    // Shift 'formatted_size' characters to the right to make room for formatted string
    memmove((*string)->data + index + formatted_size, (*string)->data + index, (*string)->size - index);
        
    // Insert formatted string
    va_start(args, format);
    vsnprintf((*string)->data + index, formatted_size + 1, format, args);
    va_end(args);
    
    // Null-terminate
    (*string)->data[new_size] = '\0';
        
    // Update string size
    (*string)->size = new_size;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_remove_at(String *string, const ByteSize index, const ByteSize count) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;
    if (index > (*string)->size) return CONTAINER_STRING_ERROR_INDEX_OUT_OF_BOUNDS;

    // Return if number of characters to remove is zero
    if (count == 0) return CONTAINER_SUCCESS;

    // Clamp remove count to valid range
    ByteSize valid_range = (*string)->size - index;
    ByteSize rmv_count = count > valid_range ? valid_range : count; 

    // Calculate new size
    ByteSize new_size = (*string)->size - rmv_count;

    // Shift characters to the left to remove 'count' characters
    memmove((*string)->data + index, (*string)->data + index + rmv_count, (*string)->size - index - rmv_count);
        
    // Null-terminate
    (*string)->data[new_size] = '\0';
        
    // Update string size
    (*string)->size = new_size;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_to_cstr(const String string, ConstStr *out_cstr) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;

    // Save to ref
    if (out_cstr) *out_cstr = string->data;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_size(const String string, ByteSize *out_size) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;

    // Save to ref
    if (out_size) *out_size = string->size;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_capacity(const String string, ByteSize *out_capacity) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;

    // Save to ref
    if (out_capacity) *out_capacity = string->capacity;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_is_empty(const String string, Bool *out_result) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;

    // Save to ref
    if (out_result) *out_result = (string->size == 0);

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ConstStr container_string_result_to_string(const ContainerResult result) {
    switch (result) {
        case CONTAINER_SUCCESS:                                                             return "CONTAINER_SUCCESS";
        
        case CONTAINER_STRING_HELPER_ERROR_REF_STRING_NULL:                                 return "CONTAINER_STRING_HELPER_ERROR_REF_STRING_NULL";
        case CONTAINER_STRING_HELPER_ERROR_NEW_CAPACITY_ZERO:                               return "CONTAINER_STRING_HELPER_ERROR_NEW_CAPACITY_ZERO";
        case CONTAINER_STRING_HELPER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_NEW_STRING:        return "CONTAINER_STRING_HELPER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_NEW_STRING";
        case CONTAINER_STRING_HELPER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_OLD_STRING:       return "CONTAINER_STRING_HELPER_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_OLD_STRING";
        
        case CONTAINER_STRING_ERROR_VALUE_NULL:                                             return "CONTAINER_STRING_ERROR_VALUE_NULL";
        case CONTAINER_STRING_ERROR_FORMAT_NULL:                                            return "CONTAINER_STRING_ERROR_FORMAT_NULL";
        case CONTAINER_STRING_ERROR_CAPACITY_ZERO:                                          return "CONTAINER_STRING_ERROR_CAPACITY_ZERO";
        case CONTAINER_STRING_ERROR_SUBSTRING_NULL:                                         return "CONTAINER_STRING_ERROR_SUBSTRING_NULL";
        case CONTAINER_STRING_ERROR_INDEX_OUT_OF_BOUNDS:                                    return "CONTAINER_STRING_ERROR_INDEX_OUT_OF_BOUNDS";
        case CONTAINER_STRING_ERROR_REF_OUT_STRING_NULL:                                    return "CONTAINER_STRING_ERROR_REF_OUT_STRING_NULL";
        case CONTAINER_STRING_ERROR_REF_STRING_NULL:                                        return "CONTAINER_STRING_ERROR_REF_STRING_NULL";
        case CONTAINER_STRING_ERROR_REF_STRING_NOT_VALID:                                   return "CONTAINER_STRING_ERROR_REF_STRING_NOT_VALID";
        case CONTAINER_STRING_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STRING:                   return "CONTAINER_STRING_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STRING";
        case CONTAINER_STRING_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STRING:                  return "CONTAINER_STRING_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STRING";
        
        default:                                                                            return "UNKNOWN_CONTAINER_RESULT";
    }
}


