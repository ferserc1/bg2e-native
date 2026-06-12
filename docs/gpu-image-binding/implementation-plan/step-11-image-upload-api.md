# Step 11 — `Image::uploadRGBA8` with internal native staging buffer

## Title
Add an `Image` data-upload API that manages its own native staging buffer
internally.

## Objective
Let a created sampled image receive CPU pixel data
(`std::array<uint8_t,4>`-style RGBA8 bytes) via an internal, backend-native
staging buffer — **no intermediate `Buffer` class** — leaving the image in a
shader-readable state.

## Context
This mirrors `readPixelsRGBA8` in reverse. Both backends already use
`Device::immediateSubmit` plus a native staging resource (Vulkan `VkBuffer` /
Metal `MTL::Buffer`) for the read path. The upload path reuses that exact
machinery.

## Expected previous state
- Step 10 complete: `Device::createImage` produces a sampled image (usage includes
  `TransferDst` on Vulkan).

## Files to review / modify
- Review: `lib/src/bg2e/gpu/vk/Image.cpp` (`readPixelsRGBA8`: staging buffer +
  `immediateSubmit` + transitions), `lib/src/bg2e/gpu/metal/Image.cpp`
  (`readPixelsRGBA8`: `MTL::Buffer` + blit), `lib/include/bg2e/gpu/Image.hpp`.
- Modify: `Image.hpp` (base virtual), `vk/Image.hpp/.cpp`, `metal/Image.hpp/.cpp`.

## Proposed design
Base class virtual (default-throwing, like `readPixelsRGBA8`):

```cpp
virtual void uploadRGBA8(const void* pixels, const Size2D& size)
{ throw std::runtime_error("Image::uploadRGBA8 not implemented"); }
```

(For the example, `size` equals the image size; assert they match.)

### Vulkan
1. Create a host-visible mapped staging `VkBuffer`
   (`VK_BUFFER_USAGE_TRANSFER_SRC_BIT`, `VMA_MEMORY_USAGE_CPU_TO_GPU`,
   `VMA_ALLOCATION_CREATE_MAPPED_BIT`) sized `w*h*4`.
2. `memcpy` the pixels into the mapped pointer.
3. `immediateSubmit`:
   - `transition(this, TransferDst)`,
   - `vkCmdCopyBufferToImage` (region with `VK_IMAGE_ASPECT_COLOR_BIT`, full
     extent),
   - `transition(this, ShaderReadOnly)`.
4. Destroy the staging buffer (`vmaDestroyBuffer`). The image's
   `currentLayout` ends at `ShaderReadOnly`, matching the descriptor write in
   `vk::ResourceSet::setSampledImage`.

### Metal
1. Create an `MTL::Buffer` (`StorageModeShared`) sized `w*h*4` (RGBA8 has no row
   alignment requirement for a 2-px width, but compute padded rows like the read
   path if needed; for 2×2 no padding is necessary).
2. `memcpy` pixels into `buffer->contents()`.
3. `immediateSubmit` with a `BlitCommandEncoder`:
   `copyFromBuffer(... sourceBytesPerRow = w*4 ...) toTexture(...)`.
4. Release the staging buffer. Metal manages read-state automatically; no explicit
   layout transition needed (`transition` is bookkeeping only on Metal).

Both implementations stay fully self-contained inside `Image` (the staging
resource is created, used, and destroyed within `uploadRGBA8`).

## Required changes
1. Add the base virtual.
2. Implement `uploadRGBA8` in both backends (Metal guarded by `#if BG2E_IS_MAC`,
   with a throwing `#else` stub).

## Compilation criteria
- Project compiles on all platforms. Additive method; nothing else changes.

## Validation criteria
- Deferred to Step 12, where a 2×2 image is uploaded and sampled. (Optionally, a
  round-trip `uploadRGBA8` then `readPixelsRGBA8` could confirm the data, but the
  example's visual result is the primary check.)

## Risks / things to check
- **Vulkan layout pre-state:** a freshly created image is `Undefined`; the first
  `transition(TransferDst)` is valid from `Undefined`. After upload it must be
  `ShaderReadOnly` so the sampled-image descriptor's `imageLayout` matches.
- **Row pitch:** for non-power-of-two or wide textures, Metal blit
  `sourceBytesPerRow` must be `w*4`; the read path pads to 256 for
  texture→buffer, but buffer→texture upload uses the source buffer's own pitch
  (`w*4`) — keep them distinct.
- **Format assumption:** only `R8G8B8A8_UNORM` is supported (assert), consistent
  with `readPixelsRGBA8`.
- Staging buffer lifetime must not be freed before `immediateSubmit` completes
  (`immediateSubmit` is synchronous — it waits on a fence on Vulkan and is
  blocking on Metal — so destroying after the call is safe).

## What NOT to do in this step
- Do not introduce a public `Buffer` class or expose the staging buffer.
- Do not add mipmap generation or sRGB conversion.
- Do not wire it into the example yet (Step 12).
