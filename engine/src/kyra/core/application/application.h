#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/types.h"


// Return codes --------------------------------------------------------- //

typedef enum Application_Result {
    APPLICATION_SUCCESS                                         = 0,
        
    APPLICATION_ERROR_REF_OUT_CONFIG_NULL                       = -1,
    APPLICATION_ERROR_FAILED_TO_OPEN_CONFIG_FILE                = -2,
    APPLICATION_ERROR_FAILED_TO_GET_FILE_SIZE                   = -3,
    APPLICATION_ERROR_FAILED_TO_CLOSE_CONFIG_FILE               = -4,
    APPLICATION_ERROR_FAILED_TO_PARSE_TO_JSON                   = -5,

} ApplicationResult;


// Application configuration -------------------------------------------- //

typedef struct Application_Config {
    Str                 name;
} ApplicationConfig;


// Application structure ------------------------------------------------ //

typedef struct Application {
    ApplicationConfig   config;

    void                (*on_startup)(struct Application *app);
    void                (*on_update)(struct Application *app, Flt32 delta_time);
    void                (*on_shutdown)(struct Application *app);
} Application;


// API functions -------------------------------------------------------- //

KYRA_ENGINE_API ApplicationResult   application_configure(ConstStr config_filepath, ApplicationConfig *out_config);


// External API --------------------------------------------------------- //

extern Application                 *application_create(ConstStr config_filepath);



