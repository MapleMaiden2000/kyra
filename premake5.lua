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
        "KYRA_SHORT_LINE_MAX_LENGTH=256",
        "KYRA_LINE_MAX_LENGTH=1024",
        "KYRA_LOG_MESSAGE_MAX_LENGTH=4096",
        "KYRA_MEMORY_ALIGNMENT_SIZE=16",
        "KYRA_CONTAINER_RESIZE_RATIO=1.618f",
        "KYRA_CONTAINER_DEFAULT_CAPACITY=16"
    }    

    files {
        "engine/src/**.h",
        "engine/src/**.c",
        "external/cjson/cJSON.c",
        "external/cjson/cJSON.h"
    }

    includedirs {
        "engine/src",
        "external/cjson",
        "external/glfw/include"
    }

    libdirs {
        "external/glfw/lib-mingw-w64"
    }

    links {
        "glfw3",
        "gdi32",
        "opengl32",
        "shell32"
    }


-- Editor Project Configuration --

project "editor"
    kind "SharedLib"
    targetname "kyra_editor"
    staticruntime "on"

    defines { "KYRA_EDITOR_EXPORT" }

    files {
        "editor/src/**.h",
        "editor/src/**.c",
        "external/cjson/cJSON.c",
        "external/cjson/cJSON.h"
    }

    removefiles { "editor/src/kyra_editor/main.c" }

    includedirs {
        "engine/src",
        "editor/src",
        "external/cjson",
        "external/glfw/include"
    }

    links { "engine" }


-- Editor Application Project Configuration --

project "editor_app"
    kind "ConsoleApp"
    staticruntime "on"

    files { "editor/src/kyra_editor/main.c" }

    includedirs {
        "engine/src",
        "editor/src",
        "external/cjson"
    }

    links {
        "engine",
        "editor"
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
        "editor/src",
        "sandbox/src",
        "external/cjson"
    }

    links { 
        "engine",
        "editor"  
    }
