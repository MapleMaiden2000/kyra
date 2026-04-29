@echo off
setlocal enabledelayedexpansion

:: Kyra Engine Bootstrap
:: Installs all required tools and libraries

:: Premake installation configurations
set PREMAKE_VERSION=5.0.0-beta2
set PREMAKE_URL=https://github.com/premake/premake-core/releases/download/v%PREMAKE_VERSION%/premake-%PREMAKE_VERSION%-windows.zip
set PREMAKE_DIR=external\premake
set PREMAKE_ZIP=%PREMAKE_DIR%\premake.zip

:: cJSON config                                                          
set CJSON_BASE_URL=https://raw.githubusercontent.com/DaveGamble/cJSON/master
set CJSON_DIR=external\cjson

echo ==== Kyra Engine Bootstrap ====

:: Ensure full directory structure
if not exist external mkdir external
if not exist %PREMAKE_DIR% mkdir %PREMAKE_DIR%
if not exist %CJSON_DIR% mkdir %CJSON_DIR%

:: Download and extract Premake
if not exist %PREMAKE_DIR%\premake5.exe (
    echo [Bootstrap-Info] Premake not found. Downloading Premake ^(v%PREMAKE_VERSION%^)...
    powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri %PREMAKE_URL% -OutFile %PREMAKE_ZIP%"
    if %ERRORLEVEL% neq 0 (
        echo [Bootstrap-Error] Failed to download Premake.
        exit /b 1
    )

    echo [Bootstrap-Info] Extracting Premake...
    powershell -Command "Expand-Archive -Path %PREMAKE_ZIP% -DestinationPath %PREMAKE_DIR% -Force"
    if %ERRORLEVEL% neq 0 (
        echo [Bootstrap-Error] Failed to extract Premake.
        exit /b 1
    )

    echo [Bootstrap-Info] Cleaning up...
    del %PREMAKE_ZIP%
    
    echo [Bootstrap-Info] Premake installed successfully.
) else (
    echo [Bootstrap-Info] Premake found.
)

:: Download cJSON
if not exist %CJSON_DIR%\cJSON.c (
    echo [Bootstrap-Info] Downloading cJSON...

    set CJSON_FILES=cJSON.c cJSON.h README.md LICENSE
    for %%f in (!CJSON_FILES!) do (
        echo [Bootstrap-Info] Downloading %%f...
        powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri %CJSON_BASE_URL%/%%f -OutFile %CJSON_DIR%\%%f"
        if !ERRORLEVEL! neq 0 (
            echo [Bootstrap-Error] Failed to download %%f.
            exit /b 1
        )
    )

    echo [Bootstrap-Info] cJSON installed successfully.
) else (
    echo [Bootstrap-Info] cJSON found.
)

:: Report result
if %ERRORLEVEL% equ 0 (
    echo [Bootstrap-Info] Kyra Engine environment is ready!
    echo [Bootstrap-Info] You can now run 'scripts\win32\generate.bat' to generate the project.
    echo [Bootstrap-Info] Then run 'scripts\win32\make.bat' to build the project.
) else (
    echo [Bootstrap-Error] Failed to generate projects during bootstrap.
    exit /b %ERRORLEVEL%
)

echo ===============================

@echo on
