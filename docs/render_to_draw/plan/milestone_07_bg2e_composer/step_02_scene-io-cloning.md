# M07-02 — Complete scene graph IO, factories and independent cloning

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M07-01 and M06 complete DB APIs.

## Scope and deliverable

- `apps/bg2e_composer/src/StageScene.cpp`
- `lib/include/bg2e/scene/Node.hpp`
- `lib/include/bg2e/scene/ComponentFactoryRegistry.hpp`
- `lib/include/bg2e/db/scene.hpp`
- `apps/draw/bg2e_composer/src/StageScene.cpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [apps/bg2e_composer/src/StageScene.cpp](../../../../apps/bg2e_composer/src/StageScene.cpp), [lib/include/bg2e/scene/Node.hpp](../../../../lib/include/bg2e/scene/Node.hpp), [lib/include/bg2e/scene/ComponentFactoryRegistry.hpp](../../../../lib/include/bg2e/scene/ComponentFactoryRegistry.hpp), [lib/include/bg2e/db/scene.hpp](../../../../lib/include/bg2e/db/scene.hpp).

## Implementation work

1. Adapt scene open/save/prefab and mesh/glTF import to draw high-level loaders and pure schema IO; preserve camera validation and error/relative-path/resource behavior.
2. Keep the existing single editable-root save/load unwrapping rule so repeated round-trips do not nest an extra root each time.
3. Update component factory/deserialization mode handling for draw DrawableComponent and independent CPU/GPU clones, including user component subclass behavior.
4. Complete GPU-only texture/resource export and scene metadata preservation using M06/M05 services; report incomplete resource saves rather than silently omit them.

## Verification and exit conditions

- Repeated save/load retains hierarchy/metadata/resource paths, and unresolved schema IO works without a graphics engine.
- Duplicated nodes independently modify CPU/GPU geometry/materials; originals, controller/component state and saved references remain coherent.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M08 prefab/context assembly relies on these root/factory/resource semantics. M09 removes legacy factory modes only after all supported files use draw/neutral paths.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-07 actual DB baseline

See [db-levels.md](../milestone_01_deferred_renderer/db-levels.md). Raw scene document IO preserves unknown JSON; typed graph IO requires registered graphics-free component parsers and serializeReferences overrides. Source references are unresolved CPU state, not a snapshot of live edits. loadResources(draw::Engine&, progress) realizes known BG2/OBJ components; the draw route never calls a production Creator. Complete resource export/import from M06 must include GPU-only contents and custom component fields; saveSceneReferences alone is insufficient. DrawableComponent cloning preserves resourcePath alongside its independent live resources.

## M01-16 reviewed baseline

Node::clone creates fresh node identifiers and DrawableComponent typed clones invoke independent CPU/GPU draw resources. Preserve those semantics and distinguish them from reference-only IO. Selectable schema/draw hooks come from M02; complete resource save remains dependent on M06. Never silently switch an existing component between Production/Draw contexts without the explicit clear boundary. See [M01 review](../milestone_01_deferred_renderer/review.md).


## M04-07 reviewed implementation requirement

Exercise scenes with marked linear RGB UV2 maps: preserve IndirectTextureMode and exported image paths, including GPU-clone independence. Empty GPU texture paths are incomplete resources, not successful serialized lightmaps. Reuse M06's completed readback/materialization and keep unmarked scalar AO semantics unchanged.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).
