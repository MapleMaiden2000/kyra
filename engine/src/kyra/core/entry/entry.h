#pragma once

#include "kyra/kyra.h"


Int32 main(Int32 argc, Int32 argv) {
    EngineResult engine_result;
    
    // Engine startup
    {
        ConstStr engine_config_filepath = "sandbox/configs/engine_config.kyra";
        
        // Pre-construction stage
        engine_result = engine_preconstruct(engine_config_filepath);
        if (engine_result != ENGINE_SUCCESS) return (Int32)engine_result;
        
        // Construction stage
        engine_result = engine_construct();
        if (engine_result != ENGINE_SUCCESS) return (Int32)engine_result;
    }

    // Application
    {
        ConstStr app_config_filepath = "sandbox/configs/app_config.kyra";

        Application *app = application_create(app_config_filepath);
        if (!app) {
            // If failed to create app, shutdown the engine            
            engine_result = engine_destruct();
            if (engine_result != ENGINE_SUCCESS) return (Int32)engine_result;
            
            return -1;
        }

        // Startup stage
        if (app->on_startup) app->on_startup(app);

        // Main loop
        {
            // Update engine
            engine_update(0.0f);

            // Update stage
            if (app->on_update) app->on_update(app, 0.0f);
        }

        // Shutdown stage
        if (app->on_shutdown) app->on_shutdown(app);
    }

    // Engine shutdown
    {
        engine_result = engine_destruct();
        if (engine_result != ENGINE_SUCCESS) return (Int32)engine_result;
    }

    return 0;
}








