#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/types.h"
#include "kyra/defines/core/containers.h"


// API functions --------------------------------------------------- //

KYRA_ENGINE_API ContainerResult     container_array_construct(const ByteSize data_size, Array *out_array);
KYRA_ENGINE_API ContainerResult     container_array_destruct(Array *array);

KYRA_ENGINE_API ContainerResult     container_array_push(Array *array, const VoidPtr data);
KYRA_ENGINE_API ContainerResult     container_array_pop(Array *array);

KYRA_ENGINE_API ContainerResult     container_array_insert(Array *array, const ByteSize index, const VoidPtr data);
KYRA_ENGINE_API ContainerResult     container_array_remove(Array *array, const VoidPtr data, const Bool remove_all);
KYRA_ENGINE_API ContainerResult     container_array_remove_at(Array *array, const ByteSize index);

KYRA_ENGINE_API ContainerResult     container_array_clear(Array *array);
KYRA_ENGINE_API ContainerResult     container_array_sort(Array *array, Int32 (*compare)(const VoidPtr left, const VoidPtr right));

KYRA_ENGINE_API ByteSize            container_array_data_size(const Array array);
KYRA_ENGINE_API ByteSize            container_array_size(const Array array);
KYRA_ENGINE_API ByteSize            container_array_capacity(const Array array);
KYRA_ENGINE_API VoidPtr             container_array_get(const Array array);
KYRA_ENGINE_API VoidPtr             container_array_get_at(const Array array, const ByteSize index);
KYRA_ENGINE_API Bool                container_array_is_empty(const Array array);

KYRA_ENGINE_API ConstStr            container_array_result_to_string(const ContainerResult result);

