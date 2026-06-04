# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Important constraints

- **Do NOT modify CMake files** unless explicitly requested.
- **Do NOT attempt to compile** unless explicitly requested.
- Always generate responses in **English**, regardless of the language the user writes in.
- Avoid loops that repeat the same response; if stuck, ask the user.

## Build system

**Generator is platform-locked:**

| Platform | Required generator |
|---|---|
| macOS | `Xcode` (native file dialogs require it) |
| Windows | `Visual Studio 17 2022` |
| Linux | `Ninja` |

CLion on macOS should use Ninja for IDE features, but must switch to Xcode to test native dialogs.

```sh
# Linux
cmake -S . -B build -G Ninja
cmake --build build

# macOS
cmake -S . -B build -G Xcode
cmake --build build

# Windows
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build
```

`VULKAN_SDK` is the only required variable. It defaults to `$VULKAN_SDK` env var. CLion does not inherit it automatically — set it in CMake cache variables under `Settings > Build > CMake`.

**Output:** All binaries land in `bin/{linux|macos|windows}/`. Compiled shaders go to `bin/{platform}/shaders/`. Assets are copied to `bin/{platform}/assets/`.

**Auto-glob:** CMake auto-includes all `.hpp`/`.cpp`/`.h`/`.c` files under `lib/`, `examples/`, `apps/`, and all `.glsl` under `shaders/`. Adding a new file in those paths is enough; no CMake edits needed.

## Architecture overview

The engine is a C++20 shared library (`libbg2e`) built in layered modules:

| Layer | Module | Purpose |
|---|---|---|
| 0 | `math`, `base` | GLM-based math, Color, Camera, MaterialAttributes, Texture (CPU side) |
| 1 | `json`, `geo` | JSON serialization, procedural geometry (Mesh + primitives) |
| 2 | `render` | Vulkan Engine, Renderer, RenderLoop, GPU Textures |
| 3 | `scene` | Node/Component scene graph, Visitor pattern |
| 4 | `db` | Asset loading: glTF, OBJ, `.bg2` (proprietary binary) |
| 5 | `manipulation` | Ray picking, selection highlight |
| 6 | `utils` | TextureCache, MaterialSerializer |
| 7 | `app` | Application, MainLoop, InputManager |
| 8 | `ui` | Dear ImGui wrapper, windows, widgets |

Headers are in `lib/include/bg2e/`, implementations in `lib/src/bg2e/`.

### Render pipeline

Uses **Vulkan dynamic rendering** — no `VkRenderPass`; requires `VK_KHR_dynamic_rendering`.

Descriptor sets (4 sets):
- **set=0**: SceneData (view + projection)
- **set=1**: PBRObjectData (model matrix + material + 5 texture samplers: albedo, normal, metalness, roughness, AO)
- **set=2**: EnvironmentData (irradiance cubemap, prefiltered envmap, BRDF LUT, maxReflectionLOD)
- **set=3**: LightData (up to 8 lights)
- **Push constants**: gamma, brightness, contrast, exposure

Forward PBR call chain:
```
RenderLoop::run() → RenderLoopDelegate::render(cmd)
  → RendererBasicForward::draw(cmd, frameIndex, colorImage)
    → DrawVisitor traverses scene graph
      → DrawableComponent::draw() → ObjectDataBinding::newDescriptorSet()
      → renderMesh->drawSubmesh()
  → UserInterface::draw(cmd, imageView)  [ImGui]
→ vkQueueSubmit2 → queuePresent
```

### New `gpu` abstraction layer (in progress)

A backend-agnostic GPU abstraction is being developed under `lib/include/bg2e/gpu/`. It provides pure abstract interfaces (`gpu::Backend`, `gpu::Device`, `gpu::Instance`, `gpu::PhysicalDevice`, `gpu::Surface`, `gpu::Queue`) with concrete implementations in `gpu/vk/` (Vulkan) and `gpu/metal/` (Metal). New low-level GPU examples live under `examples/gpu/`. This layer is separate from the existing `render/` layer.

### Scene graph

- `Node` + `Component` tree; visitors: `UpdateVisitor`, `DrawVisitor`, `InputVisitor`
- `Drawable` owns a Vulkan mesh (`render::vulkan::geo::MeshPNUUT`) and material attributes
- `DrawFunction` callback (passed by `RendererBasicForward`) provides the descriptor sets per draw call

### Shader system

GLSL sources in `shaders/src/` compile to SPIR-V at build time via `${VULKAN_SDK}/bin/glslang`. Shared PBR functions are in `shaders/src/lib/` (pbr.glsl, uniforms.glsl, normal_map.glsl, color_correction.glsl, constants.glsl). Do not move `shaders/src/lib/` relative to other shader files — glslang resolves `#include` paths relative to that location.

Ray tracing shaders (`.rgen`, `.rmiss`, `.rchit`, `.rahit`, `.rint`) are compiled with `--target-env vulkan1.2`.

### macOS specifics

- Code signing is explicitly disabled in CMake.
- The app bundle includes MoltenVK, Vulkan validation layers, and `icd.d`/`explicit_layer.d` config directories (copied by `copy_vulkan_resources()` in `cmake/utils.cmake`).
- RPATH set to `@executable_path/../lib`.

### Third-party dependencies (vendored in `lib/third_party/`)

bg2-io, bg2-scene, imgui, nativefiledialog, stb_image, tinyobj, cgltf (git submodules).

External: GLM, Vulkan SDK + VMA, SDL2, GTK3/Wayland (Linux only).

## No test framework

Examples (`examples/`) are the primary verification surface. There is no automated test suite.
