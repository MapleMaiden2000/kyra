#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/command.h"
#include "kyra/defines/core/containers.h"


// API functions ----------------------------------------------- //

KYRA_ENGINE_API CommandModuleResult     command_module_startup(ConstStr config_filepath);
KYRA_ENGINE_API CommandModuleResult     command_module_shutdown(void);

KYRA_ENGINE_API CommandModuleResult     command_module_setup_command_line(ConstStr header);
KYRA_ENGINE_API CommandModuleResult     command_module_activate_command_line(void);
KYRA_ENGINE_API CommandModuleResult     command_module_inactivate_command_line(void);
KYRA_ENGINE_API CommandModuleResult     command_module_update_command_line(void);

KYRA_ENGINE_API CommandModuleResult     command_module_register_schema(ConstStr schema_filepath);

KYRA_ENGINE_API CommandModuleResult     command_module_register_action(ConstStr root, ConstStr action, const Listener listener, const DelegateFunction callback);
KYRA_ENGINE_API CommandModuleResult     command_module_update_action(ConstStr root, ConstStr action, const Listener listener, const DelegateFunction callback);

KYRA_ENGINE_API CommandModuleResult     command_module_dispatch(const String command, const Sender sender);

KYRA_ENGINE_API CommandModuleResult     command_module_poll_input(String *out_string);

KYRA_ENGINE_API Bool                    command_module_command_line_active(void);

KYRA_ENGINE_API ConstStr                command_module_result_to_string(const CommandModuleResult result);


