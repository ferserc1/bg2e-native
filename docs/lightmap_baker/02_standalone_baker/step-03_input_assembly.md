# Step 03: Assemble model and prefab scenes

## Scope

Provide an input assembly path for a JSON lighting context plus either one .bg2 model at the origin or a JSON prefab subtree. Attach the target nodes to the context scene before updating it, so all context and target geometry participate in the TLAS and inter-occlude. Validate missing assets and ownership; preserve input files untouched.

## Acceptance and compile gate

Both modes assemble a single scene and enumerate the intended target Drawables. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Add `render::StandaloneBakeSceneAssembler` in the engine, using existing `bg2e::db` loaders. Load the context JSON as a `scene::Scene`, then import either a .bg2 model node or a prefab JSON subtree and attach it **before** calling `updateScene`. Model mode applies identity transform at the world origin. Prefab mode preserves the prefab hierarchy and local transforms. The CLI itself is not created until the penultimate step.
- Traverse the attached target subtree in deterministic preorder. The target list contains only nodes directly owning standard Drawables; context-only nodes are occluders/lighting sources, never export targets. All context and prefab/model geometry enters the same TLAS. Detect duplicate node identities and invalid source files early.
- Retain input source paths and output target identities separately; never mutate or save input JSON/.bg2 while assembling. Reject a prefab that cannot be attached cleanly; surface path and node in diagnostics.

## Fixture

A two-module sofa prefab must let module A occlude module B during GI evaluation within the complete context scene.

## Handoff

After this step, complete [prestep_04_image_formats.md](prestep_04_image_formats.md) for the next step (Add image-format discovery in db).
