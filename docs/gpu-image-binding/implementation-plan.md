# Implementation Plan — `bg2e::gpu` basic image & sampler binding

## Purpose

Extend the low-level, backend-agnostic `bg2e::gpu` API with a **minimal, common
abstraction for passing images and samplers to shaders**, and use it to grow the
existing example `examples/gpu/05_simple_triangle` until:

1. A **compute shader** writes a gradient background directly onto the frame
   color image.
2. The triangle is rendered **on top** of that gradient.
3. The triangle **samples a texture** in its fragment shader.
4. Everything works on **Vulkan and Metal** through the common `bg2e::gpu` API.

This is the first iteration of resource binding. It is **not** a full material
system, advanced descriptor management, or bindless. The goal is a clear, small
API surface that maps cleanly to both Vulkan descriptor sets and Metal direct
binding.

## Hard constraints (carried into every step)

- All engine API changes are confined to the `bg2e::gpu` namespace
  (`lib/include/bg2e/gpu/**`, `lib/src/bg2e/gpu/**`). No other engine namespace
  may change.
- CMake files must not be modified unless strictly required for build (the
  project auto-globs `lib/**` and `examples/**`, so new files under those trees
  need no CMake edits). New shaders inside the example are picked up by the
  existing `compile_metal_shaders` / GLSL build rules already wired in the
  example's `CMakeLists.txt`.
- New / modified shaders live **only** inside
  `examples/gpu/05_simple_triangle/shaders/`. No shaders are added to the engine
  shader library.
- The example `05_simple_triangle` remains the functional validation target.
- **The project must compile at the end of every step.** New virtual methods are
  added to existing abstract base classes with default implementations that throw
  (the pattern already used by `Device`, `CommandBuffer`), so neither backend is
  ever left abstract/uninstantiable mid-plan.
- Do **not** attempt to compile unless the user explicitly asks. "Compiles" below
  is a design contract, not an instruction to run the build.

## Key facts discovered in the current codebase

These drive several design decisions and are referenced by the steps.

- `gpu::PipelineLayoutDescription` (in `Common.hpp`) currently carries only
  `pushConstants`. There is already a `// future additions` comment reserving
  space for descriptor/binding descriptions. We extend it here.
- `gpu::ImageLayout` already includes `General`, `ColorAttachment`,
  `DepthAttachment`, `ShaderReadOnly`, `Present`, etc., and the Vulkan backend
  already maps `General -> VK_IMAGE_LAYOUT_GENERAL`. No new layout enum is needed.
- Vulkan swapchain images are created in
  `vk::WindowSurface::createRenderTarget` with
  `VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT` and the
  surface prefers an **sRGB** color format (`VK_COLOR_SPACE_SRGB_NONLINEAR_KHR`).
  **sRGB formats cannot be used as `VK_DESCRIPTOR_TYPE_STORAGE_IMAGE`** (they lack
  `VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT`). This is the single most important
  portability risk and is addressed explicitly in Step 08.
- Vulkan `vk::Image::buildTargetImage` already adds `SAMPLED | TRANSFER_SRC |
  TRANSFER_DST | COLOR_ATTACHMENT`. Swapchain-wrapped images
  (`initFromSwapchainImage`) inherit the swapchain `imageUsage`.
- Metal drawable wrapping is in `metal::WindowSurface::beginFrame` via
  `initFromDrawableTexture`. `CAMetalLayer` defaults to `framebufferOnly = true`,
  which **forbids using the drawable texture as a compute write target**. This is
  addressed in Step 08 (`setFramebufferOnly(false)`).
- Vulkan render attachment already uses `LOAD_OP_LOAD` when no clear value is
  supplied (`Info::attachmentInfo`). Metal's `beginRendering` currently sets
  `LoadActionDontCare` when no clear is requested, which would discard the compute
  gradient. Step 08 changes the Metal default to `LoadActionLoad` so prior color
  content (the gradient) is preserved.
- `Device::immediateSubmit` exists on both backends and is the mechanism used by
  `readPixelsRGBA8` (Vulkan staging `VkBuffer` + `vkCmdCopyImageToBuffer`; Metal
  staging `MTL::Buffer` + blit). The texture **upload** path in Steps 10–11
  mirrors this exact pattern in reverse.
- `metal::PipelineLayout` already stores its `PipelineLayoutDescription` and
  exposes a `pushConstantBufferIndex(stage)`. It is the natural home for the
  Metal binding table derived from the resource bindings.
- `vk::CommandBuffer` already tracks `_boundLayoutHandle` for graphics push
  constants; the compute path does not yet track a layout. `bindResourceSet`
  takes the pipeline explicitly (see Step 07) to obtain both the layout handle
  and the bind point, mirroring the existing `bindPipeline` overload pattern.

## Design decisions (and their justification)

### 1. Resource model — abstract shader resources, not Vulkan concepts

The common `ResourceType` enum is:

```cpp
enum class ResourceType {
    UniformBuffer,
    StorageBuffer,
    SampledImage,
    StorageImage,
    Sampler
    // CombinedImageSampler intentionally omitted for this iteration (see below)
};
```

For this iteration the engine only needs `StorageImage` (compute writes the
gradient) and `SampledImage` + `Sampler` (fragment samples a texture).
`UniformBuffer` / `StorageBuffer` are declared in the enum so the API shape is
stable, but they are **not implemented** in this plan (they map to clear future
work and keep the enum from churning later).

### 2. Separate `SampledImage` + `Sampler` instead of `CombinedImageSampler`

**Decision: use separate `SampledImage` (`VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE`) and
`Sampler` (`VK_DESCRIPTOR_TYPE_SAMPLER`) descriptors, one binding each.**

Justification:

- The desired common model in the task lists `SampledImage` and `Sampler` as two
  distinct resources, and the conceptual `ResourceSet` API uses two separate
  calls (`setSampledImage`, `setSampler`).
- Metal has **no** combined-sampler concept: a texture and a sampler are always
  bound independently (`setFragmentTexture` / `setFragmentSamplerState`), each in
  its own index namespace. Separate Vulkan descriptors map **1:1** onto Metal,
  keeping the common abstraction honest on both backends.
- GLSL expresses this cleanly with
  `texture(sampler2D(uTex, uSampler), uv)` where `uTex` is `texture2D` and
  `uSampler` is `sampler`. This keeps the example shaders explicit about the two
  resources.
- `CombinedImageSampler` can be added later as an optional optimization without
  breaking the separate model. It is omitted now to avoid two parallel code paths.

### 3. `bindResourceSet`, not `bindImage` / `bindSampler`

**Decision: add `CommandBuffer::bindResourceSet(...)` only. Do not add
`bindImage` / `bindSampler`.**

Justification:

- `ResourceSet` is the stated common model and maps directly to a Vulkan
  `VkDescriptorSet` (`vkCmdBindDescriptorSets`). Per-resource binds would leak a
  Metal-shaped, set-less idiom into the common command buffer and produce a
  second binding path to maintain.
- On Metal, `bindResourceSet` simply iterates the set's entries and resolves them
  to `setTexture` / `setFragmentTexture` / `setFragmentSamplerState` /
  `setBytes` on the currently active encoder, driven by each binding's stage and
  type (taken from the `PipelineLayout`). The set-less nature of Metal is an
  implementation detail hidden behind the common API.

### 4. Separate layouts and resource sets for compute and graphics

**Decision: the compute pass (gradient) and the graphics pass (triangle) each
get their own `PipelineLayout` and their own `ResourceSet`(s).**

Justification: they bind different resources to different stages
(`StorageImage`/Compute vs. `SampledImage`+`Sampler`/Fragment). One combined
layout would force unused bindings into both pipelines. Separate layouts are the
simplest correct option and match the task's recommendation.

### 5. Storage-capable color target — preferred path + documented fallback

**Preferred (and the path this plan takes): write the gradient directly into
`frame->colorImage()`.** This requires:

- **Vulkan**: request a **storage-capable, non-sRGB** swapchain format
  (`B8G8R8A8_UNORM`) and add `VK_IMAGE_USAGE_STORAGE_BIT` to the swapchain
  `imageUsage` (guarded by `VkSurfaceCapabilitiesKHR::supportedUsageFlags`). If
  the surface does not support storage usage, fall back to the offscreen path.
- **Metal**: set `CAMetalLayer::setFramebufferOnly(false)` so the drawable
  texture can be bound for shader write.

**Documented fallback (Step 08 risk section, not implemented unless the preferred
path proves unavailable on the target hardware):** compute writes to an
offscreen `StorageImage`, then the graphics pass either samples it as a
full-screen background or it is blitted/copied into the color attachment before
the triangle is drawn.

Moving from sRGB to UNORM swapchain output changes the perceived gamma of the
existing example slightly; this is acceptable for a low-level example and is
called out in Step 08.

## Milestones

- **End of Part 1 (Step 09):** compute shader writes a gradient into the frame
  color image, the triangle is composited on top. Resource-binding API
  (`Sampler`, `ResourceSet`, `ResourceBinding`, extended `PipelineLayout`,
  `bindResourceSet`) is complete and used by the compute pass.
- **End of Part 2 (Step 12):** `Image` gains a procedural-data upload API backed
  by an internal native staging buffer; the example creates a 2×2 procedural
  texture and the triangle samples it. Final target reached on both backends.

## Step index

### Part 1 — Pass images/samplers to shaders, then the compute gradient

| Step | Title | Functional? |
|---|---|---|
| [01](implementation-plan/step-01-common-resource-types.md) | Common resource-binding types & descriptors | No (data only) |
| [02](implementation-plan/step-02-sampler-abstract.md) | `gpu::Sampler` abstract class + `Device::createSampler` stub | No |
| [03](implementation-plan/step-03-sampler-backends.md) | `vk::Sampler` + `metal::Sampler` implementations | No |
| [04](implementation-plan/step-04-resourceset-abstract.md) | `gpu::ResourceSet` abstract class + `Device::createResourceSet` stub | No |
| [05](implementation-plan/step-05-pipelinelayout-bindings.md) | Extend `PipelineLayout` backends with resource bindings | No |
| [06](implementation-plan/step-06-resourceset-backends.md) | `vk::ResourceSet` + `metal::ResourceSet` implementations | No |
| [07](implementation-plan/step-07-bind-resourceset.md) | `CommandBuffer::bindResourceSet(...)` (base + both backends) | No |
| [08](implementation-plan/step-08-storage-color-target.md) | Make color targets storage-capable; Metal load-action fix | No (infra) |
| [09](implementation-plan/step-09-example-gradient.md) | Example: gradient compute shaders + storage-image binding | **Milestone 1** |

### Part 2 — Procedural texture upload, then the textured triangle

| Step | Title | Functional? |
|---|---|---|
| [10](implementation-plan/step-10-image-create-api.md) | `Device::createImage` + `ImageDescription` for sampled textures | No |
| [11](implementation-plan/step-11-image-upload-api.md) | `Image::uploadRGBA8` with internal native staging buffer | No |
| [12](implementation-plan/step-12-example-texture.md) | Example: 2×2 procedural texture + sampler + textured triangle | **Milestone 2** |

## Suggested commit boundaries

Each step is a single self-contained commit. Steps 01–07 build the API surface
(no behavior change to existing examples). Step 08 changes resource creation only
(still no visible change). Step 09 is the first visible result. Steps 10–11 add
the upload API (no visible change). Step 12 is the final result.
