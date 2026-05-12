#include "kyra/core/hash/xxh3/xxh3.h"

#include <string.h>


// This is the local Kyra's adaptation of Yann Collet's xxh3 official implementation. 
// Source material: https://github.com/Cyan4973/xxHash/blob/dev/xxhash.h

// Prime constants ------------------------------------------------------ //

#define XXH3_PRIME32_1  2654435761ul
#define XXH3_PRIME32_2  2246822519ul
#define XXH3_PRIME32_3  3266489917ul   /* Knuth multiplicative hash */

#define XXH3_PRIME64_1  11400714785074694791ull
#define XXH3_PRIME64_2  14029467366897019727ull
#define XXH3_PRIME64_3  1609587929392839161ull
#define XXH3_PRIME64_4  9650029242287828579ull
#define XXH3_PRIME64_5  2870177450012600261ull


// Default secret (192 bytes) ------------------------------------------- //

// The canonical kSecret from official xxHash implementation.
static const UInt8 XXH3_DEFAULT_SECRET[192] = {
    0xb8, 0xfe, 0x6c, 0x39, 0x23, 0xa4, 0x4b, 0xbe, 0x7c, 0x01, 0x81, 0x2c, 0xf7, 0x21, 0xad, 0x1c,
    0xde, 0xd4, 0x6d, 0xe9, 0x83, 0x90, 0x97, 0xdb, 0x72, 0x40, 0xa4, 0xa4, 0xb7, 0xb3, 0x67, 0x1f,
    0xcb, 0x79, 0xfb, 0x31, 0x36, 0x6b, 0x60, 0x51, 0x51, 0x34, 0x0f, 0x2e, 0x1a, 0x07, 0x82, 0x04,
    0x36, 0x3f, 0x58, 0x32, 0x45, 0x30, 0x2c, 0x92, 0x2b, 0x47, 0xc1, 0x4c, 0x01, 0x4b, 0x6e, 0x15,
    0x3f, 0x12, 0x6c, 0x5f, 0x49, 0x83, 0x3e, 0x0c, 0x36, 0x21, 0x62, 0x44, 0x93, 0x2f, 0x76, 0x5e,
    0x3f, 0x6c, 0x88, 0x13, 0x63, 0xeb, 0x17, 0x0d, 0xdd, 0x51, 0xb7, 0xf0, 0xda, 0x49, 0xd3, 0x16,
    0x55, 0x26, 0x29, 0xd4, 0x68, 0x9e, 0x2b, 0x16, 0xbe, 0x58, 0x7d, 0x47, 0xa1, 0xfc, 0x8f, 0xf8,
    0xb8, 0xd1, 0x7a, 0xd0, 0x31, 0xce, 0x45, 0xcb, 0x3a, 0x8f, 0x95, 0x16, 0x04, 0x28, 0xaf, 0xd7,
    0xfb, 0xca, 0xbb, 0x4b, 0x40, 0x7e, 0x1a, 0x81, 0x56, 0x6d, 0x71, 0x8d, 0x14, 0x7e, 0x5a, 0x0e,
    0x53, 0x22, 0x8d, 0x06, 0x65, 0x8d, 0xfc, 0x10, 0x5e, 0xfb, 0x20, 0x2c, 0x7a, 0xcb, 0xd8, 0x48,
    0x69, 0x36, 0x92, 0xfc, 0xc1, 0x5b, 0x88, 0x41, 0x88, 0x8a, 0xe1, 0x8a, 0x5b, 0x3d, 0xb9, 0x0f,
    0x38, 0x57, 0x33, 0xcf, 0x09, 0x25, 0x59, 0x4b, 0x09, 0x4d, 0xf5, 0x3f, 0x7f, 0x43, 0x17, 0xc5
};


// Helper functions ----------------------------------------------------- //

// Reads 32-bit value from pointer
static KYRA_INLINE UInt32 _hash_xxh3_read32(const UInt8 *ptr) {
    UInt32 value = 0;
    memcpy(&value, ptr, sizeof(UInt32));
    
    return value;
}

// Reads 64-bit value from pointer
static KYRA_INLINE UInt64 _hash_xxh3_read64(const UInt8 *ptr) {
    UInt64 value = 0;
    memcpy(&value, ptr, sizeof(UInt64));
    
    return value;
}

// Rotates left on 64-bit value
static KYRA_INLINE UInt64 _hash_xxh3_rotate(UInt64 value, Int32 r) {
    return (value << r) | (value >> (64 - r));
}

// Portable 64x64->128 multiplication, fold high into low
static KYRA_INLINE UInt64 _hash_xxh3_mul128_fold64(UInt64 left, UInt64 right) {
    // Low-low multiplication
    UInt64 lo_lo = (left & 0xffffffffull) * (right & 0xffffffffull);
    
    // High-low multiplication
    UInt64 hi_lo = (left >> 32) * (right & 0xffffffffull);
    
    // Low-high multiplication
    UInt64 lo_hi = (left & 0xffffffffull) * (right >> 32);
    
    // High-high multiplication
    UInt64 hi_hi = (left >> 32) * (right >> 32);
    
    // Cross multiplication
    UInt64 cross = (lo_lo >> 32) + (hi_lo & 0xffffffffull) + lo_hi;
    
    // Upper multiplication
    UInt64 upper = (hi_lo >> 32) + (cross >> 32) + hi_hi;
    
    // Lower multiplication
    UInt64 lower = (cross << 32) | (lo_lo & 0xffffffffull);
    
    // Fold high into low
    return lower ^ upper;
}

// Performs general-purpose avalanche
static KYRA_INLINE UInt64 _hash_xxh3_avalanche(UInt64 hash) {
    // XOR with right-shifted value
    hash ^= hash >> 37;
    
    // Multiply by prime
    hash *= 0x165667919E3779F9ULL;
    
    // XOR with right-shifted value
    hash ^= hash >> 32;
    
    return hash;
}

// Performs stronger avalanche for very short inputs
static KYRA_INLINE UInt64 _hash_xxh3_avalance_strong(UInt64 hash, const UInt64 length) {
    // Rotate left on 64-bit value
    hash ^= _hash_xxh3_rotate(hash, 49) ^ _hash_xxh3_rotate(hash, 24);
    
    // Multiply by prime
    hash *= 0x9FB21C651E98DF25ULL;
    
    // XOR with length
    hash ^= (hash >> 35) + length;
    
    // Multiply by prime
    hash *= 0x9FB21C651E98DF25ULL;
    
    // XOR with right-shifted value
    hash ^= hash >> 28;
    
    return hash;
}

// Mix 16 bytes of input with 16 bytes of secret, with seed
static KYRA_INLINE UInt64 _hash_xxh3_mix16B(const UInt8 *input, const UInt8 *secret, const UInt64 seed) {
    // Read first 8 bytes of input and 8 bytes of secret
    UInt64 lo = _hash_xxh3_read64(input) ^ (_hash_xxh3_read64(secret) + seed);
    
    // Read last 8 bytes of input and 8 bytes of secret
    UInt64 hi = _hash_xxh3_read64(input + 8) ^ (_hash_xxh3_read64(secret + 8) - seed);
    
    // Multiply and fold high into low
    return _hash_xxh3_mul128_fold64(lo, hi);
}


// --- Long-path --- //

#define XXH3_ACC_NUM                8                           // Number of accumulators
#define XXH3_STRIPE_LEN             64                          // Length of one stripe (64 bytes)
#define XXH3_SECRET_CONSUME_RATE    8                           // Rate at which secret is consumed
#define XXH3_BLOCK_LEN              (XXH3_STRIPE_LEN * 16)      // Length of one block (1024 bytes)

// Accumulates one 64-byte stripe into 8 accumulators
static void _hash_xxh3_accumulate_512(UInt64 *acc, const UInt8 *input, const UInt8 *secret) {
    ByteSize index = 0;
    
    for (; index < XXH3_ACC_NUM; ++index) {
        // Read 64-bit value from input and secret
        UInt64 data_val = _hash_xxh3_read64(input  + (index * 8));
        UInt64 data_key = data_val ^ _hash_xxh3_read64(secret + (index * 8));
        
        // Cross-pollinate adjacent lanes, then accumulate product
        acc[index ^ 1] += data_val;
        acc[index] += (data_key & 0xffffffffull) * (data_key >> 32);
    }
}

// Scrambles accumulators between blocks to destroy local patterns
static void _hash_xxh3_scramble_acc(UInt64 *acc, const UInt8 *secret) {
    ByteSize index = 0;
    
    for (; index < XXH3_ACC_NUM; ++index) {
        // XOR with right-shifted value
        acc[index] ^= acc[index] >> 47;
        
        // XOR with secret
        acc[index] ^= _hash_xxh3_read64(secret + (index * 8));
        
        // Multiply by prime
        acc[index] *= (UInt64)XXH3_PRIME32_1;
    }
}

// Processes all complete 1024-byte blocks
static void _hash_xxh3_hash_long_internal(UInt64 *acc, const UInt8 *input, ByteSize length, const UInt8 *secret) {
    ByteSize num_blocks = length / XXH3_BLOCK_LEN;
    ByteSize block;
    
    for (block = 0; block < num_blocks; block++) {
        ByteSize stripe = 0;
        
        // Accumulate 16 stripes of 64 bytes each
        for (; stripe < 16; ++stripe) {
            _hash_xxh3_accumulate_512(
                acc, 
                input + (block * XXH3_BLOCK_LEN) + (stripe * XXH3_STRIPE_LEN), 
                secret + (stripe * XXH3_SECRET_CONSUME_RATE)
            );
        }
        
        // Scramble accumulators between blocks
        _hash_xxh3_scramble_acc(acc, secret + 192 - XXH3_STRIPE_LEN);
    }
    
    // Remaining stripes in the last partial block
    {
        ByteSize remaining  = length - (num_blocks * XXH3_BLOCK_LEN);
        ByteSize num_stripes = remaining / XXH3_STRIPE_LEN;
        ByteSize stripe;
        
        // Process remaining stripes in the last partial block
        for (stripe = 0; stripe < num_stripes; stripe++) {
            _hash_xxh3_accumulate_512(
                acc,
                input + (num_blocks * XXH3_BLOCK_LEN) + (stripe * XXH3_STRIPE_LEN),
                secret + (stripe * XXH3_SECRET_CONSUME_RATE)
            );
        }
        
        // Always process the very last 64 bytes of input
        _hash_xxh3_accumulate_512(
            acc,
            input + length - XXH3_STRIPE_LEN,
            secret + 192 - XXH3_STRIPE_LEN - 7
        );
    }
}

// Merges all 8 accumulators into a single 64-bit value
static UInt64 _hash_xxh3_merge_accs(const UInt64 *acc, const UInt8 *secret, UInt64 start) {
    UInt64 result = start;
    ByteSize index = 0;
    
    // Merge 4 pairs of accumulators
    for (; index < 4; ++index) {
        // XOR with secret and multiply
        result += _hash_xxh3_mul128_fold64(
            acc[index * 2] ^ _hash_xxh3_read64(secret + (index * 16)),
            acc[index * 2 + 1] ^ _hash_xxh3_read64(secret + (index * 16) + 8)
        );
    }
    
    // Avalanche the result
    return _hash_xxh3_avalanche(result);
}


// Path 1: Short inputs (0–16 bytes) ------------------------------------ //

static UInt64 _hash_xxh3_len_0to16(const UInt8 *input, ByteSize length, const UInt8 *secret, const UInt64 seed) {
    // 9–16 bytes
    if (length > 8) {
        // Read first and last 8 bytes
        UInt64 lo = _hash_xxh3_read64(input) ^ (_hash_xxh3_read64(secret + 24) + seed);
        UInt64 hi = _hash_xxh3_read64(input + length - 8) ^ (_hash_xxh3_read64(secret + 32) - seed);
        
        // Combine and avalanche
        UInt64 acc = (UInt64)length + _hash_xxh3_rotate(lo, 1) + _hash_xxh3_rotate(hi, 7) + _hash_xxh3_mul128_fold64(lo, hi);
        return _hash_xxh3_avalanche(acc);
    }
    
    // 4–8 bytes
    if (length >= 4) {
        // Read first and last 32 bits
        UInt64 inp1 = (UInt64)_hash_xxh3_read32(input);
        UInt64 inp2 = (UInt64)_hash_xxh3_read32(input + length - 4);
        
        // XOR with secret
        UInt64 bitflip = (_hash_xxh3_read64(secret + 8) ^ _hash_xxh3_read64(secret + 16)) - seed;
        UInt64 keyed = (inp1 ^ (inp2 << 32)) ^ bitflip;
        
        // Avalanche
        return _hash_xxh3_avalance_strong(keyed, (UInt64)length);
    }
    
    // 1–3 bytes
    if (length > 0) {
        // Read first, middle, and last bytes
        UInt8 c1  = input[0];
        UInt8 c2  = input[length >> 1];
        UInt8 c3  = input[length - 1];
        
        // Combine and avalanche
        UInt32 combined = ((UInt32)c1 << 16) | ((UInt32)c2 << 24) | ((UInt32)c3) | ((UInt32)length << 8);
        UInt64 bitflip = (_hash_xxh3_read32(secret) ^ _hash_xxh3_read32(secret + 4)) + seed;
        return _hash_xxh3_avalanche((UInt64)combined ^ bitflip);
    }
    
    // 0 bytes (empty string)
    {
        // XOR with secret
        UInt64 bitflip = _hash_xxh3_read64(secret + 56) ^ _hash_xxh3_read64(secret + 64);
        
        // Avalanche
        return _hash_xxh3_avalanche(seed ^ bitflip);
    }
}


// Path 2: Medium inputs (17–240 bytes) --------------------------------- //

static UInt64 _hash_xxh3_len_17to240(const UInt8 *input, ByteSize length, const UInt8 *secret, const UInt64 seed) {
    // Initialise accumulator with length times prime
    UInt64 acc = (UInt64)length * XXH3_PRIME64_1;
    
    // Number of 16-byte rounds
    ByteSize num_rounds = length / 16;
    ByteSize index = 0;
    
    // Process each 16-byte chunk
    for (; index < num_rounds; ++index) {
        acc += _hash_xxh3_mix16B(input + (index * 16), secret + (index * 16), seed);
    }
    
    // Always mix in the last 16 bytes (may overlap with above)
    acc += _hash_xxh3_mix16B(input + length - 16, secret + 112, seed);
    
    // Avalanche
    return _hash_xxh3_avalanche(acc);
}


// Path 3: Long inputs (241+ bytes) ------------------------------------- //

static UInt64 _hash_xxh3_hash_long(const UInt8 *input, ByteSize length, const UInt8 *secret) {
    // Initialise 8 accumulators with specific primes
    UInt64 acc[XXH3_ACC_NUM] = {
        (UInt64)XXH3_PRIME32_3,
        (UInt64)XXH3_PRIME64_1,
        (UInt64)XXH3_PRIME64_2,
        (UInt64)XXH3_PRIME64_3,
        (UInt64)XXH3_PRIME64_4,
        (UInt64)XXH3_PRIME32_2,
        (UInt64)XXH3_PRIME64_5,
        (UInt64)XXH3_PRIME32_1
    };
    
    // Hash long inputs
    _hash_xxh3_hash_long_internal(acc, input, length, secret);
    
    // Merge accumulators
    return _hash_xxh3_merge_accs(acc, secret + 11, (UInt64)length * XXH3_PRIME64_1);
}

// Seeded (derives a per-seed custom secret from default secret)
static void _hash_xxh3_init_custom_secret(UInt8 *custom_secret, const UInt64 seed) {
    ByteSize index = 0;
    
    for (; index < 192 / 16; ++index) {
        // Read 64 bits from default secret and XOR with the seed
        UInt64 lo = _hash_xxh3_read64(XXH3_DEFAULT_SECRET + (index * 16)) + seed;
        UInt64 hi = _hash_xxh3_read64(XXH3_DEFAULT_SECRET + (index * 16) + 8) - seed;
        
        // Write 64-bit values to custom secret
        memcpy(custom_secret + (index * 16), &lo, sizeof(UInt64));
        memcpy(custom_secret + (index * 16) + 8, &hi, sizeof(UInt64));
    }
}


// API functions -------------------------------------------------------- //

KYRA_ENGINE_API HashedID hash_xxh3_compute(const VoidPtr buffer, const ByteSize size, const UInt64 seed) {
    const UInt8 *input  = (const UInt8*)buffer;
    const UInt8 *secret = XXH3_DEFAULT_SECRET;
    
    // For seeded calls, derive a custom 192-byte secret on the stack
    UInt8 custom_secret[192];
    if (seed != 0) {
        // Derive a per-seed custom secret from default secret
        _hash_xxh3_init_custom_secret(custom_secret, seed);
        secret = custom_secret;
    }
    
    // Path 1: Short inputs (0–16 bytes)
    if (size <= 16) return _hash_xxh3_len_0to16(input, size, secret, seed);
    
    // Path 2: Medium inputs (17–240 bytes)
    if (size <= 240) return _hash_xxh3_len_17to240(input, size, secret, seed);
    
    // Path 3: Long inputs (241+ bytes)
    return _hash_xxh3_hash_long(input, size, secret);
}
