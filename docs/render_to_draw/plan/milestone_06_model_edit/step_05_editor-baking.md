# M06-05 — Model UV2 preview and integrated lightmap workflow

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M06-02/M06-03 and accepted M04 APIs.

UV/topology edits must use M04's stopped-producer reload/rebuild and explicit
bake-generation invalidation. M02 selection refresh does not detect UV-only
changes after a same-topology `Drawable::reload()`, and MaterialModifier does not
notify a baker itself. Route document edits through the app's queued change
notification so editor previews, bake jobs and render histories are invalidated
consistently; see the [M02 review](../milestone_02_manipulation/review.md).

## Scope and deliverable

Correction after reported editor failure: UV2 overlap is nonfatal. Bake and safe
UV2 regeneration must use `UvAtlasOverlapPolicy::Warn`, display its warning and
retain overlapping triangles. Only structural/coordinate/geometry failures abort.
See the correction record in [progress](../progress.md).

- `apps/model_edit/src/ModelLightmapWindow.cpp`
- `lib/include/bg2e/bake/IntegratedBakerContext.hpp`
- `lib/include/bg2e/bake/UvMapPreviewRenderer.hpp`
- `apps/draw/model_edit/src/ModelLightmapWindow.cpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [apps/model_edit/src/ModelLightmapWindow.cpp](../../../../apps/model_edit/src/ModelLightmapWindow.cpp).

## Implementation work

1. Port the lightmap window/jobs to bake settings/context/pixels and configurable public draw effects, preserving target validation/progress/reset/cancel behavior.
2. Perform UV2 generation and Drawable rebuild at the coordinated waitIdle boundary, and refresh tracing state, UV previews and selected submesh references.
3. Apply completed maps to both CPU definitions and live material resources with correct RGB multiplier/UV set metadata, and persist/export through complete resource-save paths.
4. Coordinate async loading, scene locking and window/job teardown so no context borrows a replaced renderer/environment or deleted node.

## Verification and exit conditions

- Generate/preview/bake/apply/save/reload lightmaps on a model and verify live indirect toggling/baked contribution.
- Cancel/reopen/load/delete/renderer replacement during bake follows the declared safety policy and leaves UI/document state correct.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M07 scene-wide jobs and M08 standalone batch adapt these proven target/publication/export semantics; they must not copy window scheduling into CLI.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.


## M06-05 delivered implementation handoff (2026-10-19)

The draw app now provides `ModelLightmapWindow` and `EditorBaking.cpp`. Its Window
menu opens a UV/lightmap utility with UV1 and UV2 diagnostics, stopped `regenerateDraw`
UV2 replacement, selected-or-first-drawable targeting, RTAO/RTGI configuration,
RGB8/RGB32F CPU precision, chart dilation, exact sample progress, cancel, completed
GPU result preview and explicit apply. The baker records into the renderer's draw
command buffer, then calls `completeFrame` only after the app drains the submitted
work. One sample is outstanding at a time. The context borrows the engine device,
owns its baseline document root and a copied environment snapshot, and is cleaned
before renderer/stage/Engine teardown. Source edits invalidate the job; pending UV2
work is weak-target/document checked and cancels if its utility closes or target
changes. The editor excludes stage floor, fill and camera fixtures by rooting the
baker at the document graph.

Apply binds the completed GPU image to each target submesh's AO slot and updates
both live `MaterialBase` attributes and the CPU material definition with linear
sampling, UV2 and `IndirectTextureMode::BakedAmbientOcclusion` or
`BakedGlobalIllumination`. M06-02 complete model/scene export materializes the live
GPU texture as `.bg2tex`; the window does not write a detached temporary PNG.
Static source/API/lifecycle/metadata/export review and documentation checks passed.
No compilation or app/backend execution was performed. Lead checks are in
`tests/draw/editor_tools/README.md`.


## M04-07 reviewed implementation requirement

Reuse IntegratedBakerContext's one-live-target/one-open-frame contract and exact completed-sample publication. Capture result()/readPixels() before applying maps or replacing sources. Complete all targets against an immutable baseline, or explicitly invalidate and rebuild after each material adoption; the example demonstrates deferred all-target adoption. A responsive scheduler must retain commands/owners and confirm real completion; synchronous per-sample submission is the existing working policy, not an asynchronous engine service. Reject chart-unsafe denoise. Rebind previews after CPU/GPU target replacement and clear session handles before UI/Engine teardown.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).


## M06-01 job boundary

The new startup has no Document, bake context/job, UV preview or image adoption
workflow yet. Its editor floor explicitly disables ray-tracing geometry and the
stage retains drawables through cleanup. Integrate the complete legacy UV/bake
workflow from the [inventory](application-inventory.md) over public bake/draw APIs;
do not use the camera-image accumulator or scene CLI PNG as a bake scheduler.


## M06-03 edit invalidation boundary

Use [the actual editor tools](editor-tools.md): document replacement advances
generation, root duplication can replace the root with an independent group,
and queued source edits notify dirty/history changes even after partial failure.
Add explicit source/bake job invalidation to these edit notifications; document
generation alone does not change for every material or geometry edit. Sampler-only
edits retain authored GPU pixels. DrawMaterialEditor exposes AO/bakedAO/bakedGI
interpretation with explicit linear/UV2 handling and ordinary-AO reset, but does
not generate maps or implement UV preview/bake scheduling. Floor/fill/camera are
stage fixtures outside the exported document root; define bake inclusion explicitly.


## M06-04 camera baseline

The draw editor now exposes bounds framing and an explicit best-view action.
Analysis runs synchronously at a stopped renderer-update boundary, against the
selected document node or document root, with the current camera FOV and a bounded
maximum 48-pixel analysis extent with the active viewport aspect, 18-yaw by 4-pitch candidate grid. Candidate and per-submesh visibility
statistics are mapped through that analyzer instance's `analyzedSubmeshes()`
identifiers. Empty/failing/stale analysis does not apply a camera candidate. The
editor analyzer is released before Stage draw resources and Engine teardown. Keep
bake jobs independent from this measurement path.
