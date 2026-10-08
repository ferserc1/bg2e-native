# Handoff for Assemble model and prefab scenes

Next implementation step: [step-03_input_assembly.md](step-03_input_assembly.md). This handoff records step 02 implementation findings and project-lead verification.

- Changed files and relevant API decisions: `StandaloneBakerContext::initialize()` now creates a context-owned `FrameResources`, descriptor allocator, and separate production `RayTracingScene`. `updateScene()` applies the first configured viewport resize, drives `willUpdate -> UpdateVisitor -> light/environment cache refresh (camera only when present) -> didUpdate`, then records and synchronously submits the context-owned TLAS build. It rejects an empty/non-traceable scene and waits for the build fence before declaring `SceneReady`. Generic `FrameResources::cleanup()` now tolerates partially initialized command/synchronization resources so initialization failures can be unwound safely.
- Build command, platform and result: The project lead confirmed standalone step 02 compilation passed; command and platform details were not supplied. The agent did not compile.
- Runtime/fixture evidence: The project lead confirmed the headless moving-component fixture passed; the component and transform values were not supplied.
- Remaining limitations or regressions: `StandaloneLightmapBaker::update()` remains the explicit stub from step 01; bake submission is implemented by later standalone-baking steps. `updateScene()` requires at least one loaded, ray-tracing-enabled, visible, non-transparent instance and leaves the context in `Initialized` after failures.
- Resources, ownership and synchronization cautions for the next agent: Attach context and target geometry before the one initial `updateScene()`; an empty TLAS is an explicit error. Repeated scene updates wait for device idle, clear the standalone descriptor pools, clean the previous context-owned TLAS before rebuilding, and reset all live baker accumulation generations. The standalone TLAS is distinct from `_frameResources->rayTracingScene` and `FrameResources::flushFrameData()` is not used. The build command buffer is submitted on the engine graphics queue and its fence is waited synchronously; the existing `RayTracingScene::update()` records the build-to-ray/compute barrier. No draw hooks or camera are required.
- Exact next action: Implement `step-03_input_assembly.md`, loading context and model/prefab target geometry into the retained scene before `updateScene()` so all intended occluders and targets enter its single TLAS.

## Next-step instructions

Provide an input assembly path for a JSON lighting context plus either one .bg2 model at the origin or a JSON prefab subtree. Attach the target nodes to the context scene before updating it, so all context and target geometry participate in the TLAS and inter-occlude. Validate missing assets and ownership; preserve input files untouched.

## Next-step acceptance gate

Both modes assemble a single scene and enumerate the intended target Drawables. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Add `render::StandaloneBakeSceneAssembler` in the engine, using existing `bg2e::db` loaders. Load the context JSON as a `scene::Scene`, then import either a .bg2 model node or a prefab JSON subtree and attach it **before** calling `updateScene`. Model mode applies identity transform at the world origin. Prefab mode preserves the prefab hierarchy and local transforms. The CLI itself is not created until the penultimate step.
- Provide a two-stage target path: obtain target CPU meshes and apply the already available phase-2 `geo::GenerateUv2AtlasModifier` when requested, then load/reload their GPU meshes and BLAS before the sole `updateScene`/TLAS build. If a current db loader uploads during import, add a CPU-only loading path or an explicit safe reload stage; never mutate UV2 after the final GPU load without rebuilding BLAS. Context geometry and every prefab target must enter the same completed scene before TLAS creation.
- Traverse the attached target subtree in deterministic preorder. The target list contains only nodes directly owning standard Drawables; context-only nodes are occluders/lighting sources, never export targets. All context and prefab/model geometry enters the same TLAS. Detect duplicate node identities and invalid source files early.
- Retain input source paths and output target identities separately; never mutate or save input JSON/.bg2 while assembling. Reject a prefab that cannot be attached cleanly; surface path and node in diagnostics.
