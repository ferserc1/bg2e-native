# update_libraryes.ps1
#
# Updates the bg2e headers and Windows libraries in a standalone project
# previously created with create_standalone_project.ps1.
#
# Usage:
#   scripts\update_libraryes.ps1 <standalone_project_path>

param(
    [Parameter(Mandatory=$true, Position=0)]
    [string]$TargetPath
)

$ErrorActionPreference = "Stop"

function Stop-WithError {
    param([string]$Message)
    Write-Error $Message
    exit 1
}

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $ScriptDir

if (-not (Test-Path -Path $TargetPath -PathType Container)) {
    Stop-WithError "Standalone project directory does not exist: $TargetPath"
}

$ResolvedTargetPath = (Resolve-Path $TargetPath).Path
$ResolvedRepoRoot = (Resolve-Path $RepoRoot).Path

if ($ResolvedTargetPath -eq $ResolvedRepoRoot) {
    Stop-WithError "Refusing to use the bg2e-native repository as the target"
}

$LibDir = $null
if (Test-Path "$ResolvedRepoRoot\bin\windows\Release\bg2e.dll") {
    $LibDir = "$ResolvedRepoRoot\bin\windows\Release"
} elseif (Test-Path "$ResolvedRepoRoot\bin\windows\Debug\bg2e.dll") {
    $LibDir = "$ResolvedRepoRoot\bin\windows\Debug"
} elseif (Test-Path "$ResolvedRepoRoot\bin\windows\bg2e.dll") {
    $LibDir = "$ResolvedRepoRoot\bin\windows"
}

$SourceIncludeDir = "$ResolvedRepoRoot\lib\include"
$TargetIncludeDir = "$ResolvedTargetPath\include"
$TargetLibraryDir = "$ResolvedTargetPath\lib\windows"

if (-not (Test-Path "$SourceIncludeDir\bg2e.hpp" -PathType Leaf)) {
    Stop-WithError "Cannot find bg2e.hpp at $SourceIncludeDir\bg2e.hpp"
}
if (-not (Test-Path "$SourceIncludeDir\bg2e" -PathType Container)) {
    Stop-WithError "Cannot find bg2e headers at $SourceIncludeDir\bg2e"
}
if (-not $LibDir) {
    Stop-WithError "Cannot find a compiled bg2e.dll. Build bg2e-native first."
}
if (-not (Test-Path "$LibDir\bg2e.lib" -PathType Leaf)) {
    Stop-WithError "Cannot find the import library at $LibDir\bg2e.lib"
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
Write-Host "  Repository:  $ResolvedRepoRoot"
Write-Host "  Target:      $ResolvedTargetPath"
Write-Host "  Platform:    windows"
Write-Host "  Libraries:   $LibDir"
Write-Host ""

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
