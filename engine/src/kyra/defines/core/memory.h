#pragma once

#include "kyra/defines/core/types.h"


// Return codes --------------------------------------------------------- //

typedef enum Memory_Manager_Result {
    MEMORY_MANAGER_SUCCESS                                  = 0,

    MEMORY_MANAGER_ERROR_MEMORY_CONFIG_NULL                 = -1,
    MEMORY_MANAGER_ERROR_STATE_ALREADY_INITIALISED          = -2,
    MEMORY_MANAGER_ERROR_STATE_NOT_INITIALISED              = -3,
    MEMORY_MANAGER_ERROR_FAILED_TO_ALLOCATE_MEMORY_BLOCK    = -4,

    MEMORY_MANAGER_HELPER_ERROR_CAPACITY_ZERO               = -100
    
} MemoryManagerResult;

typedef enum Memory_Zone_Result {
    MEMORY_ZONE_SUCCESS                                     = 0,

    MEMORY_ZONE_ERROR_ZONE_NAME_NULL                        = -1,
    MEMORY_ZONE_ERROR_SIZE_ZERO                             = -2,
    MEMORY_ZONE_ERROR_INVALID_ADDRESS                       = -3,
    MEMORY_ZONE_ERROR_FAILED_TO_GET_MEMORY_MANAGER          = -4,
    MEMORY_ZONE_ERROR_FAILED_TO_LOCATE_ZONE                 = -5,
    MEMORY_ZONE_ERROR_FAILED_TO_GET_SIZE_CLASS_INDEX        = -6,
    MEMORY_ZONE_ERROR_FAILED_TO_RESIZE_SIZE_CLASS           = -7,
    MEMORY_ZONE_ERROR_INSUFFICIENT_MEMORY_TO_ALLOCATE       = -8,

    MEMORY_ZONE_HELPER_ERROR_ZONE_NULL                      = -100,
    MEMORY_ZONE_HELPER_ERROR_SIZE_ZERO                      = -101

    
} MemoryZoneResult;


// Configuration -------------------------------------------------------- //

typedef struct Memory_Zone_Config {
    Str                     name;
    ByteSize                capacity;
} MemoryZoneConfig;

typedef struct Memory_Config {
    MemoryZoneConfig       *zones;    
    ByteSize                zone_count;
} MemoryConfig;


// Types ---------------------------------------------------------------- //

typedef struct Memory_Zone_Size_Class {
    VoidPtr                *blocks;
    ByteSize                num_blocks;

    ByteSize                size;
    ByteSize                capacity;
} MemoryZoneSizeClass;

typedef struct Memory_Zone {
    ConstStr                name;
    UIntPtr                 addr_start;

    ByteSize                used_memory;
    ByteSize                capacity;

    MemoryZoneSizeClass    *size_classes;
    ByteSize                num_classes;
} MemoryZone;


typedef struct Memory_Manager {
    VoidPtr                 pool;

    ByteSize                used_memory;
    ByteSize                capacity;

    MemoryZone             *zones;
    ByteSize                zone_count;
} MemoryManager;


// Check endianness ------------------------------------------------------- //

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
// Check if the system is little-endian (Intel, AMD, ARM)
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define IS_LITTLE_ENDIAN 1
        #define IS_BIG_ENDIAN 0
    #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        #define IS_LITTLE_ENDIAN 0
        #define IS_BIG_ENDIAN 1
    #else
        #error "Unknown endianness detected!"
    #endif
    
#elif defined(_WIN32) || defined(__LITTLE_ENDIAN__) || defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
// Check if the system is little-endian (Windows, x86, x64, ARM)
    #define IS_LITTLE_ENDIAN 1
    #define IS_BIG_ENDIAN 0
    
#elif defined(__BIG_ENDIAN__) || defined(__sparc__) || defined(__powerpc__) || defined(__s390__)
// Check if the system is big-endian (SPARC, PowerPC, s390)
    #define IS_LITTLE_ENDIAN 0
    #define IS_BIG_ENDIAN 1

#else
// Check if the system is little-endian (fallback)
    #include <stdint.h>
    KYRA_INLINE Int32 check_endianness(void) {
        UInt16 test = 0x0001;
        return (*((UInt8 *)&test) == 0x01) ? 1 : 0;
    }
    #define IS_LITTLE_ENDIAN check_endianness()
    #define IS_BIG_ENDIAN (!IS_LITTLE_ENDIAN)
#endif


// Memory alignment ----------------------------------------------------- //

#define KYRA_APPLY_MEMORY_ALIGNMENT(size, alignment) (((size) + (alignment - 1)) & ~(alignment - 1))


// Bit-flag operations -------------------------------------------------- //

#define KYRA_BITFLAG_FIELD(bit) 					(1 << bit)
#define KYRA_BITFLAG_TOGGLE(flags, bitmask) 		(flags ^= (bitmask))
#define KYRA_BITFLAG_SET(flags, bitmask) 			(flags |= (bitmask))
#define KYRA_BITFLAG_CLEAR(flags, bitmask) 			(flags &= ~(bitmask))
#define KYRA_BITFLAG_IF_SET(bitflag, bit) 		    ((bitflag) & (bit))
#define KYRA_BITFLAG_IF_NOT_SET(bitflag, bit) 	    (!KYRA_BITFLAG_IF_SET(bitflag, bit))
#define KYRA_BITFLAG_SET_IF_NOT(bitflag, bit) 	    (KYRA_BITFLAG_IF_NOT_SET(bitflag, bit) ? KYRA_BITFLAG_SET(bitflag, bit) : (bitflag))


// Byte-swap ------------------------------------------------------------ //

#if defined(__GNUC__) || defined(__clang__)
    #define KYRA_BSWAP32(x) __builtin_bswap32(x)
    #define KYRA_BSWAP64(x) __builtin_bswap64(x)
#elif defined(_MSC_VER)
    #define KYRA_BSWAP32(x) _byteswap_ulong(x)
    #define KYRA_BSWAP64(x) _byteswap_uint64(x)
#else
    #define KYRA_BSWAP32(x) ((((x) & 0xff000000u) >> 24) | (((x) & 0x00ff0000u) >> 8) | (((x) & 0x0000ff00u) << 8) | (((x) & 0x000000ffu) << 24))
    #define KYRA_BSWAP64(x)                                                                                                                                                 \
		((((x) & 0xff00000000000000ull) >> 56) | (((x) & 0x00ff000000000000ull) >> 40) | (((x) & 0x0000ff0000000000ull) >> 24) | (((x) & 0x000000ff00000000ull) >> 8)       \
        | (((x) & 0x00000000ff000000ull) << 8) | (((x) & 0x0000000000ff0000ull) << 24) | (((x) & 0x000000000000ff00ull) << 40) | (((x) & 0x00000000000000ffull) << 56))
#endif


// Size conversion ------------------------------------------------------ //

#define MEMORY_KB_TO_BYTES(size)    (ByteSize)(size * 1024)
#define MEMORY_MB_TO_BYTES(size)    (ByteSize)(MEMORY_KB_TO_BYTES(size) * 1024)
#define MEMORY_GB_TO_BYTES(size)    (ByteSize)(MEMORY_MB_TO_BYTES(size) * 1024)


