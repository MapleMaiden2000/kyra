#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/containers.h"


// Return codes --------------------------------------------------------- //

typedef enum Application_Result {
    APPLICATION_SUCCESS                                             = 0,
        
    APPLICATION_ERROR_REF_OUT_APPLICATION_NULL                      = -1,
    APPLICATION_ERROR_REF_APPLICATION_NULL                          = -2,
    APPLICATION_ERROR_FAILED_TO_OPEN_CONFIG_FILE                    = -3,
    APPLICATION_ERROR_FAILED_TO_GET_FILE_SIZE                       = -4,
    APPLICATION_ERROR_FAILED_TO_CLOSE_CONFIG_FILE                   = -5,
    APPLICATION_ERROR_FAILED_TO_PARSE_TO_JSON                       = -6,
    APPLICATION_ERROR_FAILED_TO_CONSTRUCT_APPLICATION_NAME_STRING   = -7

} ApplicationResult;


// Application structure ------------------------------------------------ //

typedef struct Application {
    String          name;
    
    Int32           argc;
    Str            *argv;
    
    Bool            is_running;
    VoidPtr         user_data;

    void            (*on_startup)(struct Application *app);
    void            (*on_update)(struct Application *app, Flt32 delta_time);
    void            (*on_shutdown)(struct Application *app);

} Application;


// API functions -------------------------------------------------------- //

KYRA_ENGINE_API ApplicationResult   application_configure(ConstStr config_filepath, Application *out_app);
KYRA_ENGINE_API ApplicationResult   application_request_shutdown(Application *app);

KYRA_ENGINE_API ConstStr            application_result_to_string(const ApplicationResult result);


// External API --------------------------------------------------------- //

extern Application                 *application_create(void);



