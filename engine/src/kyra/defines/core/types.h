#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>


// Signed integer types ------------------------------------------------- //

typedef int8_t          Int8;
typedef int16_t         Int16;
typedef int32_t         Int32;
typedef int64_t         Int64;

#if defined(__SIZEOF_INT128__)
typedef __int128_t      Int128;
#endif


// Unsigned integer types ----------------------------------------------- //

typedef uint8_t         UInt8;
typedef uint16_t        UInt16;
typedef uint32_t        UInt32;
typedef uint64_t        UInt64;

#if defined(__SIZEOF_INT128__)
typedef __uint128_t     UInt128;
#endif


// Boolean type --------------------------------------------------------- //

#if defined(bool)
typedef bool            Bool;
#else
typedef enum Boolean { 
    false = 0, 
    true  = 1 
}                       Bool;
#endif


// Floating point types ------------------------------------------------- //

typedef float           Flt32;
typedef double          Flt64;


// Character-based types ------------------------------------------------ //

typedef char            Char;
typedef char           *Str;
typedef const char     *ConstStr;


// Memory types --------------------------------------------------------- //

typedef size_t          ByteSize;
typedef uint8_t        *BytePtr;
typedef void           *VoidPtr;
typedef size_t         *SizePtr;
typedef uintptr_t       UIntPtr;


// Variadic argument list ----------------------------------------------- //

#if defined(_MSC_VER)
typedef va_list VaList;
#else
typedef __builtin_va_list VaList;
#endif

