#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/types.h"


// Application structure ------------------------------------------------ //

typedef struct Application {
    ConstStr    name;

    void        (*on_startup)(struct Application *app);
    void        (*on_update)(struct Application *app, Flt32 delta_time);
    void        (*on_shutdown)(struct Application *app);
} Application;


// External API --------------------------------------------------------- //

extern Application  *application_create(ConstStr config_filepath);



