# Supported examples and standalone projects

The registered examples use the canonical draw API or the lower-level GPU API.
Legacy forward-renderer examples have been retired, including their private
shaders and flat shader compiler scripts.

- `draw/01_window_ui` through `draw/09_ray_tracing_pipeline`: portable drawing,
  UI, geometry, textures, cubemaps and native tracing demonstrations.
- `draw/10_deferred_renderer`: deferred PBR scene and reflected settings.
- `draw/11_manipulation`: selection, gizmos and material editing.
- `draw/12_lightmaps`: integrated baking and applied indirect maps.
- `draw/13_offscreen_application`: headless scene rendering and PNG output.
- `gpu/01_instance` through `gpu/13_ray_tracing_pipeline`: GPU primitives,
  presentation/offscreen resources, deferred cleanup and tracing.

See [the draw tutorials](../doc/tutorials/draw/index.md) and
[GPU tutorials](../doc/tutorials/gpu/index.md) for individual examples. Configure
with BG2E_BUILD_EXAMPLES enabled to include all registered examples. Linux uses
Ninja, macOS uses Xcode for native dialogs, and Windows uses Visual Studio 2022.
VULKAN_SDK is required even when selecting Metal at runtime.

For applications outside this repository, build current engine binaries first,
then run scripts/create_standalone_project.sh on Linux/macOS or
scripts/create_standalone_project.ps1 on Windows. The generated template uses
DefaultRenderLoopDelegate with RendererDeferred, a cube, environment and camera;
it contains no legacy renderer API. It links the shared engine library (.so,
.dylib, or .dll plus import .lib), not a static macOS library.

Generated projects include paired `shaders/draw/` and `shaders/bake/` resources,
headers, assets, native runtime files and CMake helpers. Custom ShaderLib/Metal
example shader helpers remain supported; engine paired shaders retain pipeline
subdirectories and their native entry points. Use the matching library updater
to refresh an existing standalone project's headers, library and engine shaders.
See [packaging and standalone ownership](../scripts/PACKAGING.md) for paths,
requirements and validation limits. Installed launch and backend compatibility
must be checked against the newly built binaries.
