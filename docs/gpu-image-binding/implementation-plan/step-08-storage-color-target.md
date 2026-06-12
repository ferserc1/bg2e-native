# Step 08 — Make color targets storage-capable; Metal load-action fix

## Title
Allow the frame color image to be written by a compute shader, and preserve its
contents when the triangle is rendered on top.

## Objective
Prepare the swapchain / drawable color image so a compute shader can write to it
as a `StorageImage` (layout `General`), and ensure the subsequent render pass
**loads** (does not discard) that content before drawing the triangle.

## Context
This is pure infrastructure: it changes how the color target is created and how
the render pass loads it. No example behavior changes yet (the gradient is wired
in Step 09), so existing examples still look the same — except for the Vulkan
swapchain format moving from sRGB to UNORM (see risks).

Discovered facts that drive this step:
- Vulkan swapchain is created in `vk::WindowSurface::createRenderTarget` with
  `imageUsage = COLOR_ATTACHMENT | TRANSFER_DST` and an **sRGB** color format.
  sRGB images cannot be `STORAGE_IMAGE`.
- Metal `CAMetalLayer` defaults to `framebufferOnly = true`, forbidding compute
  writes to the drawable texture.
- Vulkan render attachment already uses `LOAD_OP_LOAD` when no clear is supplied
  (`Info::attachmentInfo`). Metal's `beginRendering` uses `LoadActionDontCare`
  when no clear is requested, which would discard the gradient.

## Expected previous state
- Steps 01–07 complete: the binding API exists end to end but is unused.

## Files to review / modify
- Review: `lib/src/bg2e/gpu/vk/WindowSurface.cpp` (swapchain create),
  `lib/src/bg2e/gpu/vk/Surface.cpp` and `vk/Surface.hpp` (`_colorFormat`,
  `colorFormat()`), `lib/src/bg2e/gpu/vk/CommandBuffer.cpp` (`toVkLayout`,
  already maps `General`), `lib/src/bg2e/gpu/metal/WindowSurface.cpp`
  (`createRenderTarget`, `beginRendering` via `metal/CommandBuffer.cpp`),
  `lib/src/bg2e/gpu/metal/CommandBuffer.cpp` (`beginRendering` load action).
- Modify: `vk/WindowSurface.cpp`, `metal/WindowSurface.cpp`,
  `metal/CommandBuffer.cpp`. Possibly `vk/Surface` default color format.

## Proposed design
### Vulkan — storage-capable swapchain
1. Prefer a **non-sRGB, storage-capable** color format. Change the preferred
   surface format selection in `createRenderTarget` to favor
   `B8G8R8A8_UNORM` (or `R8G8B8A8_UNORM`) with `SRGB_NONLINEAR` color space,
   falling back to the first available if absent.
2. Add `VK_IMAGE_USAGE_STORAGE_BIT` to `createInfo.imageUsage`, **only if**
   `caps.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT` is set. If unsupported,
   leave it off and rely on the offscreen fallback (see risks) — but the common
   case (desktop GPUs / MoltenVK) supports storage on UNORM swapchains.

The swapchain `vk::Image`s already create an image view from the swapchain format;
a UNORM format view is valid as a storage image. No change needed in
`vk::Image::initFromSwapchainImage` beyond it inheriting the new usage implicitly
(the view format follows `_colorFormat`).

### Metal — writable drawable
In `metal::WindowSurface::createRenderTarget`, after configuring the layer:

```cpp
_layer->setFramebufferOnly(false);
```

With `framebufferOnly = false`, the drawable texture gains usage compatible with
`ShaderRead | ShaderWrite`, so the compute encoder can write to it via
`setTexture` + `texture.write(...)`.

### Metal — preserve color on render load
In `metal::CommandBuffer::beginRendering`, change the default color load action
from `MTL::LoadActionDontCare` to `MTL::LoadActionLoad`, so that when the example
does **not** call `clearColor`, the compute-written gradient is preserved.
`clearColor` still overrides to `LoadActionClear`, so example 04 (clear-only) is
unaffected.

## Required changes
1. `vk/WindowSurface.cpp`: prefer UNORM storage-capable format; conditionally add
   `VK_IMAGE_USAGE_STORAGE_BIT` to swapchain usage (guard with
   `supportedUsageFlags`).
2. `metal/WindowSurface.cpp`: `setFramebufferOnly(false)`.
3. `metal/CommandBuffer.cpp`: default color attachment `LoadAction` -> `Load`.

## Compilation criteria
- Project compiles. Changes are localized to surface creation and the Metal
  load-action default.

## Validation criteria
- `examples/gpu/05_simple_triangle` still renders the triangle. The clear-color
  background animation still works (the example still calls `clearColor` until
  Step 09 removes it).
- On Vulkan, validation reports no error for the new swapchain usage/format.
- On Metal, no validation error from `framebufferOnly = false`.

## Risks / things to check
- **sRGB → UNORM gamma shift:** moving the swapchain to UNORM changes perceived
  brightness/gamma of the existing example. Acceptable for a low-level example;
  call it out. (If exact color is required later, do gamma in-shader.)
- **Storage unsupported on swapchain:** if `supportedUsageFlags` lacks
  `STORAGE_BIT`, the preferred direct-write path is unavailable. **Fallback
  (documented, implement only if needed):** create an offscreen `StorageImage`
  via the Step 10 `Device::createImage` API, have compute write to it, then
  either (a) sample it as a fullscreen background quad in the graphics pass, or
  (b) blit/copy it into the color attachment (`TransferDst`) before drawing the
  triangle. Prefer (b) for minimal shader changes.
- **Layout flow:** confirm the per-frame transition order works:
  `General` (compute write) → `ColorAttachment` (render) → `Present`. The Vulkan
  `transition` already no-ops when old==new and maps `General` correctly.
- Metal `LoadActionLoad` on a drawable is valid; verify it does not conflict with
  `StoreActionStore` already set.

## What NOT to do in this step
- Do not write the gradient yet, change shaders, or create resource sets
  (Step 09).
- Do not implement the offscreen fallback unless the preferred path is proven
  unavailable on the test hardware; only document it here.
