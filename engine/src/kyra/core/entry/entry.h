#pragma once

#include "kyra/kyra.h"

#include <stdlib.h>


Int32 main(Int32 argc, Str *argv) {
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
        Application *app = application_create();
        if (!app) {
            // If failed to create app, shutdown the engine            
            engine_result = engine_destruct();
            if (engine_result != ENGINE_SUCCESS) return (Int32)engine_result;
            
            return -1;
        }

        // Pass CLI arguments to the application
        app->argc = argc;
        app->argv = argv;
        app->is_running = true;

        // Application startup
        if (app->on_startup) app->on_startup(app);

        // Main loop
        {
            HiResClock clock;
            clock_hires_split(&clock);

            while (app->is_running && engine_is_running()) {
                Flt64 elapsed = 0.0;
                clock_hires_elapsed_seconds(clock, &elapsed);

                // Reset for next frame
                clock_hires_split(&clock);

                // Update stage
                if (app->on_update) app->on_update(app, (Flt32)elapsed);
            }
        }

        // Shutdown stage
        if (app->on_shutdown) app->on_shutdown(app);

        // Deallocate application properties
        {
            // Application name
            container_string_destruct(&app->name);
        }
    }

    // Engine shutdown
    {
        engine_result = engine_destruct();
        if (engine_result != ENGINE_SUCCESS) return (Int32)engine_result;
    }

    return 0;
}








