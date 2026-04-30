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

// Core engine systems
#include "kyra/core/platform/filesystem/filesystem.h"
#include "kyra/core/memory/manager/memory_manager.h"
#include "kyra/core/engine/engine.h"

// Application and Entry point
#include "kyra/core/application/application.h"
#include "kyra/core/entry/entry.h"
