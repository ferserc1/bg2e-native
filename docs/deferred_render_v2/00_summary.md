# Deferred Renderer Implementation Plan v2

## General Overview

Implement a deferred renderer within the bg2e-native engine using a **layer-based architecture**. The renderer processes the scene through an ordered chain of deferred layers, where each layer generates its own G-buffers, computes lighting, and produces an intermediate image that feeds into the next layer. The final layer produces the rendered output.

**Key design principles:**
- **No MSAA** — the deferred renderer works with non-MSAA images (color + depth) provided by the swapchain
- **Layer-based pipeline** — each layer implements a complete deferred pipeline (G-buffer generation + compositing)
- **Incremental development** — each phase builds on the previous one without breaking the existing forward renderer
- **Parallel coexistence** — `RendererDeferred` coexists with `RendererBasicForward` via the same template delegate system

---

## Important: Build Policy

**DO NOT compile or build the project at any phase.** The user will personally review and test each implementation step. All verification is done by the user through manual code review. The agent's responsibility is limited to writing correct code following existing patterns and conventions.

---

## Architecture

### Layer Chain

```
┌─────────────────┐
│ Background Layer │ ← Renders skybox only (no G-buffers needed)
│  (SkyboxLayer)   │
└────────┬────────┘
         │ skyboxImage
         ▼
┌─────────────────┐
│  Opaque Layer    │ ← G-buffers: albedo, normal, material, positions
│ (DeferredLayer)  │ ← Composites G-buffers with lighting + shadows
└────────┬────────┘
         │ opaqueImage (skybox + opaque objects combined)
         ▼
┌─────────────────┐
│ Transparent Layer│ ← G-buffers: albedo, normal, material, positions
│ (DeferredLayer)  │ ← Composites over opaque layer image
└────────┬────────┘
         │ finalImage
         ▼
    Swapchain Output
```

Each layer receives the output image of the previous layer as input. The first layer (background) generates the skybox image. Each subsequent `DeferredLayer` renders its objects into G-buffers, then composites them with PBR lighting onto the input image. The last layer produces the final rendered frame.

### G-Buffer Layout

| Attachment | Format | Content |
|------------|--------|---------|
| Color 0 (albedo) | `VK_FORMAT_R8G8B8A8_UNORM` | Base color (linear, converted from sRGB in shader) |
| Color 1 (normals) | `VK_FORMAT_R8G8B8A8_SNORM` | World-space normals (XYZ mapped to [-1,1]) |
| Color 2 (materials) | `VK_FORMAT_R8G8B8A8_UNORM` | Metalness(R), Roughness(G), AO(B), Sheen(A) |
| Color 3 (positions) | `VK_FORMAT_R32G32B32A32_SFLOAT` | World-space position (X, Y, Z, 1.0) |
| Depth | `VK_FORMAT_D32_SFLOAT` | Depth buffer for depth testing |

### Shadow Processing

Shadows are computed during the compositing step of each deferred layer using **ray tracing queries** (`rayQueryEXT`), leveraging the existing RT infrastructure in the engine. When RT hardware is not available, all lights contribute without shadow testing (same fallback as the forward renderer).

---

## Plan Phases

| Phase | Title | Brief Description | Detail File |
|-------|-------|-------------------|-------------|
| 1 | RendererDeferred — Shell + Example | Empty `RendererDeferred` class with stub methods. New example project to verify template wiring. | [01_phase1_renderer_shell.md](./01_phase1_renderer_shell.md) |
| 2 | Skybox Layer | Render skybox to an intermediate image, copy to swapchain output. First functional layer. | [02_phase2_skybox_layer.md](./02_phase2_skybox_layer.md) |
| 3 | GBuffer Manager | `GBufferManager` class that creates and manages N G-buffer images from a scene. | [03_phase3_gbuffer_manager.md](./03_phase3_gbuffer_manager.md) |
| 4 | Deferred Layer | `DeferredLayer` class: G-buffer generation + PBR compositing pass. G-buffer shader + compositing shader. | [04_phase4_deferred_layer.md](./04_phase4_deferred_layer.md) |
| 5 | Layer Composition | Combine all layers in `RendererDeferred`. Chain: skybox → opaque → transparent → output. | [05_phase5_layer_composition.md](./05_phase5_layer_composition.md) |
| 6 | Integration | Template expansion for `DefaultRenderLoopDelegate` and `DefaultOffscreenApplicationDelegate`. | [06_phase6_integration.md](./06_phase6_integration.md) |

---

## New Files to Create

### C++ Headers and Sources
| File | Phase | Purpose |
|------|-------|---------|
| `lib/include/bg2e/render/RendererDeferred.hpp` | 1 | Deferred renderer class declaration |
| `lib/src/bg2e/render/RendererDeferred.cpp` | 1 | Deferred renderer implementation |
| `lib/include/bg2e/render/gbuffer/GBufferManager.hpp` | 3 | G-buffer image management |
| `lib/src/bg2e/render/gbuffer/GBufferManager.cpp` | 3 | G-buffer implementation |
| `lib/include/bg2e/render/deferred/DeferredLayer.hpp` | 4 | Deferred layer class |
| `lib/src/bg2e/render/deferred/DeferredLayer.cpp` | 4 | Deferred layer implementation |
| `lib/include/bg2e/render/deferred/SkyboxLayer.hpp` | 2 | Skybox-only layer |
| `lib/src/bg2e/render/deferred/SkyboxLayer.cpp` | 2 | Skybox layer implementation |

### Shaders
| File | Phase | Purpose |
|------|-------|---------|
| `shaders/src/deferred_gbuffer.vert.glsl` | 4 | G-buffer vertex shader (reuses forward vertex logic) |
| `shaders/src/deferred_gbuffer.frag.glsl` | 4 | G-buffer fragment shader (writes albedo, normal, material, position) |
| `shaders/src/deferred_composite.vert.glsl` | 4 | Compositing fullscreen quad vertex shader |
| `shaders/src/deferred_composite.frag.glsl` | 4 | Compositing fragment shader (PBR lighting from G-buffers) |
| `shaders/src/deferred_composite_rt.frag.glsl` | 4 | Compositing fragment shader with RT shadows |

### Modified Files
| File | Phase | Purpose |
|------|-------|---------|
| `lib/src/bg2e/render/DefaultRenderLoopDelegate.cpp` | 6 | Add explicit template instantiation for `RendererDeferred` |
| `lib/src/bg2e/render/DefaultOffscreenApplicationDelegate.cpp` | 6 | Add explicit template instantiation for `RendererDeferred` |
| `lib/include/bg2e/render/all.hpp` | 6 | Add includes for new headers |

### Example Project
| File | Phase | Purpose |
|------|-------|---------|
| `examples/XX_deferred_renderer/src/main.cpp` | 1 | Minimal example using `RendererDeferred` |

---

## Key Technical Decisions

### No MSAA
The deferred renderer does not use MSAA. The `build()` method receives `sampleCount` but all internal G-buffer images are created with `VK_SAMPLE_COUNT_1_BIT`. The output is written directly to the non-MSAA swapchain image.

### Layer Abstraction
The layer system is designed for extensibility. While the initial implementation uses only background + opaque + transparent layers, the architecture supports an arbitrary number of layers by chaining them in sequence. Each layer type implements a common interface.

### G-Buffer Position Storage
World-space positions are stored in a dedicated `R32G32B32A32_SFLOAT` attachment. This avoids complex depth reconstruction in the compositing shader and simplifies PBR lighting calculations. The tradeoff is higher memory usage (128 bits per pixel for positions).

### Reuse of Existing Infrastructure
- **Data bindings**: Reuses `FrameDataBinding`, `ObjectDataBinding`, `EnvironmentDataBinding`, `LightDataBinding`, and `RayTracingSceneDataBinding` from the scene/vk module
- **Render queue**: Uses the existing `RenderQueue` system with `RenderQueueVisitor` to filter objects by type (opaque/transparent)
- **Material system**: Uses `MaterialAttributes::isTransparent()` and `isSolid()` to route objects to the correct layer
- **Shader libraries**: Reuses `lib/pbr.glsl`, `lib/uniforms.glsl`, `lib/normal_map.glsl`, `lib/color_correction.glsl`
- **Environment resources**: Reuses `EnvironmentResources` for IBL (irradiance, specular, BRDF LUT)
