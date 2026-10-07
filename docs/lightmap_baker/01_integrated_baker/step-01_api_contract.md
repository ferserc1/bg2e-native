# Step 01: Define the public contract

## Scope

Add render API types for bake settings, RGB8/RGB32F result selection, an IntegratedBakerContext, and a per-target LightmapBaker. The context refers to one scene root; each baker owns its target image, intermediate images, accumulation history, and CPU result. Define lifetime, state transitions, RT-required errors, node-root and Drawable validation, full-resolution extent, and invalidation rules. Keep `IntegratedLightmapBaker::update(VkCommandBuffer, vulkan::FrameResources&)` as the recording entry point; the frame parameter will supply transient descriptors, while step 03 will add context-owned bake RT scenes. No rendering implementation is required yet.

## Acceptance and compile gate

Public headers and inert implementations compile; existing render API signatures remain intact. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Public headers under `lib/include/bg2e/render/`: `BakerContext.hpp`, `LightmapBaker.hpp`, `IntegratedBakerContext.hpp` and `LightmapSettings.hpp`; implementation under `lib/src/bg2e/render/`. Add them to `render/all.hpp`. Use the exact names and settings fields in [API_CONTRACT.md](../API_CONTRACT.md).
- `IntegratedBakerContext::createBaker(std::shared_ptr<scene::Node>, LightmapSettings)` returns `std::unique_ptr<IntegratedLightmapBaker>`. `IntegratedLightmapBaker::update(VkCommandBuffer, vulkan::FrameResources&)` is the recording entry point; common `LightmapBaker` exposes `completedFrames()`, `resetAccumulation()`, `readPixels()` and `image()`.
- No TLAS is allocated or borrowed in this API-scaffolding step. Step 03 adds `IntegratedBakerContext::prepareFrame(...)` and owns the bake `RayTracingScene` instances without changing the step-01 update signature. An already completed step 01 needs no source-code rework for this ownership correction.
- A baker retains a shared context reference; Engine and scene lifetime are external. Context construction checks `engine->rayTracingSupported()`. Baker construction checks root identity, Drawable type, submesh/index validity and settings. `resolution`, `accumulationFrames` and `samplesPerPixel` must be positive; error messages name the invalid field. The usable-UV2 check is added with the UV-space pass in step 02, so this step remains compilable with an explicit pending-validation error for unsupported input.
- Add inert implementations where later GPU steps are needed; throw a clear “pass not implemented” error rather than returning a successful empty map. Do not add a render loop or depend on `bg2e::gpu`.

## Review example

```cpp
auto ctx = std::make_shared<render::IntegratedBakerContext>(engine, root);
auto baker = ctx->createBaker(target, {.resolution = 512,
    .mode = render::LightmapMode::RTAO});
```

A detached node or node from another root must fail before allocating GPU targets. See the full frame-order example in [API_CONTRACT.md](../API_CONTRACT.md).

## Handoff

After this step, complete [prestep_02_uv_surface.md](prestep_02_uv_surface.md) for the next step (Build the UV-space surface pass).
