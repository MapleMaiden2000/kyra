#include <kyra/kyra.h>
#include <kyra_editor/editor/editor.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


typedef struct Sandbox_State {
    Bool use_editor;

} SandboxState;


static void on_startup(Application *app) {
    KYRA_LOG_PRINT("SANDBOX", LOGGER_VERBOSITY_INFO, "Application '%s' initialised.", container_string_cstr(app->name));

    // Allocate sandbox state
    SandboxState *state = malloc(sizeof(SandboxState));
    
    if (!state) {
        KYRA_LOG_PRINT("SANDBOX", LOGGER_VERBOSITY_ERROR, "Failed to allocate memory for state.");

        return;
    }

    // Initialise state
    {
        state->use_editor = true;

        app->user_data = state;
    }

    // Check for -NoEditor flag
    for (Int32 i = 0; i < app->argc; ++i) {
        if (strcmp(app->argv[i], "-NoEditor") == 0) {
            state->use_editor = false;
            break;
        }
    }

    if (state->use_editor) {
        KYRA_LOG_PRINT("SANDBOX", LOGGER_VERBOSITY_INFO, "Editor requested. Opening editor...");
        
        ConstStr config_filepath = "sandbox/configs/editor_config.kyra";
        
        if (editor_startup(config_filepath) != EDITOR_SUCCESS)
            KYRA_LOG_PRINT("SANDBOX", LOGGER_VERBOSITY_ERROR, "Failed to open editor!");
    }
    else {
        KYRA_LOG_PRINT("SANDBOX", LOGGER_VERBOSITY_INFO, "Running in Headless Mode (-NoEditor).");
    }

}

static void on_update(Application *app, float delta_time) {
    SandboxState *state = (SandboxState *)app->user_data;
    if (!state) return;
    
    if (state->use_editor) {
        // Update editor
        editor_update();
        
        // Request shutdown if editor requested to close
        if (editor_should_close()) application_request_shutdown(app);
    } else {
        // Headless Mode sustained loop logic
        static Bool logged = false;
        
        if (!logged) {
            KYRA_LOG_PRINT("SANDBOX", LOGGER_VERBOSITY_INFO, "Headless loop active. Delta: %.4f", delta_time);
            logged = true;
        }
    }
}

static void on_shutdown(Application *app) {
    KYRA_LOG_PRINT("SANDBOX", LOGGER_VERBOSITY_INFO, "Application '%s' shutting down...", container_string_cstr(app->name));
    
    SandboxState *state = (SandboxState *)app->user_data;
    if (state) {
        // Editor shutdown
        if (state->use_editor) {
            EditorResult shutdown_result = editor_shutdown(); 
            if (shutdown_result != EDITOR_SUCCESS)
                KYRA_LOG_PRINT("SANDBOX", LOGGER_VERBOSITY_ERROR, "Failed to close editor! Error: %s", editor_result_to_string(shutdown_result));
        }
        
        // Deallocate state
        free(state);

        // Set to NULL
        state = NULL;
        app->user_data = NULL;
    }
    
    KYRA_LOG_PRINT("SANDBOX", LOGGER_VERBOSITY_INFO, "Application '%s' shut down.", container_string_cstr(app->name));

    // Destruct application logger
    LoggerResult logger_result = logger_unregister("SANDBOX");
    if (logger_result != LOGGER_SUCCESS)
        KYRA_PRINT_ERROR("Sandbox: Failed to unregister logger (Error: %s)", logger_result_to_string(logger_result));
}


Application *application_create(void) {
    static Application app;

    ConstStr config_filepath = "sandbox/configs/app_config.kyra";

    // Configure application
    if (application_configure(config_filepath, &app) != APPLICATION_SUCCESS)
        return NULL;

    app.on_startup = on_startup;
    app.on_update = on_update;
    app.on_shutdown = on_shutdown;

    // Construct logger for application
    LoggerResult logger_result = logger_register("SANDBOX", "sandbox.log", LOGGER_FLAG_ALL);
    if (logger_result != LOGGER_SUCCESS)
        KYRA_PRINT_ERROR("Sandbox: Failed to register logger (Error: %s)", logger_result_to_string(logger_result));

    return &app;
}

