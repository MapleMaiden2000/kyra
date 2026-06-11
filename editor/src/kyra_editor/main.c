#include <kyra/kyra.h>

#include "kyra_editor/editor/editor.h"


static void on_startup(Application *app) {
    KYRA_LOG_PRINT("EDITOR_APP", LOGGER_VERBOSITY_INFO, "Initialising editor...");
    
    ConstStr config_filepath = "sandbox/assets/config/editor_config.kyra";

    // Open the editor
    if (editor_startup(config_filepath) != EDITOR_SUCCESS) {
        // Failed to do so...
        
        KYRA_LOG_PRINT("EDITOR_APP", LOGGER_VERBOSITY_FATAL, "Failed to start Editor logic!");
        
        // Request application shutdown
        application_request_shutdown(app);
    }

    KYRA_LOG_PRINT("EDITOR_APP", LOGGER_VERBOSITY_INFO, "Editor initialised.");
}

static void on_update(Application *app, float delta_time) {
    // Update editor
    editor_update();

    if (editor_should_close()) application_request_shutdown(app);
}

static void on_shutdown(Application *app) {
    KYRA_LOG_PRINT("EDITOR_APP", LOGGER_VERBOSITY_INFO, "Shutting down editor...");
    
    // Close the editor
    editor_shutdown();

    KYRA_LOG_PRINT("EDITOR_APP", LOGGER_VERBOSITY_INFO, "App: editor shut down.");

    // Destruct application logger
    LoggerResult logger_result = logger_unregister("EDITOR_APP");
    if (logger_result != LOGGER_SUCCESS)
        KYRA_CONSOLE_PRINT_ERROR("EditorApp: Failed to unregister logger (Error: %s)", logger_result_to_string(logger_result));
}

Application *application_create(void) {
    static Application app;

    ConstStr config_filepath = "sandbox/assets/config/editor_config.kyra";

    // Configure application
    if (application_configure(config_filepath, &app) != APPLICATION_SUCCESS)
        return NULL;

    app.on_startup = on_startup;
    app.on_update = on_update;
    app.on_shutdown = on_shutdown;
    
    // Construct logger for application
    LoggerResult logger_result = logger_register("EDITOR_APP", "editor_app.log", LOGGER_FLAG_ALL);
    if (logger_result != LOGGER_SUCCESS)
        KYRA_CONSOLE_PRINT_ERROR("EditorApp: Failed to register logger (Error: %s)", logger_result_to_string(logger_result));

    return &app;
}

