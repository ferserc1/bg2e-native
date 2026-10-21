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

## Experimental draw/bake shader resources

The engine also installs the paired native `shaders/draw` and `shaders/bake`
trees through the RuntimeShaders component. Preserve their relative paths in
packages and precompiled-library projects; stage filenames are not globally
unique. The paired aggregate compiles both source trees and application resource
refresh copies both even on shader-only rebuilds. On macOS the active bundle must
contain `bake/uv_surface/surface.vert.metallib`; Vulkan uses the matching `.spv`.
After updating an older checkout with missing bake registration, reload CMake
and rebuild the application, then use its refreshed bundle. See
[shader deployment](../doc/api/gpu/ShaderLibraries.md#engine-paired-drawbake-deployment)
and [draw_lightmaps](../doc/apps/draw_lightmaps.md#build-and-resources). These
resource rules do not provide a standalone draw bake scheduler or image exporter.

The [draw offscreen CLI](../doc/apps/draw_offscreen_application.md#build-and-resources)
is registered as `draw_offscreen_application` using the standard example resource
helper. Keep native draw shaders, scene assets and platform runtime libraries
with its executable/bundle. It writes one completed camera PNG; the common resource
packaging helper does not itself implement scene-image export or standalone bake
batch output. Packaging/install/standalone coverage remains subject to the actual
selected script component and target configuration; registration alone does not
establish that every distribution variant contains the example.


## Draw standalone lightmap generator product validation

`draw_lightmap_generator` is an application target under apps/draw. Standard
bundle_app rules supply native libraries/assets and paired draw/bake shader trees;
recursive Distribution discovery includes it alongside the preserved production
lightmap_generator. RuntimeShaders installs both paired trees. No special package
script or alternate shader layout is needed for the new CLI.

Registration is source evidence only. On actual supported hosts validate fresh
and shader-only builds, active bundle resources, installed Distribution and
RuntimeShaders contents, and a relocated headless job. In particular inspect
bake/uv_surface/surface.vert.metallib on Metal and its .spv peer on Vulkan at the
**executed** shader root, not just the build source tree. Record backend/device,
exact package/component/command, exit code and exported artifacts.

Successful jobs publish lightmaps.json pointing to a complete unique output
generation. Preserve that layout and independent native model sidecars when
moving job output; these outputs are not engine runtime assets. The read-only
[independent artifact inspector](../tests/bake/product/README.md) checks PNG/native
HDR maps, metadata and model material sidecars; draw engine geometry reload and
other image decoders remain separate lead checks. See the
[M08-05 evidence matrix](../docs/render_to_draw/plan/milestone_08_lightmap_generator/product-validation.md).
No backend/package success or measured performance is implied by registration or
synthetic inspector tests. Package scripts retain their existing clean-checkout,
destination and platform-generator requirements.
