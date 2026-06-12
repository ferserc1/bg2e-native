# Step 06 — `vk::ResourceSet` + `metal::ResourceSet` implementations

## Title
Implement `ResourceSet` on both backends and override `Device::createResourceSet`.

## Objective
Provide a usable common resource set: a Vulkan `VkDescriptorSet` (allocated from a
pool, written via `vkUpdateDescriptorSets`) and a Metal resolved binding table
consumed later by `bindResourceSet`.

## Context
The `PipelineLayout` now exposes per-set `VkDescriptorSetLayout`s (Vulkan) and a
binding table (Metal). This step builds the sets that reference concrete images
and samplers.

## Expected previous state
- Steps 01–05 complete.

## Files to review / modify
- Review: `lib/src/bg2e/gpu/vk/Image.cpp` (image view + layout handling),
  `lib/include/bg2e/gpu/vk/Image.hpp` (`imageView()`),
  `lib/include/bg2e/gpu/metal/Image.hpp` (`texture()`),
  `lib/src/bg2e/gpu/vk/PipelineLayout.cpp` (set-layout getter),
  `lib/src/bg2e/gpu/vk/Device.cpp` / `metal/Device.cpp` (factory wiring),
  `lib/src/bg2e/gpu/vk/Sampler.cpp` / `metal/Sampler.cpp` (`handle()`).
- Add: `lib/include/bg2e/gpu/vk/ResourceSet.hpp`,
  `lib/src/bg2e/gpu/vk/ResourceSet.cpp`,
  `lib/include/bg2e/gpu/metal/ResourceSet.hpp`,
  `lib/src/bg2e/gpu/metal/ResourceSet.cpp`.
- Modify: `vk::Device` / `metal::Device` (override `createResourceSet`).

## Proposed design
### Vulkan
`vk::ResourceSet : public gpu::ResourceSet` owns:
- a small `VkDescriptorPool` (sized for the bindings of one set — a handful of
  `STORAGE_IMAGE` / `SAMPLED_IMAGE` / `SAMPLER` descriptors), and
- one `VkDescriptorSet` allocated from the layout's
  `descriptorSetLayout(setIndex)`.

Setters record pending `VkDescriptorImageInfo` + `VkWriteDescriptorSet` keyed by
binding:
- `setStorageImage`: `descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE`,
  `imageView = vkImage->imageView()`, `imageLayout = VK_IMAGE_LAYOUT_GENERAL`.
- `setSampledImage`: `VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE`,
  `imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`.
- `setSampler`: `VK_DESCRIPTOR_TYPE_SAMPLER`, `sampler = vkSampler->handle()`.

`update()` calls `vkUpdateDescriptorSets` with the accumulated writes and clears
the pending list. Expose `VkDescriptorSet handle() const` for `bindResourceSet`.
`cleanup()` destroys the pool (which frees the set).

### Metal
`metal::ResourceSet : public gpu::ResourceSet` stores a small vector of entries:

```cpp
struct Entry {
    uint32_t     index;   // Metal arg index (from PipelineLayout)
    ResourceType type;
    ShaderStage  stage;
    MTL::Texture*      texture = nullptr;   // for Sampled/Storage image
    MTL::SamplerState* sampler = nullptr;   // for Sampler
};
```

Setters look up the matching `ResourceBinding` in the layout (to obtain
`stage` and the resolved Metal index), store the native handle, and append/replace
the entry. `update()` is a no-op (or just marks ready) because Metal binding
happens at encode time in `bindResourceSet`. Expose
`const std::vector<Entry>& entries() const`.
Guard with `#if BG2E_IS_MAC` / throwing stubs.

### Device overrides
- `vk::Device::createResourceSet(layout, setIndex)` ->
  `std::make_unique<vk::ResourceSet>(_device, vkLayout, setIndex)`.
- `metal::Device::createResourceSet(layout, setIndex)` ->
  `std::make_unique<metal::ResourceSet>(metalLayout, setIndex)`.

Both `dynamic_cast` the incoming `PipelineLayout*` to the backend type.

## Required changes
1. Add the four files; implement setters / `update()` / `cleanup()` / `handle()`
   (vk) / `entries()` (metal).
2. Override `createResourceSet` in both backend devices.

## Compilation criteria
- Project compiles. Metal source fully guarded by `#if BG2E_IS_MAC`. No CMake
  edits (auto-glob).

## Validation criteria
- Deferred to Step 09 (compute storage-image set) and Step 12 (graphics
  sampled-image + sampler set), where the sets are actually bound and drawn.
- Existing examples behave identically.

## Risks / things to check
- **Per-frame update hazard (Vulkan):** a single `VkDescriptorSet` updated every
  frame can race with a previous in-flight frame still reading it. For the
  storage-image set (which references the rotating swapchain color image), the
  example (Step 09) must use **one `ResourceSet` per frame-in-flight** (a ring of
  `surface->imageCount()` sets) and update only the slot whose fence has already
  been waited on in `beginFrame`. Document this in the `ResourceSet` header and in
  Step 09. The static sampled-texture set (Step 12) is updated once and is safe.
- Descriptor pool sizing must cover the worst-case binding counts of the set.
- `imageLayout` in the write must match the actual layout at draw/dispatch time
  (`GENERAL` for storage, `SHADER_READ_ONLY_OPTIMAL` for sampled).
- Metal: `texture()` on a drawable-backed `metal::Image` is only valid for the
  current frame; the storage-image set must be (re)assigned each frame, same as
  Vulkan.

## What NOT to do in this step
- Do not implement buffer setters (`UniformBuffer` / `StorageBuffer`).
- Do not add `bindResourceSet` yet (Step 07).
- Do not introduce a global/shared descriptor pool abstraction; a per-set pool is
  sufficient for this iteration.
