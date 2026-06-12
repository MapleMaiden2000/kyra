#pragma once

#include "kyra/core/logger/logger.h"


// Debug break
#if !defined(KYRA_DEBUG_BREAK)
    #if KYRA_PLATFORM_WINDOWS
        #include <intrin.h>
        #define KYRA_DEBUG_BREAK() __debugbreak()
    #elif KYRA_PLATFORM_LINUX || KYRA_PLATFORM_MACOS 
        #define KYRA_DEBUG_BREAK() __builtin_trap()
    #else
        #define KYRA_DEBUG_BREAK() ((void)0)  // Fallback: no-op
    #endif
#endif

// Assertion
#if defined(KYRA_ENABLE_ASSERTIONS)
    #define KYRA_ASSERT(condition)                                              \
        do {                                                                    \
            if (!(condition)) {                                                 \
                KYRA_LOG_ENGINE_FATAL("Assertion Failed: '%s'", #condition);    \
                KYRA_DEBUG_BREAK();                                             \
            }                                                                   \
        } while (0)

    #define KYRA_ASSERT_MESSAGE(condition, message)                                                 \
        do {                                                                                        \
            if (!(condition)) {                                                                     \
                KYRA_LOG_ENGINE_FATAL("Assertion Failed: '%s', Message: %s", #condition, message);  \
                KYRA_DEBUG_BREAK();                                                                 \
            }                                                                                       \
        } while (0)
#else
    #define KYRA_ASSERT(condition) ((void)0)                   // No-op
    #define KYRA_ASSERT_MESSAGE(condition, message) ((void)0)  // No-op
#endif

// Verify runtime conditions
// Callable on both debug and release modes
#define KYRA_VERIFY(condition)                                          \
    do {                                                                \
        if (!(condition)) {                                             \
            KYRA_LOG_ENGINE_FATAL("Critical Error: '%s'", #condition);  \
        }                                                               \
    } while (0)

