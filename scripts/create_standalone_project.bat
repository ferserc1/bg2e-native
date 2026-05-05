@echo off
setlocal enabledelayedexpansion

REM ============================================================================
REM create_standalone_project.bat
REM
REM Generates a standalone CMake project for building bg2e applications
REM outside the bg2e-native repository. Uses pre-compiled libraries.
REM
REM Usage:
REM   scripts\create_standalone_project.bat <target_path> [project_name]
REM
REM Arguments:
REM   target_path   - (required) Path where the project will be created
REM   project_name  - (optional) CMake project name (default: bg2e_app)
REM ============================================================================

REM ============================================================================
REM Parse arguments
REM ============================================================================

if "%~1"=="" (
    echo Usage: %~nx0 ^<target_path^> [project_name]
    echo.
    echo   target_path   Path where the project will be created
    echo   project_name  CMake project name (default: bg2e_app)
    exit /b 1
)

set "TARGET_PATH=%~1"
if "%~2"=="" (
    set "PROJECT_NAME=bg2e_app"
) else (
    set "PROJECT_NAME=%~2"
)

REM ============================================================================
REM Detect script location and repository root
REM ============================================================================

set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
for %%i in ("%SCRIPT_DIR%") do set "REPO_ROOT=%%~dpi"
set "REPO_ROOT=%REPO_ROOT:~0,-1%"

REM ============================================================================
REM Validate repository and binaries
REM ============================================================================

echo bg2e Standalone Project Generator
echo ==================================
echo   Repository:  %REPO_ROOT%
echo   Target:      %TARGET_PATH%
echo   Project:     %PROJECT_NAME%
echo   Platform:    windows
echo.

if not exist "%REPO_ROOT%\lib\include\bg2e" (
    echo ERROR: Cannot find bg2e headers at %REPO_ROOT%\lib\include\bg2e
    exit /b 1
)

if not exist "%REPO_ROOT%\assets" (
    echo ERROR: Cannot find assets directory at %REPO_ROOT%\assets
    exit /b 1
)

if not exist "%REPO_ROOT%\bin\windows\shaders" (
    echo ERROR: Cannot find compiled shaders at %REPO_ROOT%\bin\windows\shaders
    echo   Build the project first: cmake --build build
    exit /b 1
)

if not exist "%REPO_ROOT%\bin\windows\bg2e.dll" (
    echo ERROR: Cannot find pre-compiled library: %REPO_ROOT%\bin\windows\bg2e.dll
    echo   Build the project first: cmake --build build
    exit /b 1
)

if not exist "%REPO_ROOT%\bin\windows\bg2e.lib" (
    echo ERROR: Cannot find import library: %REPO_ROOT%\bin\windows\bg2e.lib
    echo   Build the project first: cmake --build build
    exit /b 1
)

if not exist "%SCRIPT_DIR%\templates\main.cpp" (
    echo ERROR: Cannot find template: %SCRIPT_DIR%\templates\main.cpp
    exit /b 1
)

if exist "%TARGET_PATH%" (
    echo ERROR: Target directory already exists: %TARGET_PATH%
    exit /b 1
)

REM ============================================================================
REM Create directory structure
REM ============================================================================

echo Creating directory structure...

mkdir "%TARGET_PATH%"
mkdir "%TARGET_PATH%\cmake"
mkdir "%TARGET_PATH%\include"
mkdir "%TARGET_PATH%\lib\linux"
mkdir "%TARGET_PATH%\lib\macos"
mkdir "%TARGET_PATH%\lib\windows"
mkdir "%TARGET_PATH%\shaders"
mkdir "%TARGET_PATH%\assets"
mkdir "%TARGET_PATH%\app\src"

REM ============================================================================
REM Copy files
REM ============================================================================

echo Copying engine headers...
copy "%REPO_ROOT%\lib\include\bg2e.hpp" "%TARGET_PATH%\include\" >nul
xcopy "%REPO_ROOT%\lib\include\bg2e" "%TARGET_PATH%\include\bg2e\" /E /I /Q >nul

echo Copying assets...
xcopy "%REPO_ROOT%\assets" "%TARGET_PATH%\assets\" /E /I /Q >nul

echo Copying compiled shaders...
xcopy "%REPO_ROOT%\bin\windows\shaders\*.spv" "%TARGET_PATH%\shaders\" /Q >nul

echo Copying CMake modules...
copy "%REPO_ROOT%\cmake\FindVulkan.cmake" "%TARGET_PATH%\cmake\" >nul
copy "%REPO_ROOT%\cmake\FindSDL2.cmake" "%TARGET_PATH%\cmake\" >nul
copy "%REPO_ROOT%\cmake\standalone_utils.cmake" "%TARGET_PATH%\cmake\" >nul

echo Copying pre-compiled library ^(windows^)...
copy "%REPO_ROOT%\bin\windows\bg2e.dll" "%TARGET_PATH%\lib\windows\" >nul
copy "%REPO_ROOT%\bin\windows\bg2e.lib" "%TARGET_PATH%\lib\windows\" >nul

REM Copy SDL2 library if available
if exist "%REPO_ROOT%\bin\windows\SDL2.dll" (
    copy "%REPO_ROOT%\bin\windows\SDL2.dll" "%TARGET_PATH%\lib\windows\" >nul
)

REM ============================================================================
REM Create TODO.md for other platforms
REM ============================================================================

echo Creating platform placeholders...

(
echo # Platform Libraries Required
echo.
echo This directory should contain the pre-compiled bg2e library for Linux.
echo.
echo ## Required Files
echo.
echo - `libbg2e.so` — the bg2e shared library
echo.
echo ## How to Build for This Platform
echo.
echo 1. Clone bg2e-native on a Linux machine:
echo    ```
echo    git clone --recursive ^<bg2e-native-repo-url^>
echo    ```
echo.
echo 2. Build the engine:
echo    ```
echo    cmake -S . -B build -G Ninja -DVULKAN_SDK=/path/to/vulkan/sdk
echo    cmake --build build
echo    ```
echo.
echo 3. Copy `bin/linux/libbg2e.so` to this directory.
echo.
echo ## Notes
echo.
echo - The library must be compiled with a compatible C++20 compiler
echo - GCC 11+ or Clang 13+ is recommended
) > "%TARGET_PATH%\lib\linux\TODO.md"

(
echo # Platform Libraries Required
echo.
echo This directory should contain the pre-compiled bg2e library for macOS.
echo.
echo ## Required Files
echo.
echo - `libbg2e.dylib` — the bg2e shared library
echo.
echo ## How to Build for This Platform
echo.
echo 1. Clone bg2e-native on a macOS machine:
echo    ```
echo    git clone --recursive ^<bg2e-native-repo-url^>
echo    ```
echo.
echo 2. Build the engine:
echo    ```
echo    cmake -S . -B build -G Xcode -DVULKAN_SDK=/path/to/vulkan/sdk
echo    cmake --build build
echo    ```
echo.
echo 3. Copy `bin/macos/libbg2e.dylib` to this directory.
echo.
echo ## Notes
echo.
echo - Xcode is required for native file dialogs
echo - You may also need to copy MoltenVK and Vulkan validation layer dylibs
echo - Install the Vulkan SDK from https://vulkan.lunarg.com/sdk/home
) > "%TARGET_PATH%\lib\macos\TODO.md"

REM ============================================================================
REM Generate CMakeLists.txt files from templates
REM ============================================================================

echo Generating CMakeLists.txt...

if not exist "%SCRIPT_DIR%\templates\CMakeLists.txt.in" (
    echo ERROR: Cannot find template: %SCRIPT_DIR%\templates\CMakeLists.txt.in
    exit /b 1
)

if not exist "%SCRIPT_DIR%\templates\app_CMakeLists.txt.in" (
    echo ERROR: Cannot find template: %SCRIPT_DIR%\templates\app_CMakeLists.txt.in
    exit /b 1
)

powershell -Command "(Get-Content '%SCRIPT_DIR%\templates\CMakeLists.txt.in') -replace '\$\{PROJECT_NAME\}', '%PROJECT_NAME%' | Set-Content '%TARGET_PATH%\CMakeLists.txt'"
powershell -Command "(Get-Content '%SCRIPT_DIR%\templates\app_CMakeLists.txt.in') -replace '\$\{PROJECT_NAME\}', '%PROJECT_NAME%' | Set-Content '%TARGET_PATH%\app\CMakeLists.txt'"

REM ============================================================================
REM Copy app/main.cpp from template
REM ============================================================================

echo Copying app\src\main.cpp from template...
copy "%SCRIPT_DIR%\templates\main.cpp" "%TARGET_PATH%\app\src\main.cpp" >nul

REM ============================================================================
REM Print summary
REM ============================================================================

echo.
echo Project created successfully!
echo.
echo Project structure:
echo   %TARGET_PATH%\
echo   +-- CMakeLists.txt
echo   +-- cmake\
echo   ^|   +-- standalone_utils.cmake
echo   ^|   +-- FindVulkan.cmake
echo   ^|   +-- FindSDL2.cmake
echo   +-- include\bg2e\
echo   +-- lib\
echo   ^|   +-- linux\    (TODO.md)
echo   ^|   +-- macos\    (TODO.md)
echo   ^|   +-- windows\  (bg2e.dll, bg2e.lib)
echo   +-- shaders\
echo   +-- assets\
echo   +-- app\
echo       +-- CMakeLists.txt
echo       +-- src\
echo           +-- main.cpp
echo.
echo To build:
echo   cd %TARGET_PATH%
echo   cmake -S . -B build -G "Visual Studio 17 2022" -DVULKAN_SDK=C:\VulkanSDK
echo   cmake --build build --config Release
echo.

endlocal
