# Step 009 — Base `gpu::GraphicsPipeline` + description

## Objective

Introduce the backend-independent `gpu::GraphicsPipeline` and `GraphicsPipelineDescription`
(minimal: vertex + fragment shader modules, attachment formats, primitive topology), and a
compile-safe `Device::createGraphicsPipeline` hook. No backend implementation.

## Context

Both backends need, at pipeline-creation time, the **attachment pixel formats** (Vulkan dynamic
rendering: `VkPipelineRenderingCreateInfo`; Metal: `RenderPipelineDescriptor` color/depth formats),
the **shader stages**, and the **pipeline layout**. The first triangle has no vertex input, so the
description omits vertex attributes entirely.

## Expected prior state

- Steps 003–008 done (shader modules + pipeline layout exist). Build green.

## Files expected to review / modify

- Create: `lib/include/bg2e/gpu/GraphicsPipeline.hpp` — `gpu::GraphicsPipeline` (abstract) +
  `GraphicsPipelineDescription` + `PrimitiveTopology` enum.
- Modify: `lib/include/bg2e/gpu/Device.hpp` — add `createGraphicsPipeline(...)` non-pure virtual,
  throwing default.
- Modify: `lib/include/bg2e/gpu/all.hpp` — include the new header.
- Review: `gpu/Common.hpp` (`PixelFormat`), `gpu/ShaderModule.hpp`, `gpu/PipelineLayout.hpp`.

## Proposed design

```cpp
// gpu/GraphicsPipeline.hpp  (sketch)
namespace bg2e::gpu {
enum class PrimitiveTopology { TriangleList, TriangleStrip, LineList, PointList };

struct GraphicsPipelineDescription {
    gpu::ShaderModule*   vertexShader   = nullptr; // not owned
    gpu::ShaderModule*   fragmentShader = nullptr; // not owned
    gpu::PipelineLayout* layout         = nullptr; // not owned
    PrimitiveTopology    topology       = PrimitiveTopology::TriangleList;
    PixelFormat          colorFormat    = PixelFormat::Undefined; // from surface frame
    PixelFormat          depthFormat    = PixelFormat::Undefined; // Undefined => no depth
    // no vertex input layout in this first iteration (vertex_id only)
    // future: cull mode, blend state, depth test/write
};

class BG2E_API GraphicsPipeline {
public:
    virtual ~GraphicsPipeline() = default;
    virtual bool isValid() const = 0;
    virtual void cleanup() = 0;
};
}
```

- `Device` gains
  `virtual std::unique_ptr<gpu::GraphicsPipeline> createGraphicsPipeline(const GraphicsPipelineDescription&);`
  with throwing default.
- Decision point — **fixed-function defaults:** for the first triangle, hardcode sensible defaults
  inside each backend (no blending, depth test off/optional, cull none, fill solid, viewport/scissor
  dynamic on Vulkan). Document them as defaults so the description stays small; expose them later.

## Required changes (no code in this plan)

- New header only; default-throwing `createGraphicsPipeline` on `gpu::Device`.

## Compilation criteria

- Build green; both backends inherit the throwing default.

## Validation criteria

- Types visible via `all.hpp`; nothing instantiates a pipeline yet.

## Risks / points to check

- Pointers (`vertexShader`, `layout`...) are non-owning; the caller (example) must keep them alive
  for the pipeline's lifetime — document clearly.
- `depthFormat == Undefined` must mean "no depth attachment" consistently in both backends.

## What must NOT be done in this step

- No backend implementation (010/011). No vertex-input / blend / depth-state surface yet.
