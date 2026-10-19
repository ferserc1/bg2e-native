# M07-05 — Scene-wide bake jobs, previews and complete saves

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M07-02–M07-04 and accepted M04/M06 bake integration.

## Scope and deliverable

Preserve the M06 overlap correction: use `UvAtlasOverlapPolicy::Warn` for bake
targets, expose the warning and keep overlapping triangles eligible for baking.
Do not restore strict overlap rejection during UV2 generation or job preflight;
malformed geometry/coordinates remain fatal.

- `apps/bg2e_composer/src/SceneLightmapWindow.cpp`
- `apps/bg2e_composer/src/AppDelegate.cpp`
- `apps/draw/bg2e_composer/src/SceneLightmapWindow.cpp`
- `lib/include/bg2e/bake/IntegratedBakerContext.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [apps/bg2e_composer/src/SceneLightmapWindow.cpp](../../../../apps/bg2e_composer/src/SceneLightmapWindow.cpp), [apps/bg2e_composer/src/AppDelegate.cpp](../../../../apps/bg2e_composer/src/AppDelegate.cpp).

## Implementation work

1. Adapt multiple target jobs to bake contexts/settings/pixels, shared public effect objects and correct target/scene-generation validation.
2. Preserve UV2 safe rebuild, preview, progress/cancel/reset and application of RGB/UV/material metadata across selected or scene-wide targets.
3. Export/load resources through complete DB paths, including temporary output/publication and unsaved state. Do not make a live renderer/context outlive its environment/scene during async import.
4. Handle unavailable RT bake hardware explicitly while preserving scene editing and no-RT raster rendering.

## Verification and exit conditions

- Several targets bake/apply/save/reload correctly without swapping material/submesh identities or losing dilation/pixel semantics.
- Cancel/import/delete/renderer replacement invalidates jobs and borrowed resources according to the reviewed lifecycle.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## M05-03/05 execution and output boundary

The headless frame driver submits serially and calls completion only after its
submission returns; its default delegate averages exactly `config.frames`
completed reconstructed camera samples in linear RGB, then performs final color
and optional SMAA once. It is not a bake target/sample scheduler, scene-generation
cancellation service, multi-target transaction or HDR lightmap writer. Reuse the
GPU-complete lifetime pattern where suitable, while retaining the reviewed
IntegratedBakerContext/job identities and explicit LightmapPixels contracts.
The M05 CLI demonstrates one PNG-suffixed temporary file, exact decode round-trip
and no-clobber publication; M07 complete-scene saves still need resource-level
preflight, rollback/protected-input rules and saved-state coordination.

## Downstream comparison

M08 standalone jobs preserve target/output identity and validation but use a windowless scheduler instead of this integrated UI path.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.


## M04-07 reviewed implementation requirement

Serial targets must release the previous IntegratedLightmapBaker before createBaker on the same context. Capture every typed output/current result before source edits; either adopt the completed batch together or rebuild generation/snapshots after each adoption. Preserve marked bakedAO/bakedGI mode and UV2 on live and stored submeshes, validate finite half-float display range separately from RGB32F export, and use the bake preview/UI bridge rather than production widgets.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).

The M06 application uses one `IntegratedBakerContext` rooted at its document and
one live baker/in-flight frame. The context borrows the initialized draw Engine;
the caller records into a normal draw frame and calls `completeFrame` only after
submission completion (or `abandonFrame` only for a discarded unsubmitted
command buffer). Composer scene jobs need explicit job/scene identity, multi-target
source invalidation and complete-result adoption around those lower-level
contracts. Do not put app-owned EditorBaking scheduling into the bake library or
reuse a UI preview handle as job output ownership. See the
[M06 review](../milestone_06_model_edit/review.md).
