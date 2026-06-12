# Step 04 — `gpu::ResourceSet` abstract class + `Device::createResourceSet` stub

## Title
Declare the common `gpu::ResourceSet` interface and a default-throwing
`Device::createResourceSet` factory.

## Objective
Add the abstract resource-set type that represents a group of shader resources
(maps to a Vulkan `VkDescriptorSet`; to a resolved binding table on Metal), plus
the `Device` factory entry point. No backend implementation yet.

## Context
A `ResourceSet` is created against a `PipelineLayout` and a `set` index, has its
resources assigned, then `update()`-d before being bound. This step only declares
the interface; Step 06 implements it. Default-throwing factory keeps both
backends instantiable.

## Expected previous state
- Steps 01–03 complete: `ResourceType`/`ResourceBinding` exist, `gpu::Sampler`
  is implemented on both backends.

## Files to review / modify
- Review: `lib/include/bg2e/gpu/Device.hpp`, `lib/include/bg2e/gpu/Image.hpp`,
  `lib/include/bg2e/gpu/Sampler.hpp`, `lib/include/bg2e/gpu/all.hpp`.
- Add: `lib/include/bg2e/gpu/ResourceSet.hpp`.
- Modify: `lib/include/bg2e/gpu/Device.hpp`, `lib/include/bg2e/gpu/all.hpp`.

## Proposed design
New header `ResourceSet.hpp`:

```cpp
#pragma once
#include <bg2e/common.hpp>
#include <bg2e/gpu/Common.hpp>
#include <cstdint>

namespace bg2e { namespace gpu {

class Image;
class Sampler;

class BG2E_API ResourceSet {
public:
    virtual ~ResourceSet() = default;

    virtual void setStorageImage(uint32_t binding, gpu::Image* image)   = 0;
    virtual void setSampledImage(uint32_t binding, gpu::Image* image)   = 0;
    virtual void setSampler(uint32_t binding, gpu::Sampler* sampler)    = 0;
    // Reserved for future iterations (declare but may be left unimplemented
    // / throwing in the backends until needed):
    // virtual void setUniformBuffer(uint32_t binding, gpu::Buffer* buffer) = 0;

    virtual void update() = 0;   // flush assignments to the backend

    virtual uint32_t setIndex() const = 0;
    virtual bool isValid() const = 0;
    virtual void cleanup() = 0;
};

}}
```

In `Device.hpp`, forward-declare `ResourceSet` and `PipelineLayout` (already
present) and add:

```cpp
virtual std::unique_ptr<ResourceSet> createResourceSet(
    PipelineLayout* layout, uint32_t setIndex)
{
    throw std::runtime_error("createResourceSet not implemented");
}
```

Add `#include <bg2e/gpu/ResourceSet.hpp>` to `all.hpp`.

## Required changes
1. Create `ResourceSet.hpp` with the abstract interface (GPL banner).
2. Add the `createResourceSet` default-throwing virtual + `class ResourceSet;`
   forward declaration to `Device.hpp`.
3. Add the header to `all.hpp`.

## Compilation criteria
- Project compiles: additive declarations only; the default-throwing factory
  keeps `vk::Device` / `metal::Device` concrete.

## Validation criteria
- Existing examples behave identically (nothing calls `createResourceSet`).

## Risks / things to check
- The factory takes a non-owning `PipelineLayout*` (caller keeps it alive),
  matching how `GraphicsPipelineDescription::layout` is treated.
- Keep `setStorageImage`/`setSampledImage`/`setSampler` as the minimal surface;
  do not add buffer setters as pure virtuals (that would force backends to
  implement them now). Buffer support is commented as future work.

## What NOT to do in this step
- Do not implement either backend `ResourceSet` (Step 06).
- Do not add `bindResourceSet` to `CommandBuffer` yet (Step 07).
