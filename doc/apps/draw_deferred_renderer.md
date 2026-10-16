# Experimental deferred renderer example and distribution

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
backend/FSR availability. Production apps and their documented workflows remain
under `bg2e::render`.
