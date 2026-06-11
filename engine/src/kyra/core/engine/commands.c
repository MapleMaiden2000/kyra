#include "kyra/core/engine/commands.h"

#include <stdio.h>
#include <string.h>

#include "kyra/core/memory/manager/memory_manager.h"
#include "kyra/core/containers/array/array.h"
#include "kyra/core/modules/command/command_module.h"
#include "kyra/core/logger/logger.h"
#include "kyra/core/engine/engine.h"
#include "kyra/core/application/application.h"


// Handler functions ----------------------------------------------- //

static void _engine_commands_log(const Sender sender, const Listener listener, const VoidPtr data) {
    CommandContext *context = (CommandContext *)data;
    if (!context->args) return;
    if (context->num_args < 2) return;

    // Get log verbosity level and message
    ConstStr level = context->args[0];
    ConstStr message = context->args[1];

    if      (!strcmp(level, "fatal"))   KYRA_LOG_ENGINE_FATAL("%s", message);
    else if (!strcmp(level, "error"))   KYRA_LOG_ENGINE_ERROR("%s", message);
    else if (!strcmp(level, "warn"))    KYRA_LOG_ENGINE_WARN("%s", message);
    else if (!strcmp(level, "info"))    KYRA_LOG_ENGINE_INFO("%s", message);
    else if (!strcmp(level, "trace"))   KYRA_LOG_ENGINE_TRACE("%s", message);
    else if (!strcmp(level, "debug"))   KYRA_LOG_ENGINE_DEBUG("%s", message);
}

static void _engine_commands_exit(const Sender sender, const Listener listener, const VoidPtr data) {
    KYRA_LOG_ENGINE_INFO("Processing exit request...");

    // Request engine shutdown
    engine_request_shutdown();
}


// API functions --------------------------------------------------- //

KYRA_ENGINE_API void engine_commands_init(void) {
    command_module_register_action("engine", "log", NULL, _engine_commands_log);
    command_module_register_action("engine", "exit", NULL, _engine_commands_exit);
}

