# M06 — Experimental model_edit application

[Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Review protocol](../review-protocol.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Entry prerequisites:** Accepted M01–M05 products/reviews; inventory the current application before copying behavior.

M02 establishes `DrawMaterialEditor`/`DrawTextureWidgets`, typed weak selection,
optional output overlays and a synchronous draw viewpoint analyzer. It does not
make generic reflected scene/component mutation safe: M06 must provide its own
queued weak-target and scene/resource-generation checks, unsaved-state and
history notifications, and teardown ordering. See [M02 review](../milestone_02_manipulation/review.md).

## Working product

A runnable `apps/draw/model_edit` reproduces the current model editor using draw, reflected graphics options, manipulation/material APIs, utils viewpoint tools and bake UV/lightmap resources. The existing apps/model_edit stays functional as reference until M09.

## Scope exclusions

- No modification of legacy ViewpointAnalyzer behavior, no generalized ResourceManager or default mesh instancing.
- No premature removal of production application paths/overloads.
- No experimental-only undocumented reductions in editor features: omissions become explicit incomplete gates.

## Ordered steps

| Step | Deliverable |
|---|---|
| [M06-01](step_01_application-copy.md) | Inventory and create apps/draw/model_edit target |
| [M06-02](step_02_complete-db-io.md) | Complete engine-aware model/scene resource load and save |
| [M06-03](step_03_editor-tools.md) | Stage, selection, materials and widget migration |
| [M06-04](step_04_viewpoint-camera.md) | Utils viewpoint analysis and camera helpers in editor |
| [M06-05](step_05_editor-baking.md) | Model UV2 preview and integrated lightmap workflow |
| [M06-06](step_06_product-validation.md) | Model editor parity and application packaging checks |
| [M06-07](step_07_review.md) | Review M06 application APIs and revise composer/CLI/retirement |
| [M06-08](step_08_documentation.md) | Update maintained documentation cumulatively through M06 |

## Milestone exit gate

- The final [documentation step](step_08_documentation.md) updates maintained `doc/` through this milestone and is accepted before the next milestone; no future implementation is documented as available.

- The new application has its own target/bundle/identity and starts without constructing a production engine.
- Model/import/resource IO and complete saves, including generated GPU-only textures, work through the two DB levels.
- Selection/submesh edits/MaterialModifier/texture previews, graphics settings, camera framing and safe mesh operations match current useful behavior.
- UV2, preview, integrated lightmaps/progress/cancel/application/export and unsaved-document behavior work.
- Production model_edit remains usable and M06 review adapts composer, CLI and retirement steps.

Provide the project lead with applicable Vulkan/Metal, no-RT and SDK-unavailable checks. The lead compiles/tests after each step; the agent performs static inspection and does not run builds/runtime checks unless explicitly instructed. Their instruction to continue accepts the preceding delivered step under the [execution protocol](../execution-protocol.md), without repeated confirmation or mandatory test artifacts. Record who checked what and leave unspecified coverage unspecified. Preserve production regression references through M08.

## Required code-versus-plan review

The technical review is the penultimate step; the cumulative documentation update is the final required step. Create `review.md` only after execution with actual revisions/contracts/results. Compare the code against every pending milestone and update affected summaries and step files, including dependencies, paths and validation. The review step lists milestone-specific comparisons; the documentation step closes the milestone under the cumulative documentation policy. Do not rewrite completed history to conceal a design change.

## M01 review handoff

Start from draw_deferred_renderer/GraphicsSettingsWindow and actual typed resources/loadResources. Complete glTF/MTL/full edited/GPU-only saves and inventory each editor widget. Scalar graphics settings and HDR ResourcePicker do not establish preview/material-editor parity. See [M01 review](../milestone_01_deferred_renderer/review.md) for source evidence, classifications and verification limits. Milestone order and product gate remain unchanged.

## Documentation closure order

The technical review now hands off to [M06-08](step_08_documentation.md), not directly to the next milestone. Earlier next-milestone handoff statements are historical and superseded by this order. See [documentation policy](../documentation-policy.md).

## M03 reviewed dependency

[M03 technical review](../milestone_03_ray_tracing/review.md) is the actual source/ownership/signal/settings baseline. Its pending-plan impact table and the concrete additions in this milestone's step files supersede historical statements that explicit profiles or effect controls are still unimplemented. Keep exact averaging, chart isolation, required-job capabilities and actual validation evidence distinct from interactive fallback/history behavior.


## M04-06 material/application handoff

The [integrated example](../../../../examples/draw/12_lightmaps/README.md) now
uses explicit MaterialAttributes::IndirectTextureMode (ao/bakedAO/bakedGI in
JSON), linear RGB UV2 maps through the AO slot and an HDR G-buffer multiplier.
Original scalar AO and a baked multiplier do not coexist in the same six-slot
material. Actual image export is still required for GPU-only baked sources;
sampler settings/empty paths alone cannot round-trip a lightmap. Preserve
RGB32F HDR and explicit mode/UV2 metadata in editor/export/batch integration.
See [material API](../../../../doc/api/draw/BakedIndirectMaps.md). Backend visual
validation is delegated to the lead; the upcoming M04 review reconciles the full
pending-consumer impact.


## M04 reviewed dependency

The [M04 technical review](../milestone_04_lightmaps/review.md) records actual
integrated bake/sample/result ownership, RGB UV2 material semantics, preview
implementation and native shader deployment. Its impact table and concrete
pending-step requirements supersede assumptions that M04 already implements a
headless scheduler, batch writer, full GPU-only resource export or camera-image
averaging. Continuation acceptance does not establish hardware-specific numeric,
performance or exported-file evidence. Preserve production references until M09.


## M05 headless export dependency

M05-05 supplies completed RGBA8 camera-image readback and a single verified PNG example, not a complete model/scene resource-save API. M06-02 owns GPU-only texture materialization, format/mip/sampler metadata and full model/scene reference publication. Use the M05 raw synchronous transfer and DB codec only where their format contracts match; preserve HDR resources and explicitly convert unsupported formats.


## M06-01 startup handoff

The registered target is draw_model_edit with native-free AppDelegate/StageScene/EditorUi
and typed draw execution. [Application inventory](application-inventory.md) maps all
production document/stage/menu/widget/bake workflows and renderer/material/UI call
references to M06-02–06. Startup shows a procedural fixture and settings; full file
IO, materials/selection/environment controls, viewpoint analysis and baking were
subsequent required steps. M06-04 now supplies explicit camera framing and best-view
analysis; M06-05 owns bake integration. The original model_edit application is preserved.


## M06-02 complete resource IO handoff

[Resource IO implementation](resource-io.md) records actual load/save APIs,
GPU-only HDR/mip archives, custom hooks, document adoption/dirty/error behavior,
publication and static evidence. Complete save is explicit-context and distinct
from reference-only IO. The draw editor now opens/imports/drops and saves models
or graph documents, with unsaved prompts. Tools, framing and bake remain M06-03–05.
Compilation and the manual resource/editor matrix await project lead validation.


## M06-03 editor tools handoff

[Editor tool implementation](editor-tools.md) records actual selection/overlay,
queued node/geometry/material edits, independent clones, sampler and baked-map
interpretation controls, environment/light/preset flows, shared input capture fix,
dirty/IO ordering and teardown. M06-04 adds stopped bounds framing and scored draw
viewpoint analysis; see [camera handoff](viewpoint-camera.md). [Application guide](../../../../doc/apps/draw_model_edit.md)
and unexecuted manual probe/checks accompany the source. A project-lead UI parity
correction now replaces the floating dashboard with the production Workspace
composition. The project lead accepted the correction by continuation. M06-04
camera analysis is also accepted by continuation. M06-05 now adds integrated UV2
and lightmap workflows; M06-06 retains cumulative product/runtime verification.


## M06-05 UV and integrated lightmap handoff

`apps/draw/model_edit/src/ModelLightmapWindow.cpp` adds a menu-launched UV and
Lightmap Baker utility with UV1/UV2 diagnostic previews, safe UV2 regeneration,
AO/GI settings, exact submitted-sample progress, cancellation, completed-result
preview and explicit application to target submesh materials. `EditorBaking.cpp`
owns the integrated bake context, retires completed frames before advancing,
invalidates jobs on document-source edits/replacement, and applies the shared GPU
image to live material textures and CPU definitions using linear sampling, UV2 and
the selected baked interpretation. Existing M06-02 complete scene/model export
publishes that GPU image as a native texture sidecar. The document graph is the
bake root; editor floor/fill/camera fixtures are excluded. Static evidence and
project-lead checks are in the updated application guide and editor matrix. No
build or runtime execution was performed by the agent. Next is M06-06 product
validation.


## M06-06 product validation handoff

Static target and distribution inspection is recorded in
[product-validation.md](product-validation.md). The draw editor uses the common
SDL bundle helper; the distribution target collector recursively includes
registered apps and installs runtime dependencies, shaders and assets. This only
confirms the configured packaging path at source level. Native app launch, feature
parity, package script execution and packaged runtime remain unverified and are
assigned to the project lead. M06-06 was accepted by continuation without backend
or scenario details. The [M06-07 technical review](review.md) compares actual
application/resource contracts and updates M07–M10 handoffs. M06-08 cumulative
documentation is the remaining milestone step; M06 remains open until it is
accepted, and no M07 work is authorized by this review alone.


## M06-08 cumulative documentation handoff

[Documentation inventory](documentation.md) maps maintained application, tutorial,
API, settings, reflection, architecture and deployment coverage to delivered source.
It includes the accepted post-review editor corrections and the lead's explicit
zero-light rendering result. M06-08 is delivered for acceptance; the next step is
[M07-01](../milestone_07_bg2e_composer/step_01_application-copy.md), gated on that
acceptance. No Composer implementation is part of this documentation step.
