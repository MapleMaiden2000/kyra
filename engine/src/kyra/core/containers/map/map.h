#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/types.h"
#include "kyra/defines/core/containers.h"


// API functions --------------------------------------------------- //

KYRA_ENGINE_API ContainerResult     container_map_construct(const ByteSize data_size, Map *out_map);
KYRA_ENGINE_API ContainerResult     container_map_destruct(Map *map);

KYRA_ENGINE_API ContainerResult     container_map_insert(Map *map, ConstStr key, const VoidPtr value);
KYRA_ENGINE_API ContainerResult     container_map_remove(Map *map, ConstStr key);
KYRA_ENGINE_API ContainerResult     container_map_update(Map *map, ConstStr key, const VoidPtr new_value);
KYRA_ENGINE_API ContainerResult     container_map_clear(Map *map);

KYRA_ENGINE_API ContainerResult     container_map_search(const Map map, ConstStr key, VoidPtr out_value);
KYRA_ENGINE_API ContainerResult     container_map_at_index(const Map map, const ByteSize index, String *out_key, VoidPtr out_value);

KYRA_ENGINE_API Bool                container_map_contains(const Map map, ConstStr key);
KYRA_ENGINE_API Bool                container_map_is_empty(const Map map);
KYRA_ENGINE_API ByteSize            container_map_get_data_size(const Map map);
KYRA_ENGINE_API ByteSize            container_map_get_size(const Map map);
KYRA_ENGINE_API ByteSize            container_map_get_capacity(const Map map);

KYRA_ENGINE_API ConstStr            container_map_result_to_string(const ContainerResult result);