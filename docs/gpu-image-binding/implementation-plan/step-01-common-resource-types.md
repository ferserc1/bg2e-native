# Step 01 — Common resource-binding types & descriptors

## Title
Add the common, backend-agnostic resource-binding data types to `gpu::Common.hpp`.

## Objective
Introduce the pure-data vocabulary the rest of the plan depends on:
`ResourceType`, `ResourceBinding`, the extended `PipelineLayoutDescription`,
`SamplerDescription` (+ filter/address enums), and an `ImageUsage` flags enum.
No backend code, no behavior change.

## Context
`PipelineLayoutDescription` currently only describes push constants and already
carries a `// future additions` comment reserving room for descriptor/binding
descriptions. This step fills that reservation with stable data structures shared
by both backends. Everything here is header-only POD, so it cannot break either
backend.

## Expected previous state
- Plan not started. `gpu::Common.hpp` defines `ShaderStage`, `ImageLayout`,
  `PushConstantRange`, `PipelineLayoutDescription` (push constants only).

## Files to review / modify
- Review: `lib/include/bg2e/gpu/Common.hpp` (existing enums/structs and style).
- Modify: `lib/include/bg2e/gpu/Common.hpp` (add new types).

## Proposed design
Add to `bg2e::gpu` in `Common.hpp`:

```cpp
enum class ResourceType {
    UniformBuffer,   // reserved, not implemented this iteration
    StorageBuffer,   // reserved, not implemented this iteration
    SampledImage,    // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
    StorageImage,    // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
    Sampler          // VK_DESCRIPTOR_TYPE_SAMPLER
};

struct ResourceBinding {
    uint32_t     set     = 0;
    uint32_t     binding = 0;
    ResourceType type    = ResourceType::SampledImage;
    ShaderStage  stage   = ShaderStage::Fragment;
    uint32_t     count   = 1;   // array size; 1 for this iteration
};
```

Extend the existing struct (keep `pushConstants`, drop the stale comment):

```cpp
struct PipelineLayoutDescription {
    std::vector<PushConstantRange> pushConstants;
    std::vector<ResourceBinding>   resourceBindings; // empty == no sets
};
```

Add sampler description vocabulary:

```cpp
enum class Filter        { Nearest, Linear };
enum class MipmapFilter  { Nearest, Linear };
enum class AddressMode    { Repeat, ClampToEdge, MirroredRepeat };

struct SamplerDescription {
    Filter       minFilter   = Filter::Linear;
    Filter       magFilter   = Filter::Linear;
    MipmapFilter mipFilter   = MipmapFilter::Nearest;
    AddressMode  addressModeU = AddressMode::Repeat;
    AddressMode  addressModeV = AddressMode::Repeat;
    AddressMode  addressModeW = AddressMode::Repeat;
};
```

Add an `ImageUsage` bit-flags enum (mirrors the existing
`PipelineBarrierFlags` flag-enum idiom: `enum class` + `operator|` / `operator&`
/ `hasFlag` helpers):

```cpp
enum class ImageUsage : uint32_t {
    None            = 0,
    ColorAttachment = 1 << 0,
    DepthStencil    = 1 << 1,
    Sampled         = 1 << 2,
    Storage         = 1 << 3,
    TransferSrc     = 1 << 4,
    TransferDst     = 1 << 5,
    Present         = 1 << 6
};
// + operator|, operator&, operator|=, hasFlag(ImageUsage, ImageUsage)
```

## Required changes
1. In `Common.hpp`, add `ResourceType`, `ResourceBinding`, `Filter`,
   `MipmapFilter`, `AddressMode`, `SamplerDescription`, and `ImageUsage` (with
   its operators), copying the exact flag-operator style already used for
   `PipelineBarrierFlags`.
2. Add `resourceBindings` to `PipelineLayoutDescription`.
3. Keep `<vector>` / `<cstdint>` includes sufficient (already present).

## Compilation criteria
- The whole project still compiles: only additive, header-only declarations are
  introduced. Existing `PipelineLayoutDescription{}` aggregate initialization in
  the example (`device->createPipelineLayout({})`,
  `graphicsLayoutDesc.pushConstants.push_back(...)`) still compiles because the
  new member is default-constructed.

## Validation criteria
- `examples/gpu/05_simple_triangle` behaves exactly as before (no runtime change).
- Grep confirms the new types exist only in `bg2e::gpu`.

## Risks / things to check
- Aggregate initialization order: adding `resourceBindings` after `pushConstants`
  keeps existing `{ ... }` initializers valid. Verify no existing code uses
  positional initialization that would now mismatch (the example uses
  `pushConstants.push_back`, so it is safe).
- Ensure `ImageUsage` operator helpers do not clash with the existing
  `PipelineBarrierFlags` operators (different enum types, so overloads are
  distinct).

## What NOT to do in this step
- Do not add `gpu::Sampler`, `gpu::ResourceSet`, or any `Device` / backend code.
- Do not wire `resourceBindings` into any pipeline layout yet (Step 05).
- Do not add `CombinedImageSampler` to `ResourceType`.
