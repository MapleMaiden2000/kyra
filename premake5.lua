-- Kyra Workspace Configuration --

workspace "kyra"
    configurations { "Debug", "Release" }
    platforms { "x64" }
    architecture "x86_64"

    toolset "gcc"
    language "C"
    cdialect "C11"

    targetdir ("out/bin/%{cfg.buildcfg}")
    objdir ("out/int/%{cfg.buildcfg}/%{prj.name}")

    filter "configurations:Debug"
        defines { "KYRA_DEBUG", "KYRA_ENABLE_ASSERTIONS" }
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines { "KYRA_RELEASE" }
        runtime "Release"
        optimize "on"

    filter "platforms:x64"
        buildoptions { "-mavx2", "-mfma" }


-- Engine Project Configuration --    

project "engine"
    kind "SharedLib"
    targetname "kyra_engine"
    staticruntime "on"

    defines { 
        "KYRA_EXPORT",
        "KYRA_LINE_MAX_LENGTH=1024",
        "KYRA_MEMORY_ALIGNMENT_SIZE=16",
    }    

    files {
        "engine/src/**.h",
        "engine/src/**.c",
        "external/cjson/cJSON.c",
        "external/cjson/cJSON.h"
    }

    includedirs {
        "engine/src",
        "external/cjson"
    }


-- Sandbox Project Configuration --

project "sandbox"
    kind "ConsoleApp"
    staticruntime "on"

    files {
        "sandbox/src/**.h",
        "sandbox/src/**.c"
    }

    includedirs {
        "engine/src",
        "sandbox/src",
    }

    links { "engine" }
