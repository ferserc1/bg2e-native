# Handoff for Reuse frame RT data safely

Next implementation step: [step-03_scene_bindings.md](step-03_scene_bindings.md). Complete this file **after** finishing step 02; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-03_scene_bindings.md.

## Next-step instructions

Connect IntegratedBakerContext to the current frame's production RayTracingScene and object/material/light bindings. Expose narrowly scoped accessors or extract shared setup where required; preserve existing RendererDeferred and DeferredLayer APIs. Require invocation after TLAS update, include GPU synchronization, and never store borrowed frame handles past update. Validate the TLAS belongs to the supplied scene root.

## Next-step acceptance gate

Integrated context can inspect a prepared frame and reject missing or mismatched RT state; existing rendering still compiles. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Add an additive `RayTracingScene::lastRootNode() const` accessor, set by successful `update(cmd, root)` and cleared by `cleanup()`. The integrated baker compares this pointer with its context root on each update; it also rejects a null TLAS and missing descriptor allocator.
- Reuse `frameResources.rayTracingScene->tlas()` and `objectInstances()`, plus current scene lights, environment and `RTMaterialDataBinding`. Factor shared binding preparation out of `DeferredLayer` only if needed. Do not modify any existing `RendererDeferred` or `DeferredLayer` public signature.
- The TLAS build barrier currently targets fragment shaders; add the stage visibility needed by compute and ray-tracing shader consumers. Keep the borrowed TLAS and descriptors scoped to the recording call. A baker must not destroy resources owned by `FrameResources`.
- A render-delegate fixture records the bake after `prepareSceneRender`. A call before TLAS update or with a different root must fail; a subsequent frame must revalidate its new TLAS.
