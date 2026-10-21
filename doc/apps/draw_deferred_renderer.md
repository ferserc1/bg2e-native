# Draw deferred renderer example and distribution

`examples/draw/10_deferred_renderer` registers target `draw_deferred_renderer`.
See the [tutorial](../tutorials/draw/10_deferred_renderer.md) for the scene
delegate structure and its small set of learning controls. This is a windowed
example, not a migrated model editor or headless application.

Configure with the platform generator: Linux Ninja, macOS Xcode for working
native file dialogs (Ninja is usable for development with that limitation),
Windows Visual Studio 17 2022. VULKAN_SDK is the required CMake variable and may
come from the environment; CLion requires its cache setting explicitly. macOS
paired Metal shaders also require Apple's compiler toolchain. Do not infer FSR
SDK presence from successful GPU initialization.

Runtime binaries, shaders and copied assets live in `bin/<platform>`; macOS
uses the .app bundle. GLSL is compiled with SDK glslang; draw pass folders pair
GLSL/Metal source and the build deploys their compiled libraries. The engine
bundles macOS Vulkan/MoltenVK configuration as required. FSR native bridge is
optional; its absence must preserve the bilinear path. Native Metal upscaling
and DLSS are not distributed features.

Packaging and standalone/library-update commands remain defined in
[scripts/PACKAGING.md](../../scripts/PACKAGING.md). Packaging requires a clean
checkout by default, a destination outside the repository and the SDK. It builds
a fresh Distribution tree and emits unpacked content plus an archive. Standalone
creation uses precompiled libraries; update_libraries uses a dedicated build tree.
Do not copy only the executable while omitting shaders/assets/runtime libraries.
This documentation pass did not run builds or packaging, or certify packaged
backend/FSR availability. Production app workflows use the same draw API.

## Delivered advanced effect controls

Graphics settings opens beside Deferred scene at startup; the scene window's
Graphics settings button reopens it. Expand Renderer for indirect mode, Global
illumination for Enabled, and Reflections for its independent Enabled toggle.
With no explicit renderer settings, Deferred now starts in GI mode with GI and
reflections requested enabled. Its resolution policy remains disabled/1.0, and
FSR/SMAA are disabled. Inspect available/status and graph selection before
interpreting unchanged output as an active effect. Pause camera
and light animation to examine convergence. [Settings](../api/draw/Settings.md)
lists diagnostic signals, exact target IDs and explicit preference/JSON behavior.

Deploy draw shaders in rtao, rtgi, rt_reflections, temporal_accumulation, denoise,
smaa and shared draw/lib native profiles. GI/reflection need Vulkan pipeline
support or Metal query support plus indexed tables/formats; plain GPU initialization
is insufficient. Missing optional SDK/effect/native profiles retain raster/IBL/
resampling with diagnostics. Keep official iryku_smaa AreaTex/SearchTex bytes and
licenses; no downloaded runtime lookup asset is needed. Native final AA requires
its own filterable storage/lookup formats as [SMAA](../api/draw/SMAAProcessor.md) specifies.

The agent performed static documentation/source checks only. Lead continuation
accepts delivered implementation without specifying all devices/scenarios;
no measured GI/reflection performance or packaged shader availability is asserted.


## Shared engine bake shaders

Current engine shader dependencies also compile/deploy the separate paired bake
tree used by [draw_lightmaps](draw_lightmaps.md). RuntimeShaders installation and
incremental app resource refresh include both draw and bake; this does not turn
the ordinary deferred example into a bake scheduler. See
[ShaderLibraries](../api/gpu/ShaderLibraries.md#engine-paired-drawbake-deployment).
