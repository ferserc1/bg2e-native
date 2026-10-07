# Handoff for Assemble model and prefab scenes

Next implementation step: [step-03_input_assembly.md](step-03_input_assembly.md). Complete this file **after** finishing step 02; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-03_input_assembly.md.

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
