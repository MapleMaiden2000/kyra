#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/memory.h"


// API functions ------------------------------------------------------------ //

KYRA_ENGINE_API MemoryManagerResult     memory_manager_startup(const MemoryConfig *config);
KYRA_ENGINE_API MemoryManagerResult     memory_manager_shutdown(void);

KYRA_ENGINE_API MemoryManagerResult     memory_manager_get(MemoryManager *out_manager);
KYRA_ENGINE_API MemoryManagerResult     memory_manager_report(void);

KYRA_ENGINE_API ConstStr                memory_manager_result_to_string(const MemoryManagerResult result);




