# Step 006 — Base `gpu::PipelineLayout` + layout description

## Objective

Introduce the backend-independent `gpu::PipelineLayout` and a `PipelineLayoutDescription` that, for
now, describes only **push-constant / small-constant ranges** (the minimal data a pipeline needs).
Add a compile-safe `Device::createPipelineLayout` hook. No backend implementation.

## Context

Vulkan requires a `VkPipelineLayout` to create any pipeline; Metal has no equivalent object but the
common API keeps a `PipelineLayout` to stay symmetric and to host the future resource-binding /
argument-buffer metadata. The first pipeline (triangle) needs *no* descriptor sets — at most a push
constant for color (step 021). So the layout description starts minimal and grows additively.

## Expected prior state

- Steps 003–005 done (shader modules exist). Build green on all platforms.

## Files expected to review / modify

- Create: `lib/include/bg2e/gpu/PipelineLayout.hpp` — `gpu::PipelineLayout` (abstract) +
  `PushConstantRange` + `PipelineLayoutDescription`.
- Modify: `lib/include/bg2e/gpu/Device.hpp` — add `createPipelineLayout(...)` non-pure virtual with
  throwing default (compile-safe pattern).
- Modify: `lib/include/bg2e/gpu/all.hpp` — include the new header.
- Review: `gpu/ShaderModule.hpp` for `ShaderStage` reuse (push constants need a stage mask).

## Proposed design

```cpp
// gpu/PipelineLayout.hpp  (sketch)
namespace bg2e::gpu {
struct PushConstantRange {
    uint32_t    offset = 0;
    uint32_t    size   = 0;     // keep within Vulkan-compatible limits (<= 128 bytes guideline)
    ShaderStage stage  = ShaderStage::Vertex; // or a stage mask later
};

struct PipelineLayoutDescription {
    std::vector<PushConstantRange> pushConstants; // empty for the first triangle
    // future: descriptor/binding set descriptions
};

class BG2E_API PipelineLayout {
public:
    virtual ~PipelineLayout() = default;
    virtual bool isValid() const = 0;
    virtual void cleanup() = 0;
};
}
```

- `Device` gains
  `virtual std::unique_ptr<gpu::PipelineLayout> createPipelineLayout(const PipelineLayoutDescription&);`
  with throwing default.
- Decision point — **stage as single enum vs bitmask:** start with a single `ShaderStage`. Vulkan
  push-constant ranges accept a stage flags mask; a single stage maps trivially. Document upgrading
  to a mask later (additive).
- Decision point — **push-constant size limit:** document the 128-byte conservative ceiling so Metal
  `setBytes` (4 KB limit) and Vulkan (min guaranteed 128 bytes) both stay valid.

## Required changes (no code in this plan)

- New header only; no `.cpp`. Default-throwing `createPipelineLayout` on `gpu::Device`.

## Compilation criteria

- Build green everywhere; both backends inherit the throwing default unchanged.

## Validation criteria

- `gpu::PipelineLayout`, `PushConstantRange`, `PipelineLayoutDescription` visible via `all.hpp`.
- Nothing instantiates a layout yet.

## Risks / points to check

- Keep the description trivially copyable / cheap; it is passed by const ref.
- Do not encode any Vulkan- or Metal-specific type in the common header.

## What must NOT be done in this step

- No backend implementation (007/008).
- No descriptor-set / resource-set types yet (future phase).
