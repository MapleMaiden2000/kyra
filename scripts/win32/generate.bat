@echo off
setlocal enabledelayedexpansion

:: Kyra Engine Project Generator
:: Re-generates the GNU Makefiles using Premakes

set PREMAKE_DIR=external\premake
set PREMAKE_EXE=%PREMAKE_DIR%\premake5.exe

echo ==== Kyra Engine Project Generator ====

echo [Generate-Info] Generating GNU Makefiles (using gmake2)...

if not exist %PREMAKE_EXE% (
    echo [Generate-Error] Premake executable not found at %PREMAKE_EXE%.
    echo Please run scripts\win32\bootstrap.bat to install all the required tools and libraries.
    exit /b 1
)

:: Create output directories
echo [Generate-Info] Creating output directories...
powershell -Command "New-Item -ItemType Directory -Path out/bin/Debug, out/bin/Release, out/int/Debug/engine, out/int/Release/engine, out/int/Debug/sandbox, out/int/Release/sandbox -Force | Out-Null"

:: Run Premake (gmake2)
%PREMAKE_EXE% gmake2

:: Report result
if %ERRORLEVEL% equ 0 (
    echo [Generate-Info] Project files generated successfully.
) else (
    echo [Generate-Error] Failed to generate project files.
    exit /b %ERRORLEVEL%
)

echo =======================================

@echo on
