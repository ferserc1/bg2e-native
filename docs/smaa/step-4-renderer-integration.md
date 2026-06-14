# Step 4: Renderer Integration — `RendererDeferred` Modifications

## Files

- `lib/include/bg2e/render/RendererDeferred.hpp` (modify)
- `lib/src/bg2e/render/RendererDeferred.cpp` (modify)

## Purpose

Integrate `SMAAProcessor` into the deferred renderer so that SMAA runs automatically after the scene is fully composed, producing an anti-aliased image for the UI overlay and present.

## Current Draw Flow

In `RendererDeferred::draw()` (lines 228–316 of `RendererDeferred.cpp`):

```
1. prepareSceneRender()              // Scene setup
2. SkyboxLayer → _skyboxImage        // Skybox
3. OpaqueLayer → _opaqueImage        // Opaque geometry + RTAO + reflections
4. TransparentLayer → colorImage     // Transparent geometry (writes to swapchain)
5. GizmoAndSelectionRenderer         // Gizmos (non-offscreen only)
6. endSceneRender()                  // Cleanup callbacks
7. Transition colorImage → COLOR_ATTACHMENT_OPTIMAL
8. Return
```

After SMAA integration:

```
1. prepareSceneRender()
2. SkyboxLayer → _skyboxImage
3. OpaqueLayer → _opaqueImage
4. TransparentLayer → colorImage
5. GizmoAndSelectionRenderer → colorImage
6. SMAAProcessor::process(colorImage) → colorImage   ← NEW
7. endSceneRender()
8. Transition colorImage → COLOR_ATTACHMENT_OPTIMAL
9. Return
```

The UI overlay is rendered by `RenderLoop` after `draw()` returns, so it is unaffected by SMAA.

## Header Modifications

### Add Include

```cpp
#include <bg2e/render/deferred/SMAAProcessor.hpp>
```

### Add Member Variable

In the `protected` section, after the existing members:

```cpp
std::unique_ptr<deferred::SMAAProcessor> _smaaProcessor;
```

### Add Public Accessors (optional)

```cpp
deferred::SMAAProcessor* smaaProcessor() const { return _smaaProcessor.get(); }
```

## Implementation Modifications

### `RendererDeferred::build()`

After the existing code that creates `_gizmoAndSelectionRenderer` (line ~125), add:

```cpp
// SMAA post-processing
_smaaProcessor = std::make_unique<deferred::SMAAProcessor>(_engine);
_smaaProcessor->build(initialExtent, colorImageFormat);
```

**Placement**: After all layers are created, before `build()` returns.

### `RendererDeferred::resize()`

After the existing code that recreates `_opaqueImage` (line ~215), add:

```cpp
if (_smaaProcessor) {
    _smaaProcessor->resize(newExtent);
}
```

**Placement**: After all layer resizes and image recreations, before `_resizeVisitor.resizeViewport()`.

### `RendererDeferred::draw()`

After the gizmo rendering block (line ~300) and before `endSceneRender()` (line ~304), insert the SMAA processing:

```cpp
    // === SMAA Anti-Aliasing ===
    if (_smaaProcessor) {
        // Transition colorImage to SHADER_READ_ONLY for SMAA input
        vulkan::Image::cmdTransitionImage(
            cmd,
            colorImage->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        // Run SMAA (reads from colorImage, writes to internal outputImage)
        const vulkan::Image* smaaOutput = _smaaProcessor->process(
            cmd, currentFrame,
            colorImage
        );

        // Copy SMAA output back to colorImage
        // cmdCopy handles the transitions: src stays SHADER_READ_ONLY,
        // dst transitions from SHADER_READ_ONLY to COLOR_ATTACHMENT_OPTIMAL
        vulkan::Image::cmdCopy(
            cmd,
            smaaOutput->handle(),
            smaaOutput->extent2D(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            colorImage->handle(),
            colorImage->extent2D(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // src final layout
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL     // dst final layout (for UI)
        );
    }

    // === End scene render (from Renderer base) ===
    endSceneRender();
```

**Important**: After SMAA, `colorImage` must be in `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL` so the UI overlay can render to it. The `cmdCopy` call handles both the copy and the final layout transition.

### Alternative: Blit Instead of Copy

If `smaaOutput` and `colorImage` have different formats (e.g., `R8G8B8A8_UNORM` vs `B8G8R8A8_UNORM`), `vkCmdCopyImage` requires matching formats. In this case, use `vkCmdBlitImage` instead, or ensure `smaaOutput` is created with the same format as `colorImage` by passing `_colorImageFormat` to `SMAAProcessor::build()`. For same-size, same-format images, `cmdCopy` is correct and efficient.

### Layout Transition Summary

| Step | colorImage Layout | smaaOutput Layout |
|------|-------------------|-------------------|
| Before SMAA | `UNDEFINED` (after gizmo rendering) | — |
| SMAA input transition | `SHADER_READ_ONLY_OPTIMAL` | — |
| After pass 1 | `SHADER_READ_ONLY_OPTIMAL` | `GENERAL` → `SHADER_READ_ONLY` |
| After pass 2 | `SHADER_READ_ONLY_OPTIMAL` | `GENERAL` → `SHADER_READ_ONLY` |
| After pass 3 | `SHADER_READ_ONLY_OPTIMAL` | `SHADER_READ_ONLY` |
| cmdCopy | `SHADER_READ_ONLY` → `COLOR_ATTACHMENT_OPTIMAL` | `SHADER_READ_ONLY` (unchanged) |

### `RendererDeferred::cleanup()`

Before `_gizmoAndSelectionRenderer.reset()` (line ~319), add:

```cpp
if (_smaaProcessor) {
    _smaaProcessor->cleanup();
    _smaaProcessor.reset();
}
```

**Placement**: SMAA cleanup should happen before layer cleanup since it doesn't depend on them.

## Constructor Note

`RendererDeferred` has a default constructor. The `_smaaProcessor` is `nullptr` until `build()` is called, so all SMAA-related code is guarded by null checks.

## Offscreen Considerations

SMAA works for both on-screen and offscreen rendering. The `_smaaProcessor` is created unconditionally in `build()`. If offscreen rendering doesn't need SMAA, it can be skipped by checking `_isOffscreen`, but this is not required — SMAA on offscreen output is valid and useful.

## No Changes to Other Systems

- **G-buffer layout**: Unchanged
- **RTAO**: Unchanged
- **RT reflections**: Unchanged
- **Transparent layer**: Unchanged
- **Skybox layer**: Unchanged
- **Gizmo renderer**: Unchanged (renders before SMAA)
- **RenderLoop**: Unchanged (UI renders after SMAA)
- **DefaultRenderLoopDelegate**: Unchanged
