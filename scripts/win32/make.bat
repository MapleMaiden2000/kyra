@echo off
setlocal enabledelayedexpansion

:: Kyra Engine Project Builder
:: Automatically detects compiler and "make" utility

:: Force utf-8 encoding for clean output if possible
chcp 65001 >nul 2>nul

echo ==== Kyra Engine Project Builder ====

:: Detect build environment
:: If Makefile is missing, generate it
:: If both Makefile and Premake are missing, initiate the full bootstrap process
echo [Make-Info] Detecting build environment...
if not exist Makefile (
    if exist external\premake\premake5.exe (
        echo [Make-Info] Makefile not found. Generating projects...
        call scripts\win32\generate.bat
    ) else (
        echo [Make-Info] Makefile and Premake not found. Running bootstrap...
        call scripts\win32\boostrap.bat
    )
)

:: Detect compiler (GCC)
where gcc >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [Make-Error] gcc not found in PATH.
    echo Please make sure MinGW-w64 is installed and added to your environment variables.
    exit /b 1
) else (
    for /f "tokens=*" %%i in ('gcc --version ^| findstr /c:"gcc"') do set GCC_VER=%%i
    echo [Make-Info] Found gcc compiler ^(v!GCC_VER!^)
)

:: Detect "make" utility (mingw32-make or make)
set MAKE_CMD=
where mingw32-make >nul 2>nul
if %ERRORLEVEL% equ 0 (
    set MAKE_CMD=mingw32-make
) else (
    where make >nul 2>nul
    if %ERRORLEVEL% equ 0 (
        set MAKE_CMD=make
    )
)

:: Build project
echo [Make-Info] Executing: %MAKE_CMD% CC=gcc %*
%MAKE_CMD% CC=gcc %*

:: Report result
if %ERRORLEVEL% equ 0 (
    echo.
    echo [Make-Info] Build completed successfully.
) else (
    echo.
    echo [Make-Error] Build failed with exit code %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)

echo =====================================

@echo on
