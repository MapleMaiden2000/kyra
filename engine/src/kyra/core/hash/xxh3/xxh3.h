#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/memory.h"
#include "kyra/defines/core/hash.h"


// API functions -------------------------------------------------------- //

KYRA_ENGINE_API HashedID    hash_xxh3_compute(const VoidPtr buffer, const ByteSize size, const UInt64 seed);

