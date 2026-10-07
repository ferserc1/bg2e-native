# Step 01: Define standalone lifecycle

## Scope

Add StandaloneBakerContext as a distinct public type sharing only the bake core with IntegratedBakerContext. Specify initialize, resize when required, updateScene, bake and cleanup ordering. Define the scene generation/version that invalidates every affected baker history. Do not trigger lifecycle calls from integrated mode.

## Acceptance and compile gate

Headers and implementation stubs compile; invalid call order reports clear errors. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Add `StandaloneBakerContext` and `StandaloneLightmapBaker` with the exact names from [API_CONTRACT.md](../API_CONTRACT.md). The context exposes `initialize(VkExtent2D)`, `updateScene(float deltaSeconds)`, `createBaker(node, settings)` and `cleanup()`. The baker exposes synchronous `update()` plus common result methods. Reuse private bake passes; do not duplicate the integrated shading algorithms.
- Define state `Created -> Initialized -> SceneReady -> Cleaned`. `createBaker` requires a scene-ready context. `update()` requires a current scene generation. Repeated `updateScene` increments generation and resets every affected baker history. Cleanup is idempotent and cannot run while an update is recording.
- Context holds a `shared_ptr<scene::Scene>`, an Engine reference and owned frame/TLAS resources. It is not an `OffscreenApplicationDelegate` and requires no active MainLoop. Retain the context from each baker to prevent premature destruction.

## API example

```cpp
auto context = std::make_shared<render::StandaloneBakerContext>(engine, scene);
context->initialize({512, 512});
context->updateScene(0.0f);
auto baker = context->createBaker(node, settings);
baker->update();
```

## Handoff

After this step, complete [prestep_02_scene_update.md](prestep_02_scene_update.md) for the next step (Drive scene updates and own the TLAS).
