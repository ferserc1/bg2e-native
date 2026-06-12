# Step 02 — `gpu::Sampler` abstract class + `Device::createSampler` stub

## Title
Declare the common `gpu::Sampler` interface and a default-throwing
`Device::createSampler` factory method.

## Objective
Add the abstract `gpu::Sampler` type and the `Device` factory entry point so
later steps can implement and use samplers, while keeping the project compiling
(no backend implementation yet — the factory throws like the existing
`createShaderModule` / `createGraphicsPipeline` defaults).

## Context
`Device` already follows a pattern where new factory methods are declared as
`virtual ... { throw std::runtime_error("... not implemented"); }`. We reuse it,
so neither `vk::Device` nor `metal::Device` becomes abstract and both remain
instantiable.

## Expected previous state
- Step 01 complete: `SamplerDescription` and its enums exist in `Common.hpp`.

## Files to review / modify
- Review: `lib/include/bg2e/gpu/Device.hpp` (factory method pattern),
  `lib/include/bg2e/gpu/all.hpp` (umbrella include list),
  `lib/include/bg2e/gpu/ShaderModule.hpp` (style reference for a small abstract).
- Add: `lib/include/bg2e/gpu/Sampler.hpp`.
- Modify: `lib/include/bg2e/gpu/Device.hpp`, `lib/include/bg2e/gpu/all.hpp`.

## Proposed design
New header `Sampler.hpp`:

```cpp
#pragma once
#include <bg2e/common.hpp>
#include <bg2e/gpu/Common.hpp>

namespace bg2e { namespace gpu {

class BG2E_API Sampler {
public:
    virtual ~Sampler() = default;
    virtual bool isValid() const = 0;
    virtual void cleanup() = 0;
};

}}
```

In `Device.hpp`, forward-declare `Sampler` and add:

```cpp
virtual std::unique_ptr<Sampler> createSampler(const SamplerDescription& description)
{
    throw std::runtime_error("createSampler not implemented");
}
```

Add `#include <bg2e/gpu/Sampler.hpp>` to `all.hpp`.

## Required changes
1. Create `lib/include/bg2e/gpu/Sampler.hpp` with the abstract class above
   (GPL header banner like every other file).
2. In `Device.hpp`: forward-declare `class Sampler;` (next to the other forward
   declarations) and add the `createSampler` default-throwing virtual. Include
   `<memory>` is already present.
3. Add the new header to `all.hpp`.

## Compilation criteria
- Project compiles: only additive declarations; the default throw keeps both
  concrete `Device` subclasses instantiable.

## Validation criteria
- Existing examples behave identically (nothing calls `createSampler` yet).

## Risks / things to check
- Keep the `Sampler` forward declaration and the factory return type
  (`std::unique_ptr<Sampler>`) consistent with the other `Device` factories.
- Do not include `Sampler.hpp` from `Device.hpp` if a forward declaration
  suffices (matches how `ShaderModule` / `PipelineLayout` are handled), to avoid
  unnecessary header coupling.

## What NOT to do in this step
- Do not implement `vk::Sampler` / `metal::Sampler` (Step 03).
- Do not override `createSampler` in either backend yet.
