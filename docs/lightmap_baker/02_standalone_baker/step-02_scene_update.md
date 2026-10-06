# Step 02: Drive scene updates and own the TLAS

## Scope

Implement headless scene/component initialization and update, transform/light/environment collection, owned frame/descriptor resources and RayTracingScene::update recording. Support explicit updateScene before any number of target bakes. Define synchronization before ray use and cleanup on errors. Verify behavior for scene components that modify transforms or scene contents.

## Acceptance and compile gate

A headless fixture can prepare a complete scene and valid TLAS without a windowed render loop. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Initialize owned `vulkan::FrameResources`/descriptor allocator and `render::vulkan::rt::RayTracingScene` using headless `Engine::init()`. Distinguish engine ownership (caller) from context ownership (frame and TLAS). Allocate and destroy command resources in matched order.
- On first preparation, call `Scene::willResize()` and `didResize()` around the context's image resize. On every `updateScene(deltaSeconds)`, call `willUpdate()`, `UpdateVisitor::update(root, deltaSeconds)`, refresh light/environment caches (and camera only if one exists), then `didUpdate()`. Include components added/removed by an update in the final TLAS traversal. Do not require a camera merely for baking.
- Collect all scene nodes, materials, transforms and lights before calling the context-owned `RayTracingScene::update(cmd, root)`. Record a barrier for subsequent compute/ray stages and wait for completion before declaring `SceneReady`. Before a later `updateScene`, wait for all previous bake submissions, then clean the old TLAS before `RayTracingScene::update` replaces its handles. A second update after a transform change must move the corresponding TLAS instance without leaking or freeing resources in flight. Do not call these lifecycle methods from `IntegratedBakerContext`.
- If reused engine components require `willDraw/didDraw`, call the pair only around their actual draw work and document the reason; never use the screen renderer's camera-dependent `prepareSceneRender` as a shortcut.

## Verification fixture

A component whose `update` moves a node must be reflected in `objectInstances()` after `updateScene` without an application loop.

## Handoff

After this step, complete [prestep_03_input_assembly.md](prestep_03_input_assembly.md) for the next step (Assemble model and prefab scenes).
