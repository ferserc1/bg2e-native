# Step 003 — Base `gpu::ShaderModule` + `ShaderStage`

## Objective

Introduce the backend-independent shader-module abstraction and the supporting `ShaderStage` enum
and `ShaderModuleDescription`, plus a compile-safe `Device` factory hook. No backend implementation.

## Context

Pipelines need shader stages. Vulkan loads SPIR-V into a `VkShaderModule`; Metal loads a `.metallib`
into an `MTL::Library` and fetches an `MTL::Function` by name. A common `gpu::ShaderModule` must
abstract "one shader stage entry point" so a pipeline can reference it uniformly. Device owns the
native device, so it is the natural factory (mirrors `Queue::createCommandBuffer`).

## Expected prior state

- Steps 001–002 done. Engine + examples build on all platforms.

## Files expected to review / modify

- Create: `lib/include/bg2e/gpu/ShaderModule.hpp` — `gpu::ShaderModule` (abstract) + `ShaderStage`
  enum + `ShaderModuleDescription` struct.
- Modify: `lib/include/bg2e/gpu/Device.hpp` — add `createShaderModule(...)` factory as a **non-pure**
  virtual with a default that throws "not implemented" (compile-safe pattern; see summary §4.2).
- Modify: `lib/include/bg2e/gpu/all.hpp` — include the new header.
- Review: `lib/include/bg2e/gpu/Common.hpp` (where enums live) to keep style consistent.

## Proposed design

```cpp
// gpu/ShaderModule.hpp  (sketch — not to be implemented here)
namespace bg2e::gpu {
enum class ShaderStage { Vertex, Fragment, Compute };

struct ShaderModuleDescription {
    std::string filePath;            // Vulkan: .spv ; Metal: .metallib
    std::string entryPoint = "main"; // Vulkan: SPIR-V entry ; Metal: MTL function name
    ShaderStage stage = ShaderStage::Vertex;
};

class BG2E_API ShaderModule {
public:
    virtual ~ShaderModule() = default;
    virtual ShaderStage stage() const = 0;
    virtual bool isValid() const = 0;
    virtual void cleanup() = 0;
};
}
```

- `Device` gains:
  `virtual std::unique_ptr<gpu::ShaderModule> createShaderModule(const ShaderModuleDescription&);`
  with a default body throwing `std::runtime_error("createShaderModule not implemented")`.
- Design decision: the description carries a **file path** (not raw bytes) so each backend reads its
  own format. *Alternative:* accept a byte span for SPIR-V and a `(path, function)` for Metal — more
  flexible but heavier; defer.

## Required changes (no code in this plan)

- New header only; no `.cpp` (abstract). Auto-glob picks up the header; no CMake change.
- Default-throwing `createShaderModule` on `gpu::Device` so both backends still compile unchanged.

## Compilation criteria

- Engine + examples build on all platforms; `vk::Device` and `metal::Device` compile **without**
  overriding `createShaderModule` (they inherit the throwing default).

## Validation criteria

- `#include <bg2e/gpu/all.hpp>` exposes `gpu::ShaderModule`, `ShaderStage`, `ShaderModuleDescription`.
- No instantiation anywhere yet.

## Risks / points to check

- Keep `ShaderStage` minimal (Vertex/Fragment/Compute) for now; extending later is additive.
- Ensure the default-throwing method does not become pure virtual (would break backend compile).

## What must NOT be done in this step

- No `vk::` / `metal::` implementation (steps 004/005).
- No pipeline types yet.
