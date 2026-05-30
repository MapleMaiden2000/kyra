#pragma once

#include "kyra/defines/shared.h"


// DLL export/import ----------------------------------------------- //

#ifdef KYRA_EDITOR_EXPORT
    // We are building editor library, so we export
    #ifdef KYRA_PLATFORM_WINDOWS
        #define KYRA_EDITOR_API __declspec(dllexport)
    #else
        #define KYRA_EDITOR_API __attribute__((visibility("default")))
    #endif
#else
    // We are using editor library (standalone app / sandbox), so we import
    #ifdef KYRA_PLATFORM_WINDOWS
        #define KYRA_EDITOR_API __declspec(dllimport)
    #else
        #define KYRA_EDITOR_API
    #endif
#endif


