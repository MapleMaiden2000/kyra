#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/hash.h"


// API functions ------------------------------------------------- //

KYRA_ENGINE_API HashedID    hash_buffer(const VoidPtr buffer, const ByteSize size, const HashMode mode);
KYRA_ENGINE_API HashedID    hash_buffer_with_seed(const VoidPtr buffer, const ByteSize size, const HashMode mode, const UInt64 seed);

KYRA_ENGINE_API HashedID    hash_str(ConstStr str, const HashMode mode);
KYRA_ENGINE_API HashedID    hash_str_with_seed(ConstStr str, const HashMode mode, const UInt64 seed);





