#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/memory.h"


// API functions ------------------------------------------------------------ //

KYRA_ENGINE_API MemoryZoneResult    memory_zone_get(ConstStr name, MemoryZone **out_zone);
KYRA_ENGINE_API MemoryZoneResult    memory_zone_clear(ConstStr name);

KYRA_ENGINE_API MemoryZoneResult    memory_zone_allocate(ConstStr name, const ByteSize size, VoidPtr *out_addr, ByteSize *out_alloc_size);
KYRA_ENGINE_API MemoryZoneResult    memory_zone_deallocate(ConstStr name, const VoidPtr addr, const ByteSize size);

KYRA_ENGINE_API ConstStr            memory_zone_result_to_string(const MemoryZoneResult result);



