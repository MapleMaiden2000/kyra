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

// Count trailing zeroes for SIMD optimisations
KYRA_INLINE Int32 _container_string_ctz(UInt32 mask) {
    #if defined (__GNUC__) || defined (__clang__)
        // Use GCC and Clang built-in function
        return __builtin_ctz(mask);
    
    #elif defined (_MSC_VER)
        // Use MSVC built-in function
        UInt32 index;
        _BitScanForward(&index, mask);
        
        return (Int32)index;
    
    #else
        // Fallback for other compilers
        if (mask == 0) return 32;
    
        Int32 index = 0;
        while ((mask & 1) == 0) {
            mask >>= 1;
            ++index;
        }
        
        return index;
    #endif
}

// Count leading zeroes for SIMD optimisations
KYRA_INLINE Int32 _container_string_clz(UInt32 mask) {
    #if defined (__GNUC__) || defined (__clang__)
        // Use GCC and Clang built-in function
        return __builtin_clz(mask);
    
    #elif defined (_MSC_VER)
        // Use MSVC built-in function
        UInt32 index;
        _BitScanReverse(&index, mask);
        
        return (Int32)index;
    
    #else
        // Fallback for other compilers
        if (mask == 0) return 32;
    
        Int32 index = 0;
        while ((mask & 0x80000000) == 0) {
            mask <<= 1;
            ++index;
        }
    
        return index;
    #endif
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
    
    va_start(args, format);
    ByteSize formatted_size = vsnprintf(NULL, 0, format, args);
    va_end(args);
    
    // Calculate new size
    ByteSize new_size = string_size + vsnprintf(NULL, 0, format, args);

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
    vsnprintf((*string)->data + string_size, formatted_size + 1, format, args);
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

    // Shift characters to the right to make room for substring
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

    // Shift characters to the right to make room for characters
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

    // Shift characters to the right to make room for formatted string
    memmove((*string)->data + index + formatted_size, (*string)->data + index, (*string)->size - index);
        
    // Insert formatted string
    va_start(args, format);
    vsnprintf((*string)->data + index, formatted_size + 1, format, args);
    va_end(args);
    
    // Null-terminate string
    (*string)->data[new_size] = '\0';
        
    // Update string size
    (*string)->size = new_size;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_remove(String *string, ConstStr substr, const Bool remove_all) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;
    if (!substr) return CONTAINER_STRING_ERROR_SUBSTRING_NULL;

    ByteSize substring_size = strlen(substr);

    Str addr_matched = strstr((*string)->data, substr);

    while (addr_matched) {
        // Shift characters to the left to overwrite 'substring_size' characters
        ByteSize index = addr_matched - (*string)->data;
        memmove((*string)->data + index, (*string)->data + index + substring_size, (*string)->size - index - substring_size);

        // Update string size
        (*string)->size -= substring_size;

        // Null-terminate string
        (*string)->data[(*string)->size] = '\0';

        if (!remove_all) {
            // If 'remove_all' is true, return since we only need to remove once
            return CONTAINER_SUCCESS;
        }
        
        // Look for next match
        addr_matched = strstr((*string)->data, substr);   
    }
    
    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_remove_chars(String *string, const Char c, const Bool remove_all) {\
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;

    // Return if requested to remove null-terminator character
    if (c == '\0') return CONTAINER_SUCCESS;

    Str addr_matched = strchr((*string)->data, c);

    while (addr_matched) {
        // Shift characters to the left to overwrite specified character
        ByteSize index = addr_matched - (*string)->data;
        memmove((*string)->data + index, (*string)->data + index + 1, (*string)->size - index - 1);

        // Decrement string size
        // Null-terminate string
        (*string)->data[--(*string)->size] = '\0';

        if (!remove_all) {
            // If 'remove_all' is true, return since we only need to remove once
            return CONTAINER_SUCCESS;
        }
        
        // Look for next match
        addr_matched = strchr((*string)->data, c);
    }

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_remove_at(String *string, const ByteSize index, const ByteSize count) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;
    if (index > (*string)->size) return CONTAINER_STRING_ERROR_INDEX_OUT_OF_BOUNDS;

    // Return if number of characters to remove is zero
    if (count == 0) return CONTAINER_SUCCESS;

    ByteSize string_size = (*string)->size;

    // Clamp remove count to valid range
    ByteSize valid_range = string_size - index;
    ByteSize rmv_count = count > valid_range ? valid_range : count; 

    // Calculate new size
    ByteSize new_size = string_size - rmv_count;
    
    // Shift characters to the left to overwrite 'rmv_count' characters
    memmove((*string)->data + index, (*string)->data + index + rmv_count, string_size - index - rmv_count);
        
    // Null-terminate string
    (*string)->data[new_size] = '\0';
        
    // Update string size
    (*string)->size = new_size;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_replace_char(String *string, const Char old_char, const Char new_char) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;

    // Return if old character is the same as new character
    if (old_char == new_char) return CONTAINER_SUCCESS;
    
    ByteSize string_size = (*string)->size;
    Int32 index = 0;

    #if defined(__AVX2__)
    {
        __m256i vec_old = _mm256_set1_epi8(old_char);
        __m256i vec_new = _mm256_set1_epi8(new_char);

        while (index + 32 <= string_size) {
            // For current block of 32 characters...
            
            // Load in 256 bits (32 characters) from current index into memory
            __m256i vec_str = _mm256_loadu_si256((__m256i *)((*string)->data + index));

            // Check for matching characters in string 
            __m256i mask_cmp = _mm256_cmpeq_epi8(vec_str, vec_old);

            // Replace old characters with new characters using 'mask_cmp'
            // Use 'vec_org' which has original cases
            __m256i result = _mm256_blendv_epi8(vec_str, vec_new, mask_cmp);

            // Save to string data
            _mm256_storeu_si256((__m256i *)((*string)->data + index), result);

            // Advance to next block (32 characters)
            index += 32;
        }
    }
    #endif

    #if defined(__SSE__)
    {
        __m128i vec_old = _mm_set1_epi8(old_char);
        __m128i vec_new = _mm_set1_epi8(new_char);

        while (index + 16 <= string_size) {
            // For current block of 16 characters...
            
            // Load in 128 bits (16 characters) from current index into memory
            __m128i vec_str = _mm_loadu_si128((__m128i*)((*string)->data + index));

            // Check for matching characters in string 
            __m128i mask_cmp = _mm_cmpeq_epi8(vec_str, vec_old);

            // Replace old characters with new characters using the 'mask_cmp'
            // Use 'vec_org' which has original cases
            __m128i mask_a = _mm_and_si128(vec_new, mask_cmp);
            __m128i mask_b = _mm_andnot_si128(mask_cmp, vec_str);
            __m128i result = _mm_or_si128(mask_a, mask_b);

            // Save to string data
            _mm_storeu_si128((__m128i*)((*string)->data + index), result);

            // Advance to next block (16 characters)
            index += 16;
        }
    }
    #endif

    // Handle remaining characters
    // Also acts as fallback implementation, in case of no SIMD support
    {
        while (index < string_size) {
            // Replace old character with new character if matched
            if ((*string)->data[index] == old_char) (*string)->data[index] = new_char;

            // Advance to next character
            ++index;
        }
    }

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_replace_substring(String *string, ConstStr old_substr, ConstStr new_substr) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;
    if (!old_substr || !new_substr) return CONTAINER_STRING_ERROR_SUBSTRING_NULL;

    // Return if old substring is the same as new substring
    if (!strcmp(old_substr, new_substr)) return CONTAINER_SUCCESS;

    ByteSize string_size = (*string)->size;
    ByteSize old_substring_size = strlen(old_substr);
    ByteSize new_substring_size = strlen(new_substr);

    if (old_substring_size > string_size) return CONTAINER_STRING_ERROR_SUBSTRING_LONGER_THAN_STRING;

    ByteSize match_count = 0;
    ByteSize index = 0;
    
    
    // --- Count matches --- // 
    
    #if defined(__AVX2__)
    {
        __m256i vec_old_first = _mm256_set1_epi8(old_substr[0]);

        while (index + 32 <= string_size) {
            // For current block of 32 characters...
            
            // Load in 256 bits (32 characters) from current index into memory
            __m256i vec_str = _mm256_loadu_si256((__m256i *)((*string)->data + index));

            // Compare with old character to create a mask
            __m256i mask_cmp = _mm256_cmpeq_epi8(vec_str, vec_old_first);

            Int32 matched_bits = _mm256_movemask_epi8(mask_cmp);
            if (!matched_bits) { 
                // No matches in this block...
                
                // Advance to next block (32 characters)
                index += 32; 
                continue; 
            }

            Bool found = false;

            // Otherwise...
            while (matched_bits) {
                Int32 matched_index = _container_string_ctz(matched_bits);
                Int32 actual_index = index + matched_index;

                if (!strncmp((*string)->data + actual_index, old_substr, old_substring_size)) {
                    // Found matching substring...

                    // Increment match count
                    ++match_count;

                    // Jump past matched substring to avoid overlapping matches
                    index = actual_index + old_substring_size;

                    // Clear bits to load the next block (32 characters)
                    matched_bits = 0;

                    found = true;
                }
                else {
                    // Otherwise... no matching substring

                    // Remove the matched bit
                    matched_bits &= ~(1 << matched_index);
                }
            }

            // Advance if no matches were found
            if (!found) index += 32;
        }
    }
    #endif

    #if defined(__SSE__) 
    {
        __m128i vec_old_first = _mm_set1_epi8(old_substr[0]);

        while (index + 16 <= string_size) {
            // For current block of 16 characters...
            
            // Load in 128 bits (16 characters) from current index into memory
            __m128i vec_str = _mm_loadu_si128((__m128i*)((*string)->data + index));
            
            // Check for matching characters in string 
            __m128i mask_cmp = _mm_cmpeq_epi8(vec_str, vec_old_first);

            Int32 matched_bits = _mm_movemask_epi8(mask_cmp);
            if (!matched_bits) { 
                // No matches in this block...
                
                // Advance to next block (16 characters)
                index += 16; 
                continue; 
            }

            Bool found = false;

            // Otherwise...
            while (matched_bits) {
                Int32 matched_index = _container_string_ctz(matched_bits);
                Int32 actual_index = index + matched_index;

                if (!strncmp((*string)->data + actual_index, old_substr, old_substring_size)) {
                    // Found matching substring...

                    // Increment match count
                    ++match_count;

                    // Jump past matched substring to avoid overlapping matches
                    index = actual_index + old_substring_size;

                    // Clear bits to load the next block (16 characters)
                    matched_bits = 0;

                    found = true;
                }
                else {
                    // Otherwise... no matching substring

                    // Remove the matched bit
                    matched_bits &= ~(1 << matched_index);
                }
            }

            // Advance if no matches were found
            if (!found) index += 16;
        }
    }
    #endif

    // Handle remaining bytes
    // Also acts as fallback implementation, in case of no SIMD support
    while (index <= string_size - old_substring_size) {
        if (!strncmp((*string)->data + index, old_substr, old_substring_size)) {
            // Found matching string...

            // Increment match count
            ++match_count;

            // Jump past matched substring to avoid overlapping matches
            index += old_substring_size;
        } else {
            // Otherwise...

            // Advance to next index and check
            ++index;
        }
    }

    if (match_count == 0) return CONTAINER_STRING_ERROR_FAILED_TO_FIND_ANY_MATCH;


    // --- Reconstruct --- // 
    
    // Calculate resize delta which handles all resizings (shrinking and expanding)
    Int64 resize_delta = (Int64)new_substring_size - (Int64)old_substring_size;
    
    // Calculate new size and capacity
    ByteSize new_size = (ByteSize)((Int64)string_size + (Int64)match_count * resize_delta);
    ByteSize new_capacity = KYRA_APPLY_MEMORY_ALIGNMENT(new_size + 1, KYRA_MEMORY_ALIGNMENT_SIZE);
    
    // Reserve construct new string
    String new_str = NULL;
    ContainerResult reserve_result = container_string_construct_reserved(new_capacity, &new_str);
    if (reserve_result != CONTAINER_SUCCESS) return reserve_result;
    

    // --- Populate --- // 

    ByteSize addr_src = 0;
    ByteSize addr_dst = 0;

    while (addr_src < string_size) {
        // Check if 
        if (addr_src <= string_size - old_substring_size && !strncmp((*string)->data + addr_src, old_substr, old_substring_size)) {
            // Founding matchined old substring at current position...

            // Copy new substring to new string
            memcpy(new_str->data + addr_dst, new_substr, new_substring_size);
            
            addr_src += old_substring_size;
            addr_dst += new_substring_size;
        }
        else {
            // Otherwise...

            // Copy character from old to new string
            // Advance to next respective index
            new_str->data[addr_dst++] = (*string)->data[addr_src++];
        }
    }

    // Null-terminate string
    new_str->data[new_size] = '\0';

    // Update string size
    new_str->size = new_size;

    // Cleanup and swap
    String old_str = *string;
    container_string_destruct(&old_str);
    *string = new_str;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_to_lower(String *string) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;

    ByteSize string_size = (*string)->size;
    ByteSize index = 0;

    #if defined(__AVX2__)
    {
        __m256i a_minus_one = _mm256_set1_epi8('A' - 1);
        __m256i z_plus_one = _mm256_set1_epi8('Z' + 1);
        __m256i mask_lower  = _mm256_set1_epi8(0x20);

        while (index + 32 <= string_size) {
            // For current block of 32 characters...

            // Load in 256 bits (32 characters) from current index into memory
            __m256i vec_str = _mm256_loadu_si256((__m256i *)((*string)->data + index));

            // Create mask for characters whichever being uppercase
            __m256i mask_upper = _mm256_and_si256(_mm256_cmpgt_epi8(vec_str, a_minus_one), _mm256_cmpgt_epi8(z_plus_one, vec_str));

            // Convert to lowercase
            vec_str = _mm256_or_si256(vec_str, _mm256_and_si256(mask_upper, mask_lower));

            // Save to string
            _mm256_storeu_si256((__m256i*)((*string)->data + index), vec_str);

            // Advance to next block (32 characters)
            index += 32;
        }
    }
    #endif

    #if defined(__SSE__)
    {
        __m128i a_minus_one = _mm_set1_epi8('A' - 1);
        __m128i z_plus_one = _mm_set1_epi8('Z' + 1);
        __m128i mask_lower = _mm_set1_epi8(0x20);

        while (index + 16 <= string_size) {
            // For current block of 16 characters...

            // Load in 128 bits (16 characters) from current index into memory
            __m128i vec_str = _mm_loadu_si128((__m128i *)((*string)->data + index));

            // Create mask for characters whichever being uppercase
            __m128i mask_upper = _mm_and_si128(_mm_cmpgt_epi8(vec_str, a_minus_one), _mm_cmpgt_epi8(z_plus_one, vec_str));

            // Convert to lowercase
            vec_str = _mm_or_si128(vec_str, _mm_and_si128(mask_upper, mask_lower));

            // Save to string
            _mm_storeu_si128((__m128i*)((*string)->data + index), vec_str);

            // Advance to next block (16 characters)
            index += 16;
        }
    }
    #endif

    // Handle remaining bytes
    // Also acts as fallback implementation, in case of no SIMD support
    while (index < string_size) {
        // Convert to lower if character is of uppercase
        if ((*string)->data[index] >= 'A' && (*string)->data[index] <= 'Z') (*string)->data[index] |= 0x20;
        
        // Advance to next character
        ++index;
    }

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_to_upper(String *string) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;

    ByteSize string_size = (*string)->size;
    ByteSize index = 0;

    #if defined(__AVX2__)
    {
        __m256i a_minus_one = _mm256_set1_epi8('A' - 1);
        __m256i z_plus_one = _mm256_set1_epi8('Z' + 1);
        __m256i mask_upper = _mm256_set1_epi8(~0x20);

        while (index + 32 <= string_size) {
            // For current block of 32 characters...

            // Load in 256 bits (32 characters) from current index into memory
            __m256i vec_str = _mm256_loadu_si256((__m256i *)((*string)->data + index));

            // Create mask for characters whichever being lowercase
            __m256i mask_lower = _mm256_and_si256(_mm256_cmpgt_epi8(vec_str, a_minus_one), _mm256_cmpgt_epi8(z_plus_one, vec_str));

            // Convert to uppercase
            vec_str = _mm256_and_si256(vec_str, _mm256_or_si256(mask_lower, mask_upper));

            // Store to string
            _mm256_storeu_si256((__m256i*)((*string)->data + index), vec_str);

            // Advance to next block (32 characters)
            index += 32;
        }
    }
    #endif

    #if defined(__SSE__)
    {   
        __m128i a_minus_one = _mm_set1_epi8('a' - 1);
        __m128i z_plus_one = _mm_set1_epi8('z' + 1);
        __m128i mask_upper = _mm_set1_epi8(~0x20);

        while (index + 16 <= string_size) {
            // For current block of 16 characters...

            // Load in 128 bits (16 characters) from current index into memory
            __m128i vec_str = _mm_loadu_si128((__m128i *)((*string)->data + index));

            // Create mask for characters whichever being lowercase
            __m128i mask_lower = _mm_and_si128(_mm_cmpgt_epi8(vec_str, a_minus_one), _mm_cmpgt_epi8(z_plus_one, vec_str));

            // Convert to uppercase
            vec_str = _mm_and_si128(vec_str, _mm_or_si128(mask_lower, mask_upper));

            // Store to string
            _mm_storeu_si128((__m128i*)((*string)->data + index), vec_str);

            // Advance to next block (16 characters)
            index += 16;
        }
    }
    #endif

    // Handle the remaining bytes
    // Also acts as fallback implementation, in case of no SIMD support
    while (index < (*string)->size) {
        // Convert to uppercase if character is of lowercase
        if ((*string)->data[index] >= 'a' && (*string)->data[index] <= 'z') (*string)->data[index] &= ~0x20;
        
        // Advance to next character
        ++index;
    }
        
    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_trim_left(String *string) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;

    ByteSize start = 0, end = (*string)->size;

    #if defined(__AVX2__)
    {
        __m256i mask_space = _mm256_set1_epi8(' ');
        __m256i mask_tab = _mm256_set1_epi8('\t');
        __m256i mask_newln = _mm256_set1_epi8('\n');
        __m256i mask_return = _mm256_set1_epi8('\r');

        while (start + 32 <= end) {
            // For current block of 32 characters...

            // Load in 256 bits (32 characters) from current 'start' index into memory
            __m256i vec_str = _mm256_loadu_si256((__m256i *)((*string)->data + start));

            // Create masks for each whitespace character
            __m256i mask_is_space = _mm256_cmpeq_epi8(vec_str, mask_space);
            __m256i mask_is_tab = _mm256_cmpeq_epi8(vec_str, mask_tab);
            __m256i mask_is_newln = _mm256_cmpeq_epi8(vec_str, mask_newln);
            __m256i mask_is_return = _mm256_cmpeq_epi8(vec_str, mask_return);

            // Combine masks to find all whitespace
            __m256i mask_all_whitespace = _mm256_or_si256(
                _mm256_or_si256(mask_is_space, mask_is_tab), _mm256_or_si256(mask_is_newln, mask_is_return)
            );

            Int32 matched_bits = _mm256_movemask_epi8(mask_all_whitespace);
            if (matched_bits == (Int32)0xffffffff) {
                // All characters are whitespace...

                // Advance to next block (32 characters)
                start += 32;
            }
            else {
                // Otherwise...

                // Locate the first non-whitespace character
                Int32 non_match_index = _container_string_ctz(~matched_bits);
                Int32 actual_index = start + non_match_index;

                // Trim to that character
                start = actual_index;

                // Exit the loop
                break;
            }
        }
    }
    #endif

    #if defined(__SSE__)
    {
        __m128i mask_space = _mm_set1_epi8(' ');
        __m128i mask_tab = _mm_set1_epi8('\t');
        __m128i mask_newln = _mm_set1_epi8('\n');
        __m128i mask_return = _mm_set1_epi8('\r');
        
        while (start + 16 <= end) {
            // For current block of 16 characters...

            // Load in 128 bits (16 characters) from current 'start' index into memory
            __m128i vec_str = _mm_loadu_si128((__m128i *)((*string)->data + start));
            
            // Create masks for each whitespace character
            __m128i mask_is_space = _mm_cmpeq_epi8(vec_str, mask_space);
            __m128i mask_is_tab = _mm_cmpeq_epi8(vec_str, mask_tab);
            __m128i mask_is_newln = _mm_cmpeq_epi8(vec_str, mask_newln);
            __m128i mask_is_return = _mm_cmpeq_epi8(vec_str, mask_return);
            
            // Combine masks to find all whitespace
            __m128i mask_all_whitespace = _mm_or_si128(
                _mm_or_si128(mask_is_space, mask_is_tab), _mm_or_si128(mask_is_newln, mask_is_return)
            );
            
            Int16 matched_bits = (Int16)_mm_movemask_epi8(mask_all_whitespace);
            if (matched_bits == (Int16)0xffff) {
                // All characters are whitespace...

                // Advance to next block (16 characters)
                start += 16;
            }
            else {
                // Otherwise...

                // Locate the first non-whitespace character
                Int32 non_match_index = _container_string_ctz(~matched_bits);
                Int32 actual_index = start + non_match_index;

                // Trim to that character
                start = actual_index;

                // Exit the loop
                break;
            }
        }
    }
    #endif

    // Handle the remaining bytes
    // Also acts as fallback implementation, in case of no SIMD support
    while (start < end) {
        if (
            (*string)->data[start] != ' ' &&
            (*string)->data[start] != '\t' && 
            (*string)->data[start] != '\n' && 
            (*string)->data[start] != '\r'
        ) {
            // Found non-whitespace character...

            // Done...
            // Exit the loop
            break;
        }
        
        // Advance to next character
        ++start;
    }

    // Shift characters to the left to overwrite leading whitespace characters
    if (start > 0) memmove((*string)->data, (*string)->data + start, end - start);

    // Update string size
    (*string)->size = end - start;

    // Null-terminate string
    (*string)->data[(*string)->size] = '\0';

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_trim_right(String *string) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;

    ByteSize start = 0, end = (*string)->size;

    #if defined(__AVX2__)
    {
        __m256i mask_space = _mm256_set1_epi8(' ');
        __m256i mask_tab = _mm256_set1_epi8('\t');
        __m256i mask_newln = _mm256_set1_epi8('\n');
        __m256i mask_return = _mm256_set1_epi8('\r');

        while (end - 32 >= start) {
            // For current block of 32 characters...

            // Load in 256 bits (32 characters) from current 'end' index into memory
            __m256i vec_str = _mm256_loadu_si256((__m256i *)((*string)->data + (end - 32)));
            
            // Create masks for each whitespace character
            __m256i mask_is_space = _mm256_cmpeq_epi8(vec_str, mask_space);
            __m256i mask_is_tab = _mm256_cmpeq_epi8(vec_str, mask_tab);
            __m256i mask_is_newln = _mm256_cmpeq_epi8(vec_str, mask_newln);
            __m256i mask_is_return = _mm256_cmpeq_epi8(vec_str, mask_return);

            // Combine masks to find all whitespace
            __m256i mask_all_whitespace = _mm256_or_si256(
                _mm256_or_si256(mask_is_space, mask_is_tab), _mm256_or_si256(mask_is_newln, mask_is_return)
            );

            Int32 matched_bits = _mm256_movemask_epi8(mask_all_whitespace);
            if (matched_bits == (Int32)0xffffffff) {
                // All characters are whitespace...

                // Roll back to previous block (32 characters)
                end -= 32;
            }
            else {
                // Otherwise...

                // Locate the first non-whitespace character
                Int32 non_match_index = _container_string_clz(~matched_bits);
                Int32 actual_index = end - non_match_index;

                // Trim to that character
                end = actual_index;

                // Exit the loop
                break;
            }
        }
    }
    #endif

    #if defined(__SSE__)
    {
        __m128i mask_space = _mm_set1_epi8(' ');
        __m128i mask_tab = _mm_set1_epi8('\t');
        __m128i mask_newln = _mm_set1_epi8('\n');
        __m128i mask_return = _mm_set1_epi8('\r');
        
        while (end - 16 >= start) {
            // For current block of 16 characters...

            // Load in 128 bits (16 characters) from current 'end' index into memory
            __m128i vec_str = _mm_loadu_si128((__m128i *)((*string)->data + (end - 16)));
            
            // Create masks for each whitespace character
            __m128i mask_is_space = _mm_cmpeq_epi8(vec_str, mask_space);
            __m128i mask_is_tab = _mm_cmpeq_epi8(vec_str, mask_tab);
            __m128i mask_is_newln = _mm_cmpeq_epi8(vec_str, mask_newln);
            __m128i mask_is_return = _mm_cmpeq_epi8(vec_str, mask_return);
            
            // Combine masks to find all whitespace
            __m128i mask_all_whitespace = _mm_or_si128(
                _mm_or_si128(mask_is_space, mask_is_tab), _mm_or_si128(mask_is_newln, mask_is_return)
            );
            
            Int16 matched_bits = (Int16)_mm_movemask_epi8(mask_all_whitespace);
            if (matched_bits == (Int16)0xffff) {
                // All characters are whitespace...

                // Roll back to previous block (16 characters)
                end -= 16;
            }
            else {
                // Otherwise...

                // Locate the first non-whitespace character
                Int32 non_match_index = _container_string_ctz(~matched_bits);
                Int32 actual_index = end - non_match_index;

                // Trim to that character
                end = actual_index;

                // Exit the loop
                break;
            }
        }
    }
    #endif

    // Handle remaining bytes
    // Also acts as fallback implementation, in case of no SIMD support
    while (end > start) {
        if (
            (*string)->data[end - 1] != ' ' && 
            (*string)->data[end - 1] != '\t' && 
            (*string)->data[end - 1] != '\n' && 
            (*string)->data[end - 1] != '\r'
        ) {
            // Found non-whitespace character...

            // Done...
            // Exit the loop
            break;
        }

        // Roll back to previous character
        --end;
    }

    // Update string size
    (*string)->size = end - start;

    // Null-terminate string
    (*string)->data[(*string)->size] = '\0';

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_trim(String *string) {
    return container_string_trim_left(string) | container_string_trim_right(string);
}

KYRA_ENGINE_API ContainerResult container_string_filter_char(String *string, const Char c, const Bool keep) {
    if (!string) return CONTAINER_STRING_ERROR_REF_STRING_NULL;

    // Return if requested to remove null-terminator character
    if (c == '\0') return CONTAINER_SUCCESS;

    ByteSize size = (*string)->size;
    ByteSize read_index = 0;
    ByteSize write_index = 0;

    #if defined(__AVX2__)
    {
        // Load in character to compare with
        __m256i vec_char = _mm256_set1_epi8(c);
        
        while (read_index + 32 <= size) {
            // For current block of 32 characters...

            // Load in 256 bits (32 characters) from current 'read_index' into memory
            __m256i vec_str = _mm256_loadu_si256((__m256i *)((*string)->data + read_index));
            
            // Compare with character
            __m256i mask_cmp = _mm256_cmpeq_epi8(vec_str, vec_char);
            
            Int32 matched_bits = _mm256_movemask_epi8(mask_cmp);
            if (keep) {
                // We are requested to keep matched characters and remove the rest...

                if (matched_bits == 0) {
                    // No matching bytes... 

                    // Advance 'read_index' to next block (32 characters)
                    read_index += 32;
                    continue;
                }
                
                while (matched_bits != 0) {
                    Int32 matched_index = _container_string_ctz(matched_bits);
                    
                    // Write to string
                    // Advance 'write_index' to next block (16 characters)
                    (*string)->data[write_index++] = (*string)->data[read_index + matched_index];
                    
                    // Remove matched bit
                    matched_bits &= ~(1 << matched_index);
                }
                
            } else {
                // Otherwise (remove matched characters and keep the rest)...

                if (matched_bits == 0) {
                    // In case of no matching characters, write all 32
                    // Advance 'write_index' to next block (32 characters)
                    memcpy((*string)->data + write_index, (*string)->data + read_index, 32);
                    write_index += 32;
                    
                    // Advance 'read_index' to next block (32 characters)
                    read_index += 32;
                    continue;
                }
                
                // If there are matching characters...
                for (ByteSize i = 0; i < 32; ++i) {
                    // For each character index inside block...

                    // Skip if character matches
                    if (matched_bits & (1 << i)) continue;
                    
                    // Write non-matching character to string
                    // Advance 'write_index' to next character 
                    (*string)->data[write_index++] = (*string)->data[read_index + i];
                }
            }
            
            // Advance 'read_index' to next block (32 characters)
            read_index += 32;
        }
    }
    #endif

    #if defined(__SSE__)
    {
        // Load in character to compare with
        __m128i vec_char = _mm_set1_epi8(c);
        
        while (read_index + 16 <= size) {
            // For current block of 16 characters...

            // Load in 128 bits (16 characters) from current 'read_index' into memory
            __m128i vec_str = _mm_loadu_si128((__m128i *)((*string)->data + read_index));
            
            // Compare with character
            __m128i mask_cmp = _mm_cmpeq_epi8(vec_str, vec_char);
            
            Int16 matched_bits = (Int16)_mm_movemask_epi8(mask_cmp);
            if (keep) {
                // We are requested to keep matched characters and remove the rest...

                if (matched_bits == 0) {
                    // No matching bytes... 

                    // Advance 'read_index' to next block (16 characters)
                    read_index += 16;
                    continue;
                }
                
                while (matched_bits != 0) {
                    Int32 matched_index = _container_string_ctz(matched_bits);
                    
                    // Write to string
                    // Advance 'write_index' to next block (16 characters)
                    (*string)->data[write_index++] = (*string)->data[read_index + matched_index];
                    
                    // Remove matched bit
                    matched_bits &= ~(1 << matched_index);
                }
                
            } else {
                // Otherwise (remove matched characters and keep the rest)...

                if (matched_bits == 0) {
                    // In case of no matching characters, write all 16
                    // Advance 'write_index' to next block (16 characters)
                    memcpy((*string)->data + write_index, (*string)->data + read_index, 16);
                    write_index += 16;
                    
                    // Advance 'read_index' to next block (16 characters)
                    read_index += 16;
                    continue;
                }
                
                // If there are matching characters...
                for (ByteSize i = 0; i < 16; ++i) {
                    // For each character index inside block...

                    // Skip if character matches
                    if (matched_bits & (1 << i)) continue;
                    
                    // Write non-matching character to string
                    // Advance 'write_index' to next character 
                    (*string)->data[write_index++] = (*string)->data[read_index + i];
                }
            }
            
            // Advance 'read_index' to next block (16 characters)
            read_index += 16;
        }
    }
    #endif

    // Handle remaining bytes
    // Also acts as fallback to scalar implementation, in case of no SIMD support
    while (read_index < size) {
        if (keep) {
            // Keep matched characters and remove the rest...

            if ((*string)->data[read_index] == c) { 
                // Character matched...
                
                // Write matched character to string
                // Advance 'write_index' to next character 
                (*string)->data[write_index++] = (*string)->data[read_index];
            }
        } else {
            // Remove matched characters and keep the rest...

            if ((*string)->data[read_index] != c) { 
                // Character not matched...

                // Write non-matched character to string
                // Advance 'write_index' to next character 
                (*string)->data[write_index++] = (*string)->data[read_index];
            }
        }
        
        // Advance 'read_index' to next character 
        ++read_index;
    }

    // Null-terminate string
    (*string)->data[write_index] = '\0';

    // Update string size
    (*string)->size = write_index;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_substring(const String string, const ByteSize index, const ByteSize size, String *out_new_substring) {
    if (!string) return CONTAINER_STRING_ERROR_STRING_NULL;
    if (index >= string->size) return CONTAINER_STRING_ERROR_INDEX_OUT_OF_BOUNDS;
    if (!out_new_substring) return CONTAINER_STRING_ERROR_REF_OUT_NEW_SUBSTRING_NULL;

    // Construct empty string for new substring if specified size is zero
    if (size == 0) return container_string_construct_empty(out_new_substring);

    // Validate specified size
    ByteSize valid_size = size > (string->size - index) ? (string->size - index) : size;

    // Position for substring
    ConstStr addr_start = string->data + index;

    // Construct new string
    {
        // Compute capacity of string (using aligned size)
        // Multiply by resize factor to reduce resizings
        ByteSize capacity = KYRA_APPLY_MEMORY_ALIGNMENT((ByteSize)((Flt32)valid_size * KYRA_CONTAINER_RESIZE_RATIO), KYRA_MEMORY_ALIGNMENT_SIZE);

        // Allocate memory for string
        ByteSize alloc_size = sizeof(struct Container_String) + capacity;
        ByteSize mem_size = 0;
        if (memory_zone_allocate("string", alloc_size, (VoidPtr *)out_new_substring, &mem_size) != MEMORY_ZONE_SUCCESS)
            return CONTAINER_STRING_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STRING;

        // Copy value to string
        (*out_new_substring)->data = (Str)KYRA_APPLY_MEMORY_ALIGNMENT((UIntPtr)(*out_new_substring) + sizeof(struct Container_String), KYRA_MEMORY_ALIGNMENT_SIZE);
        memcpy((*out_new_substring)->data, addr_start, valid_size);
        (*out_new_substring)->data[valid_size] = '\0';

        // Initialise remaining properties
        (*out_new_substring)->size = valid_size;
        (*out_new_substring)->capacity = capacity;
        (*out_new_substring)->memory_size = mem_size;
    }

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_equals(const String left, ConstStr right, Bool *out_result) {
    if (!left || !right) return CONTAINER_STRING_ERROR_STRING_NULL;

    ByteSize left_size = left->size;
    ByteSize right_size = strlen(right);

    // Return if comparing strings do not even have same size 
    if (left_size != right_size) {
        // Save to ref
        if (out_result) *out_result = false;
        
        return CONTAINER_STRING_ERROR_LEFT_AND_RIGHT_SIZES_MISMATCHED;
    }

    ByteSize index = 0;
    Bool matched = true;

    #if defined(__AVX2__)
    {
        __m256i vec_left, vec_right;
        
        while ((index + 32 <= left_size) && (index + 32 <= right_size)) {
            // For current block of 32 characters...

            // Load in 256 bits (32 characters) from current index into memory
            vec_left = _mm256_loadu_si256((__m256i *)(left->data + index));
            vec_right = _mm256_loadu_si256((__m256i *)(right + index));
            
            if (_mm256_testz_si256(vec_left, vec_right)) {
                // 'testz' returns 1...
                // It means the vectors are not equal
                
                // Set 'matched' false and exit 
                matched = false;
                break;
            }
            
            // Advance to next block (32 characters)
            index += 32;
        }
    }
    #endif 

    #if defined(__SSE__)
    {
        __m128i vec_left, vec_right;
        
        while ((index + 16 <= left_size) && (index + 16 <= right_size)) {
            // For current block of 16 characters...

            // Load in 128 bits (16 characters) from current index into memory
            vec_left = _mm_loadu_si128((__m128i *)(left->data + index));
            vec_right = _mm_loadu_si128((__m128i *)(right + index));
            
            if (_mm_movemask_epi8(_mm_cmpeq_epi8(vec_left, vec_right)) != 0xffff) {
                // Vectors are not equal...

                // Set 'matched' false and exit
                matched = false;
                break;
            }
            
            // Advance to next block (16 characters)
            index += 16;
        }
    }
    #endif 

    // Handle remaining bytes
    // Also acts as fallback to scalar implementation, in case of no SIMD support
    while ((index < left->size) && (index < right_size)) {
        if (left->data[index] != right[index]) {
            // Characters are not equal...

            // Set 'matched' false and exit
            matched = false;
            break;
        }
        
        // Advance to next character
        ++index;
    }
    
    // Save to ref
    if (out_result) *out_result = matched;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_equals_string(const String left, const String right, Bool *out_result) {
    return container_string_equals(left, right->data, out_result);
}

KYRA_ENGINE_API ContainerResult container_string_search(const String string, ConstStr substr, Int32 *out_index) {
    if (!string) return CONTAINER_STRING_ERROR_STRING_NULL;
    if (!substr) return CONTAINER_STRING_ERROR_SUBSTRING_NULL;

    ByteSize index = 0;
    ByteSize string_size = string->size;
    ByteSize substring_size = strlen(substr);
    
    if (substring_size > string_size) {
        // Substring is longer than string...

        // Save to ref
        if (out_index) *out_index = -1;
        
        return CONTAINER_STRING_ERROR_SUBSTRING_LONGER_THAN_STRING;
    }

    #if defined(__AVX2__)
    {
        while (index + 32 <= string_size) {
            // For current block of 32 characters...

            // Load in 256 bits (32 characters) from current index into memory
            __m256i vec_str = _mm256_loadu_si256((__m256i *)(string->data + index));
            
            // Check if the first character of the substring matches
            __m256i substr_first = _mm256_set1_epi8(substr[0]);
            __m256i mask_cmp = _mm256_cmpeq_epi8(vec_str, substr_first);
            
            Int32 match_bits = _mm256_movemask_epi8(mask_cmp);
            while (match_bits) {
                // Get index of least significant bit to find the first occurrence of substring
                Int32 matched_index = _container_string_ctz(match_bits);
                
                ByteSize start = index + matched_index;
                if (start + substring_size <= string_size) {
                    if (!strncmp(string->data + start, substr, substring_size)) {
                        // Substrings matched...

                        // Save to ref
                        if (out_index) *out_index = start;

                        return CONTAINER_SUCCESS;
                    }
                }
                
                // Remove the least significant bit
                match_bits &= (match_bits - 1);
            }
            
            // Advance to next block (32 characters)
            index += 32;
        }
    }
    #endif

    #if defined(__SSE__)
    {
        while (index + 16 <= string_size) {
            __m128i vec_str = _mm_loadu_si128((__m128i *)(string->data + index));
            
            // Check if the first character of the substring matches
            __m128i substr_first = _mm_set1_epi8(substr[0]);
            __m128i mask_cmp = _mm_cmpeq_epi8(vec_str, substr_first);
            
            Int32 match_bits = _mm_movemask_epi8(mask_cmp);
            while (match_bits) {
                // Get index of least significant bit to find the first occurrence of substring
                Int32 matched_index = _container_string_ctz(match_bits);
                
                ByteSize start = index + matched_index;
                if (start + substring_size <= string_size) {
                    if (!strncmp(string->data + start, substr, substring_size)) {
                        // Substring matched...

                        // Save to ref
                        if (out_index) *out_index = start;
                        
                        return CONTAINER_SUCCESS;
                    }
                }
                
                // Remove the least significant bit
                match_bits &= (match_bits - 1);
            }
            
            // Advance to next block (16 characters)
            index += 16;
        }
    }
    #endif

    // Handle remaining bytes
    // Also acts as fallback to scalar implementation, in case of no SIMD support
    while (index < string_size) {
        if (string->data[index] == substr[0]) {
            if (!strncmp(string->data + index, substr, substring_size)) {
                *out_index = index;
                return CONTAINER_SUCCESS;
            }
        }

        // Advance to next character
        ++index;
    }

    // Save to ref
    if (out_index) *out_index = -1;

    return CONTAINER_STRING_ERROR_FAILED_TO_FIND_ANY_MATCH;
}

KYRA_ENGINE_API ContainerResult container_string_search_string(const String string, const String substr, Int32 *out_index) {
    return container_string_search(string, substr->data, out_index);
}

KYRA_ENGINE_API ContainerResult container_string_contains(const String string, ConstStr substr, Bool *out_result) {
    if (!string) return CONTAINER_STRING_ERROR_STRING_NULL;
    if (!substr) return CONTAINER_STRING_ERROR_SUBSTRING_NULL;

    // Search for the substring
    Int32 search_index = 0;
    ContainerResult search_result = container_string_search(string, substr, &search_index);

    // Save to ref
    if (out_result) *out_result = (search_index != -1);

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_contains_string(const String string, const String substr, Bool *out_result) {
    return container_string_contains(string, substr->data, out_result);
}

KYRA_ENGINE_API ContainerResult container_string_begins_with(const String string, ConstStr prefix, Bool *out_result) {
    if (!string) return CONTAINER_STRING_ERROR_STRING_NULL;
    if (!prefix) return CONTAINER_STRING_ERROR_PREFIX_NULL;

    ByteSize string_size = string->size;
    ByteSize prefix_size = strlen(prefix);
    if (prefix_size > string->size) return CONTAINER_STRING_ERROR_PREFIX_LONGER_THAN_STRING;

    ByteSize index = 0;
    Bool matched = true;

    #if defined(__AVX2__)
    {
        __m256i vec_str, vec_prefix;
        
        while ((index + 32 <= string_size) && (index + 32 <= prefix_size)) {
            // For current block of 32 characters...

            // Load in 256 bits (32 characters) from current index into memory
            vec_str = _mm256_loadu_si256((__m256i *)(string->data + index));
            vec_prefix = _mm256_loadu_si256((__m256i *)(prefix + index));
            
            if (_mm256_testz_si256(vec_str, vec_prefix)) {
                // 'testz' returns 1...
                // It means the vectors are not equal
                
                // Set 'matched' false and exit 
                matched = false;
                break;
            }
            
            // Advance to next block (32 characters)
            index += 32;
        }
    }
    #endif 

    #if defined(__SSE__)
    {
        __m128i vec_str, vec_prefix;
        
        while ((index + 16 <= string_size) && (index + 16 <= prefix_size)) {
            // For current block of 16 characters...

            // Load in 128 bits (16 characters) from current index into memory
            vec_str = _mm_loadu_si128((__m128i *)(string->data + index));
            vec_prefix = _mm_loadu_si128((__m128i *)(prefix + index));
            
            if (_mm_movemask_epi8(_mm_cmpeq_epi8(vec_str, vec_prefix)) != 0xffff) {
                // Vectors are not equal...

                // Set 'matched' false and exit
                matched = false;
                break;
            }
            
            // Advance to next block (16 characters)
            index += 16;
        }
    }
    #endif 

    // Handle remaining bytes
    // Also acts as fallback to scalar implementation, in case of no SIMD support
    while ((index < string_size) && (index < prefix_size)) {
        if (string->data[index] != prefix[index]) {
            // Characters are not equal...

            // Set 'matched' false and exit
            matched = false;
            break;
        }
        
        // Advance to next character
        ++index;
    }
    
    // Save to ref
    if (out_result) *out_result = matched;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_begins_with_string(const String string, const String prefix, Bool *out_result) {
    return container_string_begins_with(string, prefix->data, out_result);
}

KYRA_ENGINE_API ContainerResult container_string_ends_with(const String string, ConstStr suffix, Bool *out_result) {
    if (!string) return CONTAINER_STRING_ERROR_STRING_NULL;
    if (!suffix) return CONTAINER_STRING_ERROR_SUFFIX_NULL;

    ByteSize string_size = string->size;
    ByteSize suffix_size = strlen(suffix);
    if (suffix_size > string->size) return CONTAINER_STRING_ERROR_SUFFIX_LONGER_THAN_STRING;

    ByteSize index = 0;
    Bool matched = true;
    Str addr_cmp = string->data + string->size - suffix_size;

    #if defined(__AVX2__)
    {
        __m256i vec_str, vec_suffix;
        
        while (index += 32 <= suffix_size) {
            // For current block of 32 characters...

            // Load in 256 bits (32 characters) from current index into memory
            vec_str = _mm256_loadu_si256((__m256i *)(addr_cmp + index));
            vec_suffix = _mm256_loadu_si256((__m256i *)(suffix + index));
            
            if (_mm256_testz_si256(vec_str, vec_suffix)) {
                // 'testz' returns 1...
                // It means the vectors are not equal
                
                // Set 'matched' false and exit 
                matched = false;
                break;
            }
            
            // Advance to block (32 characters)
            index += 32;
        }
    }
    #endif

    #if defined(__SSE__)
    {
        __m128i vec_str, vec_suffix;
        
        while (index + 16 <= suffix_size) {
            // For current block of 16 characters...

            // Load in 128 bits (16 characters) from current index into memory
            vec_str = _mm_loadu_si128((__m128i *)(addr_cmp + index));
            vec_suffix = _mm_loadu_si128((__m128i *)(suffix + index));
            
            if (_mm_movemask_epi8(_mm_cmpeq_epi8(vec_str, vec_suffix)) != 0xffff) {
                // Vectors are not equal...

                // Set 'matched' false and exit
                matched = false;
                break;
            }
            
            // Advance to next block (16 characters)
            index += 16;
        }
    }
    #endif
    
    // Handle remaining bytes
    // Also acts as fallback to scalar implementation, in case of no SIMD support
    while (index < suffix_size) {
        if (addr_cmp[index] != suffix[index]) {
            // Characters are not equal...

            // Set 'matched' false and exit
            matched = false;
            break;
        }
        
        // Advance to next character
        ++index;
    }

    // Save to ref
    if (out_result) *out_result = matched;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_ends_with_string(const String string, const String suffix, Bool *out_result) {
    return container_string_ends_with(string, suffix->data, out_result);
}

KYRA_ENGINE_API ContainerResult container_string_to_cstr(const String string, ConstStr *out_cstr) {
    if (!string) return CONTAINER_STRING_ERROR_STRING_NULL;

    // Save to ref
    if (out_cstr) *out_cstr = string->data;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_size(const String string, ByteSize *out_size) {
    if (!string) return CONTAINER_STRING_ERROR_STRING_NULL;

    // Save to ref
    if (out_size) *out_size = string->size;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_capacity(const String string, ByteSize *out_capacity) {
    if (!string) return CONTAINER_STRING_ERROR_STRING_NULL;

    // Save to ref
    if (out_capacity) *out_capacity = string->capacity;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_string_is_empty(const String string, Bool *out_result) {
    if (!string) return CONTAINER_STRING_ERROR_STRING_NULL;

    // Save to ref
    if (out_result) *out_result = (string->size == 0);

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ConstStr container_string_get_cstr(const String string) {
    if (!string) return NULL;
    
    return string->data;
}

KYRA_ENGINE_API ByteSize container_string_get_size(const String string) {
    if (!string) return 0;
    
    return string->size;
}

KYRA_ENGINE_API ByteSize container_string_get_capacity(const String string) {
    if (!string) return 0;
    
    return string->capacity;
}

KYRA_ENGINE_API Bool container_string_get_is_empty(const String string) {
    if (!string) return true;

    return (string->size == 0);
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
        case CONTAINER_STRING_ERROR_SUBSTRING_LONGER_THAN_STRING:                           return "CONTAINER_STRING_ERROR_SUBSTRING_LONGER_THAN_STRING";
        case CONTAINER_STRING_ERROR_PREFIX_NULL:                                            return "CONTAINER_STRING_ERROR_PREFIX_NULL";
        case CONTAINER_STRING_ERROR_PREFIX_LONGER_THAN_STRING:                              return "CONTAINER_STRING_ERROR_PREFIX_LONGER_THAN_STRING";
        case CONTAINER_STRING_ERROR_SUFFIX_NULL:                                            return "CONTAINER_STRING_ERROR_SUFFIX_NULL";
        case CONTAINER_STRING_ERROR_SUFFIX_LONGER_THAN_STRING:                              return "CONTAINER_STRING_ERROR_SUFFIX_LONGER_THAN_STRING";
        case CONTAINER_STRING_ERROR_INDEX_OUT_OF_BOUNDS:                                    return "CONTAINER_STRING_ERROR_INDEX_OUT_OF_BOUNDS";
        case CONTAINER_STRING_ERROR_LEFT_AND_RIGHT_SIZES_MISMATCHED:                        return "CONTAINER_STRING_ERROR_LEFT_AND_RIGHT_SIZES_MISMATCHED";
        case CONTAINER_STRING_ERROR_REF_OUT_STRING_NULL:                                    return "CONTAINER_STRING_ERROR_REF_OUT_STRING_NULL";
        case CONTAINER_STRING_ERROR_REF_STRING_NULL:                                        return "CONTAINER_STRING_ERROR_REF_STRING_NULL";
        case CONTAINER_STRING_ERROR_REF_STRING_NOT_VALID:                                   return "CONTAINER_STRING_ERROR_REF_STRING_NOT_VALID";
        case CONTAINER_STRING_ERROR_REF_OUT_NEW_SUBSTRING_NULL:                             return "CONTAINER_STRING_ERROR_REF_OUT_NEW_SUBSTRING_NULL";
        case CONTAINER_STRING_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STRING:                   return "CONTAINER_STRING_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STRING";
        case CONTAINER_STRING_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STRING:                  return "CONTAINER_STRING_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STRING";
        case CONTAINER_STRING_ERROR_FAILED_TO_FIND_ANY_MATCH:                               return "CONTAINER_STRING_ERROR_FAILED_TO_FIND_ANY_MATCH";
        
        default:                                                                            return "UNKNOWN_CONTAINER_RESULT";
    }
}


