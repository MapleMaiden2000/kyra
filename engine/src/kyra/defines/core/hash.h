#pragma once

#include "kyra/defines/core/types.h"


// Types ----------------------------------------------------------- //

typedef UInt64 HashedID;


// Hash modes ------------------------------------------------------ //

typedef enum Hash_Mode {
    HASH_MODE_XXH3     = 0
} HashMode;


// Macros ---------------------------------------------------------- //

#define INVALID_HASH ((HashedID)0xffffffffffffffffu)




