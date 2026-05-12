#include "kyra/core/hash/hash.h"

#include <string.h>

#include "kyra/core/hash/xxh3/xxh3.h"


// API functions ------------------------------------------------- //

KYRA_ENGINE_API HashedID hash_buffer(const VoidPtr buffer, const ByteSize size, const HashMode mode) {
    if (!buffer || size == 0) return INVALID_HASH;
    
    switch (mode) {
        case HASH_MODE_XXH3:
            return hash_xxh3_compute(buffer, size, 0);
        
        default:
            return INVALID_HASH;
    }
}

KYRA_ENGINE_API HashedID hash_buffer_with_seed(const VoidPtr buffer, const ByteSize size, const HashMode mode, const UInt64 seed) {
    if (!buffer || size == 0) return INVALID_HASH;
    
    switch (mode) {
        case HASH_MODE_XXH3:
            return hash_xxh3_compute(buffer, size, seed);
        
        default:
            return INVALID_HASH;
    }
}

KYRA_ENGINE_API HashedID hash_str(ConstStr str, const HashMode mode) {
    return hash_buffer((VoidPtr)str, strlen(str), mode);
}

KYRA_ENGINE_API HashedID hash_str_with_seed(ConstStr str, const HashMode mode, const UInt64 seed) {
    return hash_buffer_with_seed((VoidPtr)str, strlen(str), mode, seed);
}

