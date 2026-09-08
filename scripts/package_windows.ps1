<#
.SYNOPSIS
Build and package the engine and apps from a clean Git checkout.
.EXAMPLE
.\scripts\package_windows.ps1 -Configuration Debug -Destination C:\packages
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('Debug', 'Release')][string]$Configuration,
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()][string]$Destination,
    [string]$VulkanSdk = $env:VULKAN_SDK
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
function Invoke-Checked {
    param([string]$Command, [string[]]$Arguments)
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Command failed with exit code $LASTEXITCODE" }
}
$repo = Split-Path -Parent $PSScriptRoot
# Read-only preflight must precede directory creation and every build command.
$status = @(Invoke-Checked git @('-C', $repo, 'status', '--porcelain=v1', '--untracked-files=all', '--ignore-submodules=none'))
if ($status.Count) { throw "Repository has pending changes. Commit or resolve them first:`n$($status -join "`n")" }
$submodules = @(Invoke-Checked git @('-C', $repo, 'submodule', 'status', '--recursive'))
if ($submodules | Where-Object { $_ -match '^[+U-]' }) { throw 'Submodules must be initialized and match their committed revisions.' }
$commit = (Invoke-Checked git @('-C', $repo, 'rev-parse', 'HEAD')).Trim()
$buildId = (Invoke-Checked git @('-C', $repo, 'rev-parse', '--short=12', 'HEAD')).Trim()
if ($env:OS -ne 'Windows_NT') { throw 'Run this script on Windows.' }
$null = Get-Command cmake -ErrorAction Stop
if (-not $VulkanSdk -or -not (Test-Path -LiteralPath $VulkanSdk -PathType Container)) { throw 'Specify -VulkanSdk or set VULKAN_SDK.' }
$destinationPath = [IO.Path]::GetFullPath($Destination)
$repoPrefix = [IO.Path]::GetFullPath($repo).TrimEnd('\') + '\'
if ($destinationPath -eq $repoPrefix.TrimEnd('\') -or $destinationPath.StartsWith($repoPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Destination must be outside the repository.'
}
$name = "bg2engine_$buildId"
$package = Join-Path $destinationPath $name
$archive = "$package.zip"
if ((Test-Path -LiteralPath $package) -or (Test-Path -LiteralPath $archive)) { throw "Output already exists: $package or $archive" }
# Unique build tree: no old binaries, caches or shaders and no destructive cleanup.
$build = Join-Path ([IO.Path]::GetTempPath()) ("bg2engine-build-" + [guid]::NewGuid().ToString('N'))
$products = Join-Path $build 'bin/windows'
Write-Host "Building $commit ($Configuration) in $build"
Invoke-Checked cmake @('-S', $repo, '-B', $build, '-G', 'Visual Studio 17 2022', '-A', 'x64',
    "-DVULKAN_SDK=$VulkanSdk", "-DPRODUCT_DIR=$products", '-DBG2E_BUILD_EXAMPLES=OFF',
    '-DBG2E_BUILD_TESTS=OFF', '-DBG2E_PACKAGE_APPS=ON')
Invoke-Checked cmake @('--build', $build, '--config', $Configuration, '--target', 'bg2e_distribution', '--parallel')
$status = @(Invoke-Checked git @('-C', $repo, 'status', '--porcelain=v1', '--untracked-files=all', '--ignore-submodules=none'))
$currentCommit = (Invoke-Checked git @('-C', $repo, 'rev-parse', 'HEAD')).Trim()
if ($status.Count -or $currentCommit -ne $commit) { throw 'Repository changed during the build; package aborted.' }
$null = New-Item -ItemType Directory -Path $package
Invoke-Checked cmake @('--install', $build, '--config', $Configuration, '--prefix', $package, '--component', 'Distribution')
@{ commit = $commit; configuration = $Configuration; platform = 'windows'; builtAtUtc = [DateTime]::UtcNow.ToString('o') } |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $package 'build-info.json') -Encoding UTF8
# CMake ZIP avoids Compress-Archive's per-file size limitation.
Push-Location -LiteralPath $destinationPath
try { Invoke-Checked cmake @('-E', 'tar', 'cf', $archive, '--format=zip', '--', $name) }
finally { Pop-Location }
Write-Host "Package: $package`nArchive: $archive`nBuild files retained: $build"
