# Step 03: Build the context-owned bake RT scene

## Scope

Make `IntegratedBakerContext` own production `RayTracingScene` instances built from its scene root. Use one instance per in-flight frame slot so an active GPU submission never loses its TLAS when `FrameResources::flushFrameData()` runs or another slot is recorded. Prepare the selected slot once per bake frame and share its TLAS, object/material/light bindings among all target bakers. Preserve existing `RendererDeferred` and `DeferredLayer` public APIs.

## Acceptance and compile gate

Integrated context can prepare its own TLAS and reject a missing preparation, invalid frame slot or mismatched root; the ordinary renderer's frame TLAS may be absent or cleaned without invalidating the bake TLAS. Existing rendering still compiles. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Add `IntegratedBakerContext::prepareFrame(VkCommandBuffer, vulkan::FrameResources&)` and a private per-slot `std::vector<std::unique_ptr<vulkan::rt::RayTracingScene>>`. Define the context destructor out of line where `RayTracingScene` is complete. Require the supplied frame to be `Engine::currentFrameResources()` and call it once per active bake frame after the application has updated components, before any baker `update`. Select a context-owned scene by `Engine::currentFrameResourcesIndex()`; allocate it on demand. Record `RayTracingScene::update(cmd, rootNode())` for the **context root**, not the target node. Reject a null/empty TLAS when no traceable target exists. This method invokes no scene lifecycle hooks.
- The application loop has waited for the selected slot's `frameFence` before recording. After that wait, release that slot's previous bake TLAS (`cleanup()`) before calling `RayTracingScene::update`; its current implementation otherwise replaces buffers and handles without releasing the old acceleration structure. Never clean another in-flight slot. `FrameResources::flushFrameData()` may clean its own `rayTracingScene` but cannot touch context-owned scenes. Context destruction/scene swap waits for all affected submissions or defers their destruction.
- Use the selected context-owned scene's `tlas()` and `objectInstances()` with current scene lights, environment and `RTMaterialDataBinding`. `FrameResources` may allocate descriptors valid only through the current submission; do not retain it or those descriptors. Factor shared binding preparation out of `DeferredLayer` only if needed. Do not change existing `RendererDeferred` or `DeferredLayer` public signatures. Do not add `RayTracingScene::lastRootNode()`: the context itself supplies the root to its owned scene.
- Extend the TLAS build barrier's destination visibility beyond the current fragment-only stage to the actual compute and ray-tracing shader consumers. `IntegratedLightmapBaker::update(cmd, frame)` must verify that `prepareFrame` was called for the same `Engine::currentFrame()` number, command buffer and current slot, that its target still belongs to the context root, and that the owned TLAS and frame descriptor allocator are valid. Each baker may consume the prepared TLAS once per frame; several bakers share it without another build.
- A render-delegate fixture prepares and bakes in the same command stream both before and after ordinary scene rendering. It must work even when `frame.rayTracingScene` is null or cleaned, reject `update` before `prepareFrame` (including a recycled slot whose command buffer handle is reused), reject a target moved to another root, and survive two in-flight slots plus a return to an earlier slot without leaking or freeing an in-flight TLAS.

## Frame-order example

```cpp
// Application component update has already run in RenderLoop.
context->prepareFrame(cmd, frame); // builds this slot's context-owned TLAS once
bakerA->update(cmd, frame);         // consumes the prepared TLAS
bakerB->update(cmd, frame);         // shares it without rebuilding
// Ordinary renderer may record before or after this sequence.
```

## Handoff

After this step, complete [prestep_04_rtao.md](prestep_04_rtao.md) for the next step (Implement RTAO baking).
