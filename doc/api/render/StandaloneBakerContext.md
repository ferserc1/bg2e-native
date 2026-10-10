# StandaloneBakerContext and StandaloneLightmapBaker

**Header:** `<bg2e/render/StandaloneBakerContext.hpp>`
**Namespace:** `bg2e::render`

`StandaloneBakerContext final : BakerContext` retains a `scene::Scene`, owns a
single execution frame and TLAS, and provides synchronous baking without a
window, camera, `MainLoop`, renderer or `OffscreenApplication`.

| Signature | Contract |
|---|---|
| `StandaloneBakerContext(Engine*, std::shared_ptr<scene::Scene>)` | Requires a scene/root and initialized ray-tracing engine. Root identity is fixed. |
| `void initialize(VkExtent2D)` | Positive extent; valid only in Created state. Allocate execution, lighting and environment resources. |
| `void updateScene(float deltaSeconds)` | Valid after initialization; finite nonnegative delta. Wait for old work, run resize once and component update, refresh light/environment caches, build/submit/wait the complete scene TLAS. Reset all live bakers and advance their scene generation. |
| `std::shared_ptr<StandaloneLightmapBaker> createBaker(std::shared_ptr<scene::Node>, LightmapSettings = {})` | Valid only after successful `updateScene()`. Store a weak tracking reference; validate target and settings. |
| `void cleanup()` | Wait and release owned execution resources. Idempotent after cleanup; context cannot be reinitialized. Invalid during a baker update. |
| destructor | Call `cleanup()`. Engine must still be alive. |

Scene states are Created → Initialized → SceneReady → Cleaned. Another
`updateScene()` is allowed in SceneReady; failure leaves the context Initialized,
so baking must wait for a successful update. Replacing `Scene::rootNode()`
requires a new context. Concurrent/reentrant baker updates are rejected.

`StandaloneLightmapBaker final : LightmapBaker` adds `void update()`: record one
sample using the prepared scene generation, submit to the graphics queue, wait
on the private fence, clear descriptors, mark result and call `Engine::nextFrame()`.
It does not update components or rebuild the TLAS. Invalid state, stale generation,
concurrent update or exhausted accumulation throws `std::logic_error`; submission
and fence failures report runtime errors.

```cpp
bg2e::render::Engine engine;
engine.init();
{
    bg2e::render::StandaloneBakeSceneAssembler assembler(&engine);
    auto assembly = assembler.assembleModel("studio.json", "chair.bg2");
    auto context = std::make_shared<bg2e::render::StandaloneBakerContext>(
        &engine, assembly.scene);
    context->initialize({512, 512});
    context->updateScene(0.0f);
    auto baker = context->createBaker(assembly.targets.at(0).node);
    while (baker->completedFrames() < baker->settings().accumulationFrames)
        baker->update();
    auto pixels = baker->readPixels();
    baker.reset();
    context->cleanup();
}
engine.cleanup();
```

Every Drawable needed for tracing must be GPU-loaded before `updateScene()`.
The context owns environment resources derived from the scene, with a procedural
sky-dome fallback. Camera caches are refreshed only if a camera exists.
Environment ownership is retained until engine cleanup callbacks run.

Sources: [header](../../../lib/include/bg2e/render/StandaloneBakerContext.hpp),
[implementation](../../../lib/src/bg2e/render/StandaloneBakerContext.cpp).
See [assembler](StandaloneBakeSceneAssembler.md), [batch](StandaloneBakeBatch.md),
and [context architecture](../../architecture/lightmaps/contexts.md).
