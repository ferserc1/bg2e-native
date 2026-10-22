# Build, shaders and distribution

Run repository scripts from the repository root. C++20 and VULKAN_SDK are required;
VULKAN_SDK defaults from the environment. Set it explicitly in IDE CMake caches
when the IDE does not inherit that environment.

| Platform | Generator and toolchain |
|---|---|
| Linux | Ninja, native compiler and required dialog/window dependencies. |
| macOS | Xcode for working native file dialogs; Ninja is usable for development but dialogs can break. Metal compiler tools are required for Metal libraries. |
| Windows | Visual Studio 17 2022, MSVC runtime and Vulkan SDK. |

Configure using the platform generator and `-DVULKAN_SDK=/path/to/sdk`, then build
that generated tree. Runtime outputs use bin/linux or bin/macos; Windows uses
configuration-specific output. Preserve shader/asset relative paths and macOS
bundled Vulkan libraries, ICD and layer resources. Code signing is disabled by
the project, so a build is not a signed distribution.

## Paired native shaders

Engine shader sources are `shaders/src/draw` and `shaders/src/bake`, with explicit
Vulkan GLSL and Metal stages. CMake aggregates both trees, preserves unique Metal
intermediates and refreshes bundle resources on shader-only changes. Runtime
shader roots contain draw/ and bake/ with matching paths. Retired flat GLSL
frontends are no longer part of the workflow. Reused output directories may still
contain obsolete artifacts; use fresh packaging output to establish absence.
Optional FFX and SMAA resources retain their own availability rules.
See [ShaderLibraries](../api/gpu/ShaderLibraries.md).

## Distribution and external applications

[Packaging scripts](../../scripts/PACKAGING.md) build fresh distribution trees
and register the three canonical apps: model_edit, bg2e_composer and
lightmap_generator. They require a valid SDK, an external destination and a
clean checkout by default; use only flags supported by the selected script.

Use scripts/create_standalone_project.sh TARGET [NAME] or the PowerShell sibling
for an external precompiled-library project. The template uses the draw deferred
renderer. Keep copied headers, platform library, assets and paired shader/helper
resources together. scripts/update_libraries.sh PROJECT Debug|Release (or its
PowerShell sibling) is intended to refresh the engine library/headers/shaders/helper.
After shader preflight, its copy phase replaces
engine draw/bake shader directories while preserving custom application resources
outside them; refresh is not transactional. Platform library lookup follows current
configuration-specific paths and macOS fallbacks.

Build, install, package, launch and standalone creator execution are separate
validation activities. Source inspection of registrations or scripts does not
establish a native execution result.

## Standalone shader preflight

Creators and updaters check the canonical paired shader paths
`draw/raster/gbuffer.frag.spv` and `bake/uv_surface/surface.frag.spv` before copying.
The shell scripts also require the corresponding `.metallib` files on macOS.
Build current sources first; these checks establish file presence, not binary/header
ABI compatibility. See [packaging details](../../scripts/PACKAGING.md#standalone-shader-preflight).

## Package scope by platform

Linux emits .tar.gz and alone offers --allow-dirty/--overwrite; macOS and Windows
emit .zip and require a clean checkout with unused output paths. Linux retains
only the engine among installed top-level shared libraries and relies on target
system dependencies. Windows resolves non-system DLL imports/compiler runtimes;
macOS retains bundle runtime rules. Application packaging disables examples/tests,
so draw_offscreen_application is not included by those default scripts. None of
these source rules establishes successful installation, relocation or execution.
