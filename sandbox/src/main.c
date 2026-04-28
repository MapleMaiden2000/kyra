#include <kyra/kyra.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


static void on_startup(Application *app) {
    printf("Sandbox: Application '%s' startup completed.\n", app->name);
}

static void on_update(Application *app, float delta_time) {
    printf("Sandbox: Application '%s' updated.\n", app->name);
}

static void on_shutdown(Application *app) {
    printf("Sandbox: Application '%s' shutdown completed.\n", app->name);
}


Application *application_create(ConstStr config_filepath) {
    if (!config_filepath) return NULL;

    static Application app;
    app.name = "Sandbox App";
    app.on_startup = on_startup;
    app.on_update = on_update;
    app.on_shutdown = on_shutdown;

    return &app;
}

