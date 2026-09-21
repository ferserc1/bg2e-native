# update_libraries.ps1
#
# Builds the bg2e library with CMake and updates the bg2e headers and Windows
# libraries in a standalone project previously created with
# create_standalone_project.ps1.
#
# The build uses a dedicated directory (build-update-libraries/) so it does
# not interfere with the CLion build directory nor with the temporary build
# trees used by the package_*.ps1 scripts.
#
# Usage:
#   scripts\update_libraries.ps1 <standalone_project_path> -Configuration Debug|Release [-VulkanSdk <path>]

param(
    [Parameter(Mandatory=$true, Position=0)]
    [string]$TargetPath,
    [Parameter(Mandatory=$true)]
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration,
    [string]$VulkanSdk = $env:VULKAN_SDK
)

$ErrorActionPreference = "Stop"

function Stop-WithError {
    param([string]$Message)
    Write-Error $Message
    exit 1
}

function Invoke-Checked {
    param([string]$Command, [string[]]$Arguments)
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) { Stop-WithError "$Command failed with exit code $LASTEXITCODE" }
}

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $ScriptDir

if (-not $VulkanSdk -or -not (Test-Path -LiteralPath $VulkanSdk -PathType Container)) {
    Stop-WithError "Specify -VulkanSdk or set the VULKAN_SDK environment variable to a valid Vulkan SDK directory."
}

$null = Get-Command cmake -ErrorAction Stop

if (-not (Test-Path -Path $TargetPath -PathType Container)) {
    Stop-WithError "Standalone project directory does not exist: $TargetPath"
}

$ResolvedTargetPath = (Resolve-Path $TargetPath).Path
$ResolvedRepoRoot = (Resolve-Path $RepoRoot).Path

if ($ResolvedTargetPath -eq $ResolvedRepoRoot) {
    Stop-WithError "Refusing to use the bg2e-native repository as the target"
}

$BuildDir = "$ResolvedRepoRoot\build-update-libraries"
$Products = "$BuildDir\bin\windows"
# Visual Studio is a multi-config generator: binaries land in a per-configuration subdirectory.
$LibDir = "$Products\$Configuration"

$SourceIncludeDir = "$ResolvedRepoRoot\lib\include"
$TargetIncludeDir = "$ResolvedTargetPath\include"
$TargetLibraryDir = "$ResolvedTargetPath\lib\windows"

if (-not (Test-Path "$SourceIncludeDir\bg2e.hpp" -PathType Leaf)) {
    Stop-WithError "Cannot find bg2e.hpp at $SourceIncludeDir\bg2e.hpp"
}
if (-not (Test-Path "$SourceIncludeDir\bg2e" -PathType Container)) {
    Stop-WithError "Cannot find bg2e headers at $SourceIncludeDir\bg2e"
}

if (-not (Test-Path "$ResolvedTargetPath\CMakeLists.txt" -PathType Leaf)) {
    Stop-WithError "Target does not look like a standalone project: missing CMakeLists.txt"
}
if (-not (Test-Path "$ResolvedTargetPath\cmake\standalone_utils.cmake" -PathType Leaf)) {
    Stop-WithError "Target does not look like a generated standalone project: missing cmake\standalone_utils.cmake"
}
if (-not (Test-Path $TargetIncludeDir -PathType Container)) {
    Stop-WithError "Target does not contain an include directory"
}
if (-not (Test-Path $TargetLibraryDir -PathType Container)) {
    Stop-WithError "Target does not contain lib\windows"
}

Write-Host "bg2e Standalone Library Updater"
Write-Host "================================"
Write-Host "  Repository:    $ResolvedRepoRoot"
Write-Host "  Target:        $ResolvedTargetPath"
Write-Host "  Platform:      windows"
Write-Host "  Configuration: $Configuration"
Write-Host "  Build dir:     $BuildDir"
Write-Host ""

Write-Host "Configuring and building bg2e ($Configuration)..."
Invoke-Checked cmake @('-S', $ResolvedRepoRoot, '-B', $BuildDir, '-G', 'Visual Studio 17 2022', '-A', 'x64',
    "-DVULKAN_SDK=$VulkanSdk", "-DPRODUCT_DIR=$Products", "-DCMAKE_BUILD_TYPE=$Configuration",
    '-DBG2E_BUILD_EXAMPLES=OFF', '-DBG2E_BUILD_TESTS=OFF')
Invoke-Checked cmake @('--build', $BuildDir, '--config', $Configuration, '--target', 'bg2e', '--parallel')

if (-not (Test-Path "$LibDir\bg2e.dll" -PathType Leaf)) {
    Stop-WithError "Build finished but the library was not found at $LibDir\bg2e.dll"
}
if (-not (Test-Path "$LibDir\bg2e.lib" -PathType Leaf)) {
    Stop-WithError "Cannot find the import library at $LibDir\bg2e.lib"
}

Write-Host "Updating engine headers..."
$TargetHeaderTree = "$TargetIncludeDir\bg2e"
if (Test-Path $TargetHeaderTree) {
    Remove-Item $TargetHeaderTree -Recurse -Force
}
Copy-Item "$SourceIncludeDir\bg2e.hpp" "$TargetIncludeDir\bg2e.hpp" -Force
Copy-Item "$SourceIncludeDir\bg2e" $TargetHeaderTree -Recurse -Force

Write-Host "Updating pre-compiled libraries..."
Copy-Item "$LibDir\bg2e.dll" "$TargetLibraryDir\bg2e.dll" -Force
Copy-Item "$LibDir\bg2e.lib" "$TargetLibraryDir\bg2e.lib" -Force

Write-Host ""
Write-Host "Standalone project libraries updated successfully."
