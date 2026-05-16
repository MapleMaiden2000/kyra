#include "kyra/core/containers/array/array.h"

#include <memory.h>
#include <stdlib.h>
#include <string.h>

#include "kyra/core/memory/zone/memory_zone.h"


// Internal structure ---------------------------------------------- //

struct Container_Array {
    VoidPtr     data;
    ByteSize    data_size;

    ByteSize    size;
    ByteSize    capacity;
    
    // For allocations/deallocations
    ByteSize    memory_size;
};


// Helper functions ------------------------------------------------ //

static ContainerResult _container_array_resize(Array *array, const ByteSize new_capacity) {
    if (!array || !(*array)) return CONTAINER_ARRAY_HELPER_ERROR_ARRAY_NULL;
    if (new_capacity == 0) return CONTAINER_ARRAY_HELPER_ERROR_NEW_CAPACITY_ZERO;

    Array old_array = *array;
    Array new_array = NULL;

    // Calculate allocation size
    ByteSize pool_size = new_capacity * old_array->data_size;
    ByteSize alloc_size = KYRA_APPLY_MEMORY_ALIGNMENT(sizeof(struct Container_Array) + pool_size, KYRA_MEMORY_ALIGNMENT_SIZE);
    ByteSize mem_size = 0;

    // Allocate new array
    if (memory_zone_allocate("containers", alloc_size, (VoidPtr *)&new_array, &mem_size) != MEMORY_ZONE_SUCCESS)
        return CONTAINER_ARRAY_HELPER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_NEW_ARRAY;

    // Set new array properties
    {
        new_array->data_size = old_array->data_size;
        new_array->size = old_array->size;
        new_array->capacity = new_capacity;
        new_array->memory_size = mem_size;
        new_array->data = (VoidPtr)KYRA_APPLY_MEMORY_ALIGNMENT((UIntPtr)new_array + sizeof(struct Container_Array), KYRA_MEMORY_ALIGNMENT_SIZE);
    }

    // Copy old array data over
    memcpy(new_array->data, old_array->data, old_array->size * old_array->data_size);

    // Deallocate old array
    memory_zone_deallocate("containers", (VoidPtr)old_array, old_array->memory_size);
    
    // Save new map to ref
    *array = new_array;

    return CONTAINER_SUCCESS;
}


// API functions --------------------------------------------------- //

KYRA_ENGINE_API ContainerResult container_array_construct(const ByteSize data_size, Array *out_array) {
    if (data_size == 0) return CONTAINER_ARRAY_ERROR_DATA_SIZE_ZERO;
    if (!out_array) return CONTAINER_ARRAY_ERROR_REF_OUT_ARRAY_NULL;

    // Calculate allocation size
    ByteSize pool_size = KYRA_CONTAINER_DEFAULT_CAPACITY * data_size;
    ByteSize alloc_size = KYRA_APPLY_MEMORY_ALIGNMENT(sizeof(struct Container_Array) + pool_size, KYRA_MEMORY_ALIGNMENT_SIZE);
    ByteSize mem_size = 0;

    // Allocate array
    if (memory_zone_allocate("containers", alloc_size, (VoidPtr *)out_array, &mem_size) != MEMORY_ZONE_SUCCESS)
        return CONTAINER_ARRAY_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_ARRAY;

    // Initialise array properties
    {
        (*out_array)->data_size = data_size;
        (*out_array)->size = 0;
        (*out_array)->capacity = KYRA_CONTAINER_DEFAULT_CAPACITY;
        (*out_array)->memory_size = mem_size;
        (*out_array)->data = (VoidPtr)KYRA_APPLY_MEMORY_ALIGNMENT((UIntPtr)(*out_array) + sizeof(struct Container_Array), KYRA_MEMORY_ALIGNMENT_SIZE);
    }

    // Set array elements to zero
    memset((*out_array)->data, 0, data_size * KYRA_CONTAINER_DEFAULT_CAPACITY);

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_array_destruct(Array *array) {
    if (!array || !(*array)) return CONTAINER_ARRAY_ERROR_REF_ARRAY_NULL;

    // Deallocate array
    if (memory_zone_deallocate("containers", (VoidPtr)(*array), (*array)->memory_size) != MEMORY_ZONE_SUCCESS)
        return CONTAINER_ARRAY_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_ARRAY;

    // Set to NULL
    *array = NULL;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_array_push(Array *array, const VoidPtr data) {
    if (!array || !(*array)) return CONTAINER_ARRAY_ERROR_REF_ARRAY_NULL;
    if (!data) return CONTAINER_ARRAY_ERROR_DATA_NULL;
    
    if ((*array)->size == (*array)->capacity) {
        // Array is full...

        // Resize the array
        ContainerResult resize = _container_array_resize(array, (*array)->capacity * 2);
        if (resize != CONTAINER_SUCCESS) return resize;
    }

    // Copy data to array
    memcpy(
        (VoidPtr)((UIntPtr)((*array)->data) + ((*array)->size * (*array)->data_size)), 
        data, 
        (*array)->data_size
    );

    // Increment array size
    (*array)->size++;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_array_pop(Array *array) {
    if (!array || !(*array)) return CONTAINER_ARRAY_ERROR_REF_ARRAY_NULL;

    // Clear last element
    memset(
        (VoidPtr)((UIntPtr)((*array)->data) + ((*array)->size - 1) * (*array)->data_size),
        0,
        (*array)->data_size
    );

    // Decrement array size as removing last element
    (*array)->size--;
 
    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_array_insert(Array *array, const ByteSize index, const VoidPtr data) {
    if (!array || !(*array)) return CONTAINER_ARRAY_ERROR_REF_ARRAY_NULL;
    if (index > (*array)->size) return CONTAINER_ARRAY_ERROR_INDEX_OUT_OF_BOUNDS;
    if (!data) return CONTAINER_ARRAY_ERROR_DATA_NULL;
    
    // If index is array size (indicating after last element), use push
    if (index == (*array)->size) return container_array_push(array, data);

    if ((*array)->size == (*array)->capacity) {
        // Array is full...

        // Resize the array
        ContainerResult resize = _container_array_resize(array, (*array)->capacity * 2);
        if (resize != CONTAINER_SUCCESS) return resize;
    }

    // Shift elements to the right
    {
        UIntPtr shift_dest = (UIntPtr)(*array)->data + ((index + 1) * (*array)->data_size);
        UIntPtr shift_src = (UIntPtr)(*array)->data + (index * (*array)->data_size);
        ByteSize shift_size = ((*array)->size - index) * (*array)->data_size;
        memmove((VoidPtr)shift_dest, (VoidPtr)shift_src, shift_size);
    }

    // Copy data to array
    memcpy(
        (VoidPtr)((UIntPtr)((*array)->data) + (index * (*array)->data_size)), 
        data, 
        (*array)->data_size
    );

    // Increment array size
    (*array)->size++;
    
    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_array_remove(Array *array, const VoidPtr data, const Bool remove_all) {
    if (!array || !(*array)) return CONTAINER_ARRAY_ERROR_REF_ARRAY_NULL;
    if (!data) return CONTAINER_ARRAY_ERROR_DATA_NULL;

    for (ByteSize index = 0; index < (*array)->size; ++index) {
        VoidPtr elem = (VoidPtr)((UIntPtr)((*array)->data) + (index * (*array)->data_size));

        // For every element...

        // Check if matched
        if (!memcmp(elem, data, (*array)->data_size)) {
            // Element matched with specified data...

            // Shift elements to the left
            {
                UIntPtr shift_dest = (UIntPtr)(*array)->data + (index * (*array)->data_size);
                UIntPtr shift_src = (UIntPtr)(*array)->data + ((index + 1) * (*array)->data_size);
                ByteSize shift_size = ((*array)->size - index - 1) * (*array)->data_size;
                memmove((VoidPtr)shift_dest, (VoidPtr)shift_src, shift_size);
            }

            // Clear last element
            memset(
                (VoidPtr)((UIntPtr)((*array)->data) + ((*array)->size - 1) * (*array)->data_size),
                0,
                (*array)->data_size
            );

            // Decrement array size
            (*array)->size--;

            // If only one element should be removed, exit the loop since we are done
            if (!remove_all) break;

            // Decrement index to check new element at the same position
            index--;
        }
    }

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_array_remove_at(Array *array, const ByteSize index) {
    if (!array || !(*array)) return CONTAINER_ARRAY_ERROR_REF_ARRAY_NULL;
    if (index > (*array)->size) return CONTAINER_ARRAY_ERROR_INDEX_OUT_OF_BOUNDS;

    // If index is array size (indicating after last element), use pop
    if (index == (*array)->size - 1) return container_array_pop(array);

    // Shift elements to the left
    {
        UIntPtr shift_dest = (UIntPtr)(*array)->data + (index * (*array)->data_size);
        UIntPtr shift_src = (UIntPtr)(*array)->data + ((index + 1) * (*array)->data_size);
        ByteSize shift_size = ((*array)->size - index - 1) * (*array)->data_size;
        memmove((VoidPtr)shift_dest, (VoidPtr)shift_src, shift_size);
    }

    // Clear last element
    memset(
        (VoidPtr)((UIntPtr)((*array)->data) + ((*array)->size - 1) * (*array)->data_size),
        0,
        (*array)->data_size
    );

    // Decrement array size
    (*array)->size--;
    
    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_array_clear(Array *array) {
    if (!array || !(*array)) return CONTAINER_ARRAY_ERROR_REF_ARRAY_NULL;

    // Clear all elements
    memset((*array)->data, 0, (*array)->size * (*array)->data_size);

    // Set array size to zero
    (*array)->size = 0;

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ContainerResult container_array_sort(Array *array, Int32 (*compare)(const VoidPtr left, const VoidPtr right)) {
    if (!array || !(*array)) return CONTAINER_ARRAY_ERROR_REF_ARRAY_NULL;
    if (!compare) return CONTAINER_ARRAY_ERROR_INVALID_COMPARE_FUNCPTR;

    // Sort array
    if ((*array)->size > 1) 
        qsort(
            (*array)->data, 
            (*array)->size, 
            (*array)->data_size, 
            (int (*)(const void *, const void *))compare
        );

    return CONTAINER_SUCCESS;
}

KYRA_ENGINE_API ByteSize container_array_data_size(const Array array) {
    if (!array) return 0;

    return array->data_size;
}

KYRA_ENGINE_API ByteSize container_array_size(const Array array) {
if (!array) return 0;
    
    return array->size;
}

KYRA_ENGINE_API ByteSize container_array_capacity(const Array array) {
    if (!array) return 0;
    
    return array->capacity;
}

KYRA_ENGINE_API VoidPtr container_array_get(const Array array) {
    if (!array) return NULL;
    
    return array->data;
}

KYRA_ENGINE_API VoidPtr container_array_get_at(const Array array, const ByteSize index) {
    if (!array || (index > array->size)) return NULL;

    return (VoidPtr)((UIntPtr)(array->data) + (index * array->data_size));
}

KYRA_ENGINE_API Bool container_array_is_empty(const Array array) {
    if (!array) return true;

    return (array->size == 0);
}

KYRA_ENGINE_API ConstStr container_array_result_to_string(const ContainerResult result) {
    switch (result) {
        case CONTAINER_SUCCESS:                                                             return "CONTAINER_SUCCESS";

        case CONTAINER_ARRAY_HELPER_ERROR_ARRAY_NULL:                                       return "CONTAINER_ARRAY_HELPER_ERROR_ARRAY_NULL";
        case CONTAINER_ARRAY_HELPER_ERROR_NEW_CAPACITY_ZERO:                                return "CONTAINER_ARRAY_HELPER_ERROR_NEW_CAPACITY_ZERO";
        case CONTAINER_ARRAY_HELPER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_NEW_ARRAY:          return "CONTAINER_ARRAY_HELPER_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_NEW_ARRAY";
        
        case CONTAINER_ARRAY_ERROR_DATA_SIZE_ZERO:                                          return "CONTAINER_ARRAY_ERROR_DATA_SIZE_ZERO";
        case CONTAINER_ARRAY_ERROR_DATA_NULL:                                               return "CONTAINER_ARRAY_ERROR_DATA_NULL";
        case CONTAINER_ARRAY_ERROR_INDEX_OUT_OF_BOUNDS:                                     return "CONTAINER_ARRAY_ERROR_INDEX_OUT_OF_BOUNDS";
        case CONTAINER_ARRAY_ERROR_INVALID_COMPARE_FUNCPTR:                                 return "CONTAINER_ARRAY_ERROR_INVALID_COMPARE_FUNCPTR";
        case CONTAINER_ARRAY_ERROR_REF_OUT_ARRAY_NULL:                                      return "CONTAINER_ARRAY_ERROR_REF_OUT_ARRAY_NULL";
        case CONTAINER_ARRAY_ERROR_REF_ARRAY_NULL:                                          return "CONTAINER_ARRAY_ERROR_REF_ARRAY_NULL";
        case CONTAINER_ARRAY_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_ARRAY:                     return "CONTAINER_ARRAY_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_ARRAY";
        case CONTAINER_ARRAY_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_ARRAY:                    return "CONTAINER_ARRAY_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_ARRAY";

        default:                                                                            return "UNKNOWN_CONTAINER_RESULT";
    }
}



