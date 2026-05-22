#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/logger.h"


// API functions --------------------------------------------------- //

KYRA_ENGINE_API LoggerResult    logger_startup(ConstStr config_filepath);
KYRA_ENGINE_API LoggerResult    logger_shutdown(void);

KYRA_ENGINE_API LoggerResult    logger_register(ConstStr id, ConstStr filename, const LoggerFlags flags);
KYRA_ENGINE_API LoggerResult    logger_unregister(ConstStr id);

KYRA_ENGINE_API LoggerResult    logger_update(ConstStr id, ConstStr new_filename, const LoggerFlags new_flags);

KYRA_ENGINE_API LoggerResult    logger_print(
    ConstStr            id,

    LoggerVerbosity     verbosity,
    LoggerFlags         flags,

    ConstStr            at_file,
    UInt32              at_line,
    ConstStr            at_function,

    ConstStr            format,
    ...
);

KYRA_ENGINE_API ConstStr        logger_result_to_string(const LoggerResult result);


// Logging macros -------------------------------------------------- //

#define KYRA_LOG_PRINT(id, verbosity, ...) \
    logger_print(id, verbosity, LOGGER_FLAG_ALL, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define KYRA_LOG_ENGINE_TRACE(...)  KYRA_LOG_PRINT("KYRA_ENGINE", LOGGER_VERBOSITY_TRACE,   __VA_ARGS__)
#define KYRA_LOG_ENGINE_DEBUG(...)  KYRA_LOG_PRINT("KYRA_ENGINE", LOGGER_VERBOSITY_DEBUG,   __VA_ARGS__)
#define KYRA_LOG_ENGINE_INFO(...)   KYRA_LOG_PRINT("KYRA_ENGINE", LOGGER_VERBOSITY_INFO,    __VA_ARGS__)
#define KYRA_LOG_ENGINE_WARN(...)   KYRA_LOG_PRINT("KYRA_ENGINE", LOGGER_VERBOSITY_WARNING, __VA_ARGS__)
#define KYRA_LOG_ENGINE_ERROR(...)  KYRA_LOG_PRINT("KYRA_ENGINE", LOGGER_VERBOSITY_ERROR,   __VA_ARGS__)
#define KYRA_LOG_ENGINE_FATAL(...)  KYRA_LOG_PRINT("KYRA_ENGINE", LOGGER_VERBOSITY_FATAL,   __VA_ARGS__)


