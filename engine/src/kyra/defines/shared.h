#pragma once

// Platform detection --------------------------------------------------- //

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    // Windows
    #define KYRA_PLATFORM_WINDOWS 1
    #ifndef _WIN64
        #error "64-bit is required on Windows!"
    #endif
#elif defined(__linux__) || defined(__gnu_linux__)
    // Linux
    #define KYRA_PLATFORM_LINUX 1
#elif defined(__APPLE__) || defined(__MACH__)
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC
        // macOS
        #define KYRA_PLATFORM_MACOS 1
    #else
        // Other Apple platforms
        #error "Only macOS is supported for Apple platforms!"
    #endif
#else
    // Unknown or unsupported platform
    #error "Unknown or unsupported platform!"
#endif


// DLL export/import ---------------------------------------------------- //

#ifdef KYRA_EXPORT
    // We are building the engine, so we export
    #ifdef KYRA_PLATFORM_WINDOWS
        #define KYRA_ENGINE_API __declspec(dllexport)
    #else
        #define KYRA_ENGINE_API __attribute__((visibility("default")))
    #endif
#else
    // We are using the engine (editor/sandbox), so we import
    #ifdef KYRA_PLATFORM_WINDOWS
        #define KYRA_ENGINE_API __declspec(dllimport)
    #else
        #define KYRA_ENGINE_API
    #endif
#endif


// Compiler specifics --------------------------------------------------- //

#if defined(__GNUC__) || defined(__clang__)
    // GCC and Clang
    #define KYRA_INLINE __attribute__((always_inline)) inline
    #define KYRA_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
    // MSVC
    #define KYRA_INLINE __forceinline
    #define KYRA_NOINLINE __declspec(noinline)
#else
    // Unknown or unsupported compiler
    #define KYRA_INLINE inline
    #define KYRA_NOINLINE
#endif




