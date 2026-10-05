# Step 03: Reuse frame RT data safely

## Scope

Connect IntegratedBakerContext to the current frame's production RayTracingScene and object/material/light bindings. Expose narrowly scoped accessors or extract shared setup where required; preserve existing RendererDeferred and DeferredLayer APIs. Require invocation after TLAS update, include GPU synchronization, and never store borrowed frame handles past update. Validate the TLAS belongs to the supplied scene root.

## Acceptance and compile gate

Integrated context can inspect a prepared frame and reject missing or mismatched RT state; existing rendering still compiles. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Add an additive `RayTracingScene::lastRootNode() const` accessor, set by successful `update(cmd, root)` and cleared by `cleanup()`. The integrated baker compares this pointer with its context root on each update; it also rejects a null TLAS and missing descriptor allocator.
- Reuse `frameResources.rayTracingScene->tlas()` and `objectInstances()`, plus current scene lights, environment and `RTMaterialDataBinding`. Factor shared binding preparation out of `DeferredLayer` only if needed. Do not modify any existing `RendererDeferred` or `DeferredLayer` public signature.
- The TLAS build barrier currently targets fragment shaders; add the stage visibility needed by compute and ray-tracing shader consumers. Keep the borrowed TLAS and descriptors scoped to the recording call. A baker must not destroy resources owned by `FrameResources`.
- A render-delegate fixture records the bake after `prepareSceneRender`. A call before TLAS update or with a different root must fail; a subsequent frame must revalidate its new TLAS.

## Frame-order example

```cpp
renderer.prepareSceneRender(cmd, frameIndex, frame); // existing renderer path
baker->update(cmd, frame);                           // borrows this frame only
```

## Handoff

After this step, complete [prestep_04_rtao.md](prestep_04_rtao.md) for the next step (Implement RTAO baking).
