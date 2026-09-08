# Application packages

Windows (run from a Visual Studio 2022 developer PowerShell with CMake on PATH):

```powershell
.\scripts\package_windows.ps1 -Configuration Debug -Destination C:\packages
.\scripts\package_windows.ps1 -Configuration Release -Destination C:\packages\release -VulkanSdk C:\VulkanSDK\1.4.xxx.0
```

macOS and Linux:

```sh
bash scripts/package_macos.sh Debug /existing/package/directory "$VULKAN_SDK"
bash scripts/package_linux.sh Release /existing/package/directory "$VULKAN_SDK"
```

Configuration and destination are mandatory. No configuration defaults to Release.
All scripts reject staged, unstaged and untracked changes, dirty submodules,
and uninitialized or mismatched submodules before creating files or building.
Commit the packaging changes before using the scripts. Ignored files do not count
as pending Git changes. Build inputs should be tracked to make the commit useful
as an identifier.

Each invocation creates a fresh temporary build tree and retains it for diagnostics.
Its runtime output uses bin/windows/Debug or bin/windows/Release inside that tree
(on other platforms, bin/linux or bin/macos). Existing project bin/ output is never
used, so deleted applications and stale shaders cannot leak into a package.
The destination must be outside the repository. Existing packages are not overwritten.

The folder and ZIP are named bg2engine_<12-character-or-longer-local-HEAD-hash>.
This identifies the commit actually built, without fetching or switching to GitHub's
latest remote commit. build-info.json records its full hash and configuration.
Use separate destinations for Debug and Release of the same commit.
The ZIP contains the named folder and all its contents.

CMake's bg2e_distribution target builds bg2e and executable targets recursively
registered under apps/, including their dependencies. Register new application
folders in apps/CMakeLists.txt normally; no packaging script target list needs editing.
BG2E_BUILD_EXAMPLES and BG2E_BUILD_TESTS default to ON for normal builds and are
turned OFF by the scripts. Distribution install rules are opt-in via BG2E_PACKAGE_APPS.

Windows packages contain application executables, the engine DLL, recursively
resolved non-system DLL imports, the MSVC redistributable runtime, built assets
and shaders, and target PDBs for Debug. Import/static libraries and intermediate
build files are excluded. Missing runtime dependencies fail installation.
The engine currently uses /MD in both configurations; Debug therefore packages
symbols and the release CRT. This does not change the project's runtime selection.

InstallRequiredSystemLibraries supplies compiler runtimes; the CMake runtime
dependency scanner resolves other imported DLLs using the output and Vulkan SDK
Bin directories. Dynamically loaded plugins/layers are not discoverable through
binary imports and need explicit install rules if an application requires them.
Target machines still need a compatible graphics driver/Vulkan implementation.

macOS uses Xcode and existing app bundle resource/library rules; Linux uses Ninja,
installs shared dependencies with an executable-relative RPATH, and leaves the
platform C runtime to the host. These scripts require validation on their native
platforms; they are not a claim of universal Linux portability or signed macOS distribution.

References:
- https://cmake.org/cmake/help/latest/module/InstallRequiredSystemLibraries.html
- https://cmake.org/cmake/help/latest/command/install.html#installing-runtime-dependencies
