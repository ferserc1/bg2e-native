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

## M05-05 camera export boundary

The accepted M05 product reads one completed camera surface, whose CLI explicitly
uses R8G8B8A8_UNORM, verifies its PNG dimensions and RGBA bytes, then publishes a
single non-overwriting file. Generic `gpu::Image::readPixels` is synchronous,
full-subresource raw-format transfer; it is not scene serialization. M07 must
materialize GPU-only texture content in its own resource-save flow, preserving
formats/mips/samplers/material metadata and relative resource paths. Reuse the
appropriate typed transfer and DB encoders, not the camera CLI's RGBA8 PNG policy.
Reference the headless app only for completion/lifetime discipline; it does not
make reference-only `saveSceneReferences` a complete save.

## M01-16 reviewed baseline

Node::clone creates fresh node identifiers and DrawableComponent typed clones invoke independent CPU/GPU draw resources. Preserve those semantics and distinguish them from reference-only IO. Selectable schema/draw hooks come from M02; complete resource save remains dependent on M06. Never silently switch an existing component between Production/Draw contexts without the explicit clear boundary. See [M01 review](../milestone_01_deferred_renderer/review.md).


## M04-07 reviewed implementation requirement

Exercise scenes with marked linear RGB UV2 maps: preserve IndirectTextureMode and exported image paths, including GPU-clone independence. Empty GPU texture paths are incomplete resources, not successful serialized lightmaps. Reuse M06's completed readback/materialization and keep unmarked scalar AO semantics unchanged.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).


## M06-02 implemented resource publication baseline

Reuse the [complete draw IO contract](../milestone_06_model_edit/resource-io.md):
explicit engine-aware scene/model save snapshots native GPU color mips/layers and
publishes unique resources before replacing the main file. `.bg2tex` preserves
HDR and requires draw texture decoding; raw/schema/reference IO still exports no
resources. Custom schema/reference/draw hooks preserve component fields; custom
external-resource types require an explicit export contract. Do not route this
through production image loaders. Format-specific batch image outputs/conversions
remain this consumer's responsibility; native archive support does not implement
the pending batch writer or scheduler. Preserve previous input generations and
mark document/batch success only after output publication.

The M06 review additionally confirms distinct explicit-engine draw overloads:
`loadScene(path, draw::Engine&, progress)`, complete `saveScene(root,
draw::Engine&, path, progress)`, and `storeDrawableBg2(path, draw::Engine&,
node)`. Use raw/schema/reference overloads only for their CPU/unknown-field
contracts. Native `.bg2tex` v1 preserves GPU color mip/layer bytes and format but
requires little-endian host data and draw decoding. OBJ/MTL and glTF realization
remain the documented implemented subsets. Custom external resources still need
component-owned serializers; they are not inferred from a cloned live texture.
See [M06 review](../milestone_06_model_edit/review.md).

## M07-01 actual application handoff

The [application inventory](application-inventory.md) records the new app-owned
M06 baseline. StageScene initializes Document::root to a `Scene` group containing
the startup cube; camera/floor/fill remain container fixtures. Current Open/glTF/OBJ
actions replace that document, not insert subtrees. Finish Composer insertion,
selected-model export, new/close and active-camera/projection behavior here. Audit
the inherited single-child unwrapping rule with root components/metadata and
repeated saves; do not infer full factory/component parity from the baseline.
The copy's independent clone and complete-save calls are actual engine services,
not a replacement for these Composer regressions.
