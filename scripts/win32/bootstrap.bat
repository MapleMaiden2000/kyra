@echo off
setlocal enabledelayedexpansion

:: Kyra Engine Bootstrap
:: Installs all required tools and libraries

:: Premake installation configurations
set PREMAKE_VERSION=5.0.0-beta2
set PREMAKE_URL=https://github.com/premake/premake-core/releases/download/v%PREMAKE_VERSION%/premake-%PREMAKE_VERSION%-windows.zip
set PREMAKE_DIR=external\premake
set PREMAKE_ZIP=%PREMAKE_DIR%\premake.zip

echo ==== Kyra Engine Bootstrap ====

:: Ensure full directory structure
if not exist external mkdir external
if not exist %PREMAKE_DIR% mkdir %PREMAKE_DIR%

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

:: Generate project files
echo [Bootstrap-Info] Generating project files...
call scripts\win32\generate.bat

:: Report result
if %ERRORLEVEL% equ 0 (
    echo [Bootstrap-Info] Kyra Engine environment is ready!
    echo [Bootstrap-Info] You can now run 'scripts\win32\make.bat' to build the project.
) else (
    echo [Bootstrap-Error] Failed to generate projects during bootstrap.
    exit /b %ERRORLEVEL%
)

echo ===============================

@echo on
