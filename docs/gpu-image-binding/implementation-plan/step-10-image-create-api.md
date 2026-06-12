# Step 10 — `Device::createImage` + `ImageDescription` for sampled textures

## Title
Add a common API to create a standalone (non-surface) GPU image usable as a
sampled texture.

## Objective
Allow the example (and future code) to create an owned `gpu::Image` with a chosen
size, format, and usage — specifically a sampled 2D texture — without going
through a surface. This is the container the procedural data is uploaded into in
Step 11.

## Context
Today, `gpu::Image` instances are only created internally by surfaces
(`buildTargetImage` / `buildDepthImage` / swapchain / drawable wrappers). There is
no public factory on `Device`. Both backends already have private build methods we
can reuse / extend.

## Expected previous state
- Steps 01–09 complete (gradient milestone reached). `ImageUsage` flags exist
  (Step 01).

## Files to review / modify
- Review: `lib/include/bg2e/gpu/Image.hpp`,
  `lib/src/bg2e/gpu/vk/Image.cpp` (`buildTargetImage`),
  `lib/src/bg2e/gpu/metal/Image.cpp` (`buildTargetImage`),
  `lib/include/bg2e/gpu/Device.hpp`.
- Modify: `Device.hpp` (factory + `ImageDescription`),
  `vk/Device.cpp` / `metal/Device.cpp` (override),
  possibly `vk/Image.hpp/.cpp` and `metal/Image.hpp/.cpp` (a `buildSampledImage`
  helper).

## Proposed design
Add `ImageDescription` (in `Common.hpp` or `Image.hpp`):

```cpp
struct ImageDescription {
    Size2D      size;
    PixelFormat format = PixelFormat::R8G8B8A8_UNORM;
    ImageUsage  usage  = ImageUsage::Sampled | ImageUsage::TransferDst;
};
```

Add to `Device`:

```cpp
virtual std::unique_ptr<Image> createImage(const ImageDescription& description)
{ throw std::runtime_error("createImage not implemented"); }
```

### Vulkan
Add `vk::Image::buildSampledImage(device, size, format, usage)` (or generalize
`buildTargetImage` to accept usage flags). For a sampled texture, usage maps to
`VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT`
(translate from `ImageUsage`). Allocate device-local via VMA (same as
`buildTargetImage`), create the view, set `currentLayout = Undefined`. Override
`vk::Device::createImage` to build and return it.

### Metal
Add `metal::Image::buildSampledImage(device, size, format, usage)`: an
`MTL::TextureDescriptor` with `TextureType2D`, the mapped pixel format,
`StorageModePrivate`, and `usage = MTL::TextureUsageShaderRead` (add
`ShaderWrite` only if `ImageUsage::Storage` is requested). Override
`metal::Device::createImage`.

`ImageUsage -> VkImageUsageFlags` / `MTL::TextureUsage` is a small mapping helper
in each backend.

## Required changes
1. Define `ImageDescription` and add `Device::createImage` default-throwing
   virtual.
2. Add `buildSampledImage` to both backend `Image` classes (or extend the
   existing build method signature) and the mapping from `ImageUsage`.
3. Override `createImage` in both devices.

## Compilation criteria
- Project compiles. Additive API; existing surface-created images are untouched
  (their internal build methods keep their current fixed usage).

## Validation criteria
- `device->createImage({ {2,2}, R8G8B8A8_UNORM, Sampled|TransferDst })` returns a
  valid `Image` on both backends (exercised in Step 12).
- Existing examples behave identically.

## Risks / things to check
- Keep `buildTargetImage` / swapchain / drawable creation unchanged; only add a
  new path so the gradient milestone is not disturbed.
- `R8G8B8A8_UNORM` must be in the `PixelFormat -> native` maps already (it is, per
  `Common.hpp` and `Format.cpp`).
- Ownership: returned `Image` is fully owned by the caller (`unique_ptr`), unlike
  swapchain-wrapped images.

## What NOT to do in this step
- Do not implement data upload yet (Step 11).
- Do not add depth/cube/mip/array support; 2D single-mip only.
- Do not route surface images through the new factory.
