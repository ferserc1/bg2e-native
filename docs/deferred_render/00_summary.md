# Implementation Plan — Deferred Renderer

## General Overview

Implement a deferred renderer within the bg2e-native engine, following an incremental progression from scratch to a functional pipeline with:

- **G-Buffer pass:** geometry rendering into multiple attachments (albedo, tangent-space normals, material properties).
- **Compositing pass:** per-pixel lighting calculation reading from the G-buffers, with shadow support via ray tracing (when available) or without shadows (fallback).
- **Transparent objects:** rendered in a third pass directly to the swapchain after compositing.

---

## Plan Phases

| Phase | Title | Brief Description | Detail File |
|-------|-------|-------------------|-------------|
| 0 | `RendererDeferred` — empty shell | Empty class with stubs for all virtual methods to verify the template delegate wiring works. | [01_renderer_shell.md](./01_renderer_shell.md) |
| 1 | `GpuAttachmentBuffer` — G-Buffer infrastructure | Class that manages creation, formatting, MSAA and lifecycle management of G-buffer images. | [02_gpu_attachment_buffer.md](./02_gpu_attachment_buffer.md) |
| 2 | `DeferredCompositor` — compositing pass | Class that manages the compositing pass pipeline, including GLSL shaders to read G-buffers and calculate per-pixel lighting (with or without ray tracing). | [03_deferred_compositor.md](./03_deferred_compositor.md) |
| 3 | `RendererDeferred` — full implementation | Actual implementation of `RendererDeferred` methods: draw with G-buffer pass → compositing pass → transparent passthrough. | [04_renderer_deferred_impl.md](./04_renderer_deferred_impl.md) |
| 4 | Integration — template expansion | Explicit template instantiation for `RendererDeferred` in `DefaultRenderLoopDelegate` and `DefaultOffscreenApplicationDelegate`. | [05_integration.md](./05_integration.md) |

---

## Key Technical Notes

### Design decisions made during analysis planning:

**G-Buffer Formats:**
| Attachment | Format | Content |
|------------|--------|---------|
| Color 0 | `VK_FORMAT_R8G8B8A8_UNORM` | Albedo (untinted, SRGB in compositor) |
| Color 1 | `VK_FORMAT_R8G8B8A8_SNORM` | Tangent-space normals (XYZ → [-1,1]) |
| Color 2 | `VK_FORMAT_R8G8B8A8_UNORM` | Metalness (R), Roughness (G), AO (B), Emissive (A) |

**MSAA:**
- All G-buffer attachments (color + depth) use 4x MSAA.
- Resolution to single-sample format is performed before the compositing pass via `cmdResolveImage()`.

**Shadows:**
- If hardware supports ray tracing → the compositor uses `rayQueryEXT` (same approach as `basic_forward_rt_shadows.frag.glsl`).
- If no RT → all lights contribute without shadow testing (same as `basic_forward.frag.glsl`).

**Transparent objects:**
- Rendered in a third pass directly to the swapchain after compositing, with alpha blending.
- This avoids coherence issues in G-buffers with transparent objects.

---

## Required Files (new + modified)

### New:
- `lib/include/bg2e/render/RendererDeferred.hpp` (phase 0)
- `lib/src/bg2e/render/RendererDeferred.cpp` (phase 0)
- `lib/include/bg2e/render/GpuAttachmentBuffer.hpp` (phase 1)
- `lib/src/bg2e/render/GpuAttachmentBuffer.cpp` (phase 1)
- `lib/include/bg2e/render/DeferredCompositor.hpp` (phase 2)
- `lib/src/bg2e/render/DeferredCompositor.cpp` (phase 2)
- `shaders/src/deferred_lighting.vert.glsl` (phase 2)
- `shaders/src/deferred_lighting.frag.glsl` (phase 2)

### Modified:
- `lib/src/bg2e/render/DefaultRenderLoopDelegate.cpp` (phase 4)
- `lib/src/bg2e/render/DefaultOffscreenApplicationDelegate.cpp` (phase 4)
- `lib/include/bg2e/render/all.hpp` (phase 4 — new header includes)
