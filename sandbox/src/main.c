#include <kyra/kyra.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


static void on_startup(Application *app) {
    KYRA_PRINT_INFO("Sandbox: Application startup completed.");
}

static void on_update(Application *app, float delta_time) {
    KYRA_PRINT_INFO("Sandbox: Application updated.");
}

static void on_shutdown(Application *app) {
    KYRA_PRINT_INFO("Sandbox: Application shutdown completed.");
}


Application *application_create(ConstStr config_filepath) {
    if (!config_filepath) return NULL;

    static Application app;

    // Configure application
    if (application_configure(config_filepath, &app.config) != APPLICATION_SUCCESS)
        return NULL;

    app.on_startup = on_startup;
    app.on_update = on_update;
    app.on_shutdown = on_shutdown;

    return &app;
}

