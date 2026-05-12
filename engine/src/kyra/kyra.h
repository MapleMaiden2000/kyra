#pragma once

/**
 * Kyra Engine
 * 
 * This file provides a single entry point for all public engine systems, data structures, and definitions.
 */


// Shared configurations and Definitions
#include "kyra/defines/shared.h"
#include "kyra/defines/core/types.h"
#include "kyra/defines/core/filesystem.h"
#include "kyra/defines/core/memory.h"
#include "kyra/defines/core/console.h"
#include "kyra/defines/core/containers.h"
#include "kyra/defines/core/clock.h"
#include "kyra/defines/core/hash.h"

// Core engine systems
#include "kyra/core/platform/filesystem/filesystem.h"
#include "kyra/core/memory/manager/memory_manager.h"
#include "kyra/core/memory/zone/memory_zone.h"
#include "kyra/core/misc/console/console.h"
#include "kyra/core/containers/string/string.h"
#include "kyra/core/hal/clock/wall/wall.h"
#include "kyra/core/hal/clock/hi_res/hi_res.h"
#include "kyra/core/hash/hash.h"
#include "kyra/core/engine/engine.h"

// Application and Entry point
#include "kyra/core/application/application.h"
#include "kyra/core/entry/entry.h"
