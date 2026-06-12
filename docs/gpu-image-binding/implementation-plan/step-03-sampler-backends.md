# Step 03 — `vk::Sampler` + `metal::Sampler` implementations

## Title
Implement the `Sampler` abstraction on both backends and override
`Device::createSampler`.

## Objective
Provide a usable, common `gpu::Sampler` mapping to `VkSampler` and
`MTL::SamplerState`, created from `SamplerDescription`.

## Context
Samplers are leaf resources with no per-frame state, so they can be created once
and reused. This is the smallest standalone backend object and is implemented
before `ResourceSet` (which references it).

## Expected previous state
- Step 02 complete: `gpu::Sampler` abstract + `Device::createSampler` stub exist.

## Files to review / modify
- Review: `lib/include/bg2e/gpu/vk/PipelineLayout.hpp` / `.cpp` and
  `lib/include/bg2e/gpu/metal/PipelineLayout.hpp` / `.cpp` (smallest concrete
  backend objects, for class style and `cleanup()` conventions);
  `lib/src/bg2e/gpu/vk/Format.cpp` and `lib/src/bg2e/gpu/metal/Format.cpp`
  (enum-to-native mapping style); `lib/src/bg2e/gpu/metal/Image.cpp`
  (`#if BG2E_IS_MAC` guarding pattern).
- Add: `lib/include/bg2e/gpu/vk/Sampler.hpp`, `lib/src/bg2e/gpu/vk/Sampler.cpp`,
  `lib/include/bg2e/gpu/metal/Sampler.hpp`, `lib/src/bg2e/gpu/metal/Sampler.cpp`.
- Modify: `lib/src/bg2e/gpu/vk/Device.cpp`, `lib/src/bg2e/gpu/metal/Device.cpp`
  (override `createSampler`), and the corresponding `vk::Device` /
  `metal::Device` headers.

## Proposed design
### Vulkan
`vk::Sampler : public gpu::Sampler` holds a `VkSampler` + `VkDevice`. Map
`SamplerDescription` fields with small local helpers:

- `Filter -> VkFilter` (`Nearest -> VK_FILTER_NEAREST`, `Linear -> VK_FILTER_LINEAR`).
- `MipmapFilter -> VkSamplerMipmapMode`.
- `AddressMode -> VkSamplerAddressMode`
  (`Repeat -> VK_SAMPLER_ADDRESS_MODE_REPEAT`, etc.).

Create with `vkCreateSampler`; `cleanup()` calls `vkDestroySampler`. Expose
`VkSampler handle() const`.

### Metal
`metal::Sampler : public gpu::Sampler` holds an `MTL::SamplerState*`. Build an
`MTL::SamplerDescriptor`, set `minFilter` / `magFilter`
(`MTL::SamplerMinMagFilterNearest|Linear`), `mipFilter`
(`MTL::SamplerMipFilterNearest|Linear`), and `sAddressMode` / `tAddressMode` /
`rAddressMode` (`MTL::SamplerAddressModeRepeat` / `ClampToEdge` /
`MirrorRepeat`). Create via `device->handle()->newSamplerState(desc)`;
`cleanup()` releases it. Expose `MTL::SamplerState* handle() const`.
Guard the implementation with `#if BG2E_IS_MAC` / `#else` throwing stubs exactly
like `metal::Image.cpp`.

### Device overrides
- `vk::Device::createSampler` -> `std::make_unique<vk::Sampler>(_device, desc)`.
- `metal::Device::createSampler` -> `std::make_unique<metal::Sampler>(this, desc)`.

## Required changes
1. Add the four files above (headers + sources), following the existing GPL
   banner and namespace nesting (`bg2e::gpu::vk` / `bg2e::gpu::metal`).
2. Declare and implement the `createSampler` override in each backend `Device`.

## Compilation criteria
- Project compiles on macOS (Xcode) and Linux/Windows: the Metal source is fully
  `#if BG2E_IS_MAC`-guarded; the Vulkan source is unconditional. New `.cpp`/`.hpp`
  under `lib/` are auto-globbed by CMake — no CMake edits.

## Validation criteria
- `device->createSampler({})` returns a valid sampler on both backends (can be
  spot-checked by the example in Step 12; not yet wired here).
- Existing examples behave identically.

## Risks / things to check
- `MTL::SamplerState` and `VkSampler` are created once and owned by the caller
  (`unique_ptr`). Make ownership/`cleanup()` symmetric with `vk::PipelineLayout`.
- Verify `metalcpp` exposes `MTL::SamplerDescriptor` / `newSamplerState`
  (it does via `Metal/Metal.hpp` already pulled by `metal/common.hpp`).
- Default `SamplerDescription{}` (linear/linear/nearest/repeat) must produce a
  valid sampler on both backends.

## What NOT to do in this step
- Do not reference samplers from any `ResourceSet` (Step 06) or shader yet.
- Do not add anisotropy, LOD clamping, comparison samplers, or borders.
