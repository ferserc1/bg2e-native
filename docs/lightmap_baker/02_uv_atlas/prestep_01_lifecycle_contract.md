# Handoff for Define standalone lifecycle

Next implementation step: [../03_standalone_baker/step-01_lifecycle_contract.md](../03_standalone_baker/step-01_lifecycle_contract.md). Complete this file **after** finishing UV-atlas step 07; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in ../03_standalone_baker/step-01_lifecycle_contract.md.

## Next-step instructions

Add StandaloneBakerContext as a distinct public type sharing only the bake core with IntegratedBakerContext. Specify initialize, resize when required, updateScene, bake and cleanup ordering. Define the scene generation/version that invalidates every affected baker history. Do not trigger lifecycle calls from integrated mode.

## Next-step acceptance gate

Headers and implementation stubs compile; invalid call order reports clear errors. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Add `StandaloneBakerContext` and `StandaloneLightmapBaker` with the exact names from [API_CONTRACT.md](../API_CONTRACT.md). The context exposes `initialize(VkExtent2D)`, `updateScene(float deltaSeconds)`, `createBaker(node, settings)` and `cleanup()`. The baker exposes synchronous `update()` plus common result methods. Reuse private bake passes; do not duplicate the integrated shading algorithms.
- Define state `Created -> Initialized -> SceneReady -> Cleaned`. `createBaker` requires a scene-ready context. `update()` requires a current scene generation. Repeated `updateScene` increments generation and resets every affected baker history. Cleanup is idempotent and cannot run while an update is recording.
- Context holds a `shared_ptr<scene::Scene>`, an Engine reference and owned frame/TLAS resources. It is not an `OffscreenApplicationDelegate` and requires no active MainLoop. Retain the context from each baker to prevent premature destruction.
