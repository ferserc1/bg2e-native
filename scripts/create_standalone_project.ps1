# create_standalone_project.ps1
#
# Generates a standalone CMake project for building bg2e applications
# outside the bg2e-native repository. Uses pre-compiled libraries.
#
# Usage:
#   scripts\create_standalone_project.ps1 <target_path> [project_name]
#
# Arguments:
#   target_path   - (required) Path where the project will be created
#   project_name  - (optional) CMake project name (default: bg2e_app)

param(
    [Parameter(Mandatory=$true, Position=0)]
    [string]$TargetPath,

    [Parameter(Position=1)]
    [string]$ProjectName = "bg2e_app"
)

$ErrorActionPreference = "Stop"

# ============================================================================
# Detect script location and repository root
# ============================================================================

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $ScriptDir

# ============================================================================
# Detect library directory (Release or Debug)
# ============================================================================

$LibDir = $null
if (Test-Path "$RepoRoot\bin\windows\Release\bg2e.dll") {
    $LibDir = "$RepoRoot\bin\windows\Release"
} elseif (Test-Path "$RepoRoot\bin\windows\Debug\bg2e.dll") {
    $LibDir = "$RepoRoot\bin\windows\Debug"
} elseif (Test-Path "$RepoRoot\bin\windows\bg2e.dll") {
    $LibDir = "$RepoRoot\bin\windows"
}

# ============================================================================
# Validate repository and binaries
# ============================================================================

Write-Host "bg2e Standalone Project Generator"
Write-Host "=================================="
Write-Host "  Repository:  $RepoRoot"
Write-Host "  Target:      $TargetPath"
Write-Host "  Project:     $ProjectName"
Write-Host "  Platform:    windows"
Write-Host ""

if (-not (Test-Path "$RepoRoot\lib\include\bg2e")) {
    Write-Host "ERROR: Cannot find bg2e headers at $RepoRoot\lib\include\bg2e"
    exit 1
}

if (-not (Test-Path "$RepoRoot\assets")) {
    Write-Host "ERROR: Cannot find assets directory at $RepoRoot\assets"
    exit 1
}

if (-not (Test-Path "$RepoRoot\bin\windows\shaders")) {
    Write-Host "ERROR: Cannot find compiled shaders at $RepoRoot\bin\windows\shaders"
    Write-Host "  Build the project first: cmake --build build"
    exit 1
}

if (-not $LibDir) {
    Write-Host "ERROR: Cannot find pre-compiled library bg2e.dll"
    Write-Host "  Searched in:"
    Write-Host "    $RepoRoot\bin\windows\Release\"
    Write-Host "    $RepoRoot\bin\windows\Debug\"
    Write-Host "    $RepoRoot\bin\windows\"
    Write-Host "  Build the project first: cmake --build build"
    exit 1
}

if (-not (Test-Path "$LibDir\bg2e.lib")) {
    Write-Host "ERROR: Cannot find import library: $LibDir\bg2e.lib"
    Write-Host "  Build the project first: cmake --build build"
    exit 1
}

if (-not (Test-Path "$ScriptDir\templates\main.cpp")) {
    Write-Host "ERROR: Cannot find template: $ScriptDir\templates\main.cpp"
    exit 1
}

if (Test-Path $TargetPath) {
    Write-Host "ERROR: Target directory already exists: $TargetPath"
    exit 1
}

Write-Host "Using libraries from: $LibDir"
Write-Host ""

# ============================================================================
# Create directory structure
# ============================================================================

Write-Host "Creating directory structure..."

New-Item -ItemType Directory -Path $TargetPath -Force | Out-Null
New-Item -ItemType Directory -Path "$TargetPath\cmake" -Force | Out-Null
New-Item -ItemType Directory -Path "$TargetPath\include" -Force | Out-Null
New-Item -ItemType Directory -Path "$TargetPath\lib\linux" -Force | Out-Null
New-Item -ItemType Directory -Path "$TargetPath\lib\macos" -Force | Out-Null
New-Item -ItemType Directory -Path "$TargetPath\lib\windows" -Force | Out-Null
New-Item -ItemType Directory -Path "$TargetPath\shaders" -Force | Out-Null
New-Item -ItemType Directory -Path "$TargetPath\assets" -Force | Out-Null
New-Item -ItemType Directory -Path "$TargetPath\app\src" -Force | Out-Null

# ============================================================================
# Copy files
# ============================================================================

Write-Host "Copying engine headers..."
Copy-Item "$RepoRoot\lib\include\bg2e.hpp" "$TargetPath\include\" -Force
Copy-Item "$RepoRoot\lib\include\bg2e" "$TargetPath\include\bg2e\" -Recurse -Force

Write-Host "Copying assets..."
Copy-Item "$RepoRoot\assets\*" "$TargetPath\assets\" -Recurse -Force

Write-Host "Copying compiled shaders..."
Copy-Item "$RepoRoot\bin\windows\shaders\*.spv" "$TargetPath\shaders\" -Force

Write-Host "Copying CMake modules..."
Copy-Item "$RepoRoot\cmake\FindVulkan.cmake" "$TargetPath\cmake\" -Force
Copy-Item "$RepoRoot\cmake\FindSDL2.cmake" "$TargetPath\cmake\" -Force
Copy-Item "$RepoRoot\cmake\standalone_utils.cmake" "$TargetPath\cmake\" -Force

Write-Host "Copying pre-compiled library (windows)..."
Copy-Item "$LibDir\bg2e.dll" "$TargetPath\lib\windows\" -Force
Copy-Item "$LibDir\bg2e.lib" "$TargetPath\lib\windows\" -Force

if (Test-Path "$LibDir\SDL2.dll") {
    Copy-Item "$LibDir\SDL2.dll" "$TargetPath\lib\windows\" -Force
}

# ============================================================================
# Create TODO.md for other platforms
# ============================================================================

Write-Host "Creating platform placeholders..."

$linuxTodo = @"
# Platform Libraries Required

This directory should contain the pre-compiled bg2e library for Linux.

## Required Files

- libbg2e.so - the bg2e shared library

## How to Build for This Platform

1. Clone bg2e-native on a Linux machine:
   git clone --recursive <bg2e-native-repo-url>

2. Build the engine:
   cmake -S . -B build -G Ninja -DVULKAN_SDK=/path/to/vulkan/sdk
   cmake --build build

3. Copy bin/linux/libbg2e.so to this directory.

## Notes

- The library must be compiled with a compatible C++20 compiler
- GCC 11+ or Clang 13+ is recommended
"@

$macosTodo = @"
# Platform Libraries Required

This directory should contain the pre-compiled bg2e library for macOS.

## Required Files

- libbg2e.dylib - the bg2e shared library

## How to Build for This Platform

1. Clone bg2e-native on a macOS machine:
   git clone --recursive <bg2e-native-repo-url>

2. Build the engine:
   cmake -S . -B build -G Xcode -DVULKAN_SDK=/path/to/vulkan/sdk
   cmake --build build

3. Copy bin/macos/libbg2e.dylib to this directory.

## Notes

- Xcode is required for native file dialogs
- You may also need to copy MoltenVK and Vulkan validation layer dylibs
- Install the Vulkan SDK from https://vulkan.lunarg.com/sdk/home
"@

$linuxTodo | Out-File -FilePath "$TargetPath\lib\linux\TODO.md" -Encoding UTF8
$macosTodo | Out-File -FilePath "$TargetPath\lib\macos\TODO.md" -Encoding UTF8

# ============================================================================
# Generate CMakeLists.txt files from templates
# ============================================================================

Write-Host "Generating CMakeLists.txt..."

if (-not (Test-Path "$ScriptDir\templates\CMakeLists.txt.in")) {
    Write-Host "ERROR: Cannot find template: $ScriptDir\templates\CMakeLists.txt.in"
    exit 1
}

if (-not (Test-Path "$ScriptDir\templates\app_CMakeLists.txt.in")) {
    Write-Host "ERROR: Cannot find template: $ScriptDir\templates\app_CMakeLists.txt.in"
    exit 1
}

$template = Get-Content "$ScriptDir\templates\CMakeLists.txt.in" -Raw
$template = $template -replace '\$\{PROJECT_NAME\}', $ProjectName
$template | Out-File -FilePath "$TargetPath\CMakeLists.txt" -Encoding UTF8

$appTemplate = Get-Content "$ScriptDir\templates\app_CMakeLists.txt.in" -Raw
$appTemplate = $appTemplate -replace '\$\{PROJECT_NAME\}', $ProjectName
$appTemplate | Out-File -FilePath "$TargetPath\app\CMakeLists.txt" -Encoding UTF8

# ============================================================================
# Copy app/main.cpp from template
# ============================================================================

Write-Host "Copying app\src\main.cpp from template..."
Copy-Item "$ScriptDir\templates\main.cpp" "$TargetPath\app\src\main.cpp" -Force

# ============================================================================
# Print summary
# ============================================================================

Write-Host ""
Write-Host "Project created successfully!"
Write-Host ""
Write-Host "Project structure:"
Write-Host "  $TargetPath\"
Write-Host "  +-- CMakeLists.txt"
Write-Host "  +-- cmake\"
Write-Host "  |   +-- standalone_utils.cmake"
Write-Host "  |   +-- FindVulkan.cmake"
Write-Host "  |   +-- FindSDL2.cmake"
Write-Host "  +-- include\bg2e\"
Write-Host "  +-- lib\"
Write-Host "  |   +-- linux\    (TODO.md)"
Write-Host "  |   +-- macos\    (TODO.md)"
Write-Host "  |   +-- windows\  (bg2e.dll, bg2e.lib)"
Write-Host "  +-- shaders\"
Write-Host "  +-- assets\"
Write-Host "  +-- app\"
Write-Host "      +-- CMakeLists.txt"
Write-Host "      +-- src\"
Write-Host "          +-- main.cpp"
Write-Host ""
Write-Host "To build:"
Write-Host "  cd $TargetPath"
Write-Host "  cmake -S . -B build -G `"Visual Studio 17 2022`" -DVULKAN_SDK=C:\VulkanSDK"
Write-Host "  cmake --build build --config Release"
Write-Host ""
