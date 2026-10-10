# IntegratedBakerContext and IntegratedLightmapBaker

**Header:** `<bg2e/render/IntegratedBakerContext.hpp>`
**Namespace:** `bg2e::render`

`IntegratedBakerContext final : BakerContext` records bake work into the
application's current production engine frame. It owns a TLAS and bake descriptor
allocator per frame slot, plus shared material/light bindings and UV lighting passes.
It does not submit frames or invoke scene lifecycle callbacks.

| Member | Contract |
|---|---|
| `IntegratedBakerContext(Engine*, scene::Node*)` | Root and engine must remain valid; requires ray tracing. |
| `prepareFrame(VkCommandBuffer, vulkan::FrameResources&)` | Capture lights and record a full-scene TLAS build once per engine frame. Command must be non-null and belong to the engine's current frame resources. |
| `setEnvironmentResources(EnvironmentResources*)` | Borrow the active renderer's IBL resources. Required for RTGI; the renderer must outlive their use. |
| `createBaker(std::shared_ptr<scene::Node>, LightmapSettings = {})` | Return `std::unique_ptr<IntegratedLightmapBaker>` retaining this shared context. Validate loaded target and UV2. |
| destructor | Defer release of GPU resources through `Engine::deferredExec`. |

`IntegratedLightmapBaker final : LightmapBaker` has one public additional method:
`void update(VkCommandBuffer, vulkan::FrameResources&)`. It records one full-atlas
sample after `prepareFrame()` succeeds for the same frame, slot, and command buffer.
Each baker may update once per engine frame. Repeated context preparation,
repeated baker update in one frame, missing preparation, or updating after the
configured accumulation count throws `std::logic_error`. Wrong command/frame
resources or detached targets throw `std::invalid_argument`.

```cpp
// engine and root are borrowed; renderer owns environmentResources().
auto context = std::make_shared<bg2e::render::IntegratedBakerContext>(engine, root);
context->setEnvironmentResources(renderer->environmentResources());
auto baker = context->createBaker(targetNode);

// After scene update, inside the current frame's command recording:
context->prepareFrame(cmd, frameResources);
baker->update(cmd, frameResources);
// Submit/advance through the application's render loop before readPixels().
```

Several bakers share one preparation. Keep geometry, UV2, materials and
lighting stable across accumulation, or invalidate/recreate the relevant resources.
`resetAccumulation()` does not remove the once-per-frame guard: a second update
in that same frame remains invalid. The prepared TLAS covers the whole context
root, not just the target.

Sources: [declarations](../../../lib/include/bg2e/render/IntegratedBakerContext.hpp),
[implementation](../../../lib/src/bg2e/render/IntegratedBakerContext.cpp).
See [LightmapBaker](LightmapBaker.md) and [context architecture](../../architecture/lightmaps/contexts.md).
