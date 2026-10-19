# M09-01 — Actual dependency inventory and deletion manifest

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** Accepted M08 review and all product evidence.

## Scope and deliverable

- `lib/include/bg2e/render/`
- `lib/src/bg2e/render/`
- `lib/include/bg2e/scene/vk/`
- `apps/`
- `examples/`
- `tests/`
- `cmake/`
- `scripts/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/render/](../../../../lib/include/bg2e/render), [lib/src/bg2e/render/](../../../../lib/src/bg2e/render), [lib/include/bg2e/scene/vk/](../../../../lib/include/bg2e/scene/vk), [apps/](../../../../apps), [examples/](../../../../examples), [tests/](../../../../tests), [cmake/](../../../../cmake), [scripts/](../../../../scripts).

## Implementation work

1. Scan includes, qualified types, aliases/macros, umbrella headers, CMake targets, tests, standalone templates, package/install/resource paths and indirect callers outside render.
2. Build a per-file remove/migrate/keep manifest distinguishing obsolete application APIs from neutral CPU functionality and still-needed GPU backends/SDK assets.
3. Cross-check every production feature against an accepted new product/API. Close required gaps before deleting its reference implementation; retain historical design documents as evidence, not maintained usage instructions.
4. Record the final public namespace/type/accessor/target naming choices and data-format compatibility expectations separately from deliberately unsupported old applications.

## Verification and exit conditions

- Every remaining render/scene::vk/legacy Drawable reference has an assigned replacement or deletion, including build/scripts/tests.
- The manifest does not schedule deletion of author assets, GPU Vulkan support or graphics-free scene/file functions.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M09-02–M09-06 execute this manifest; M10 uses its source-to-document mapping as the authority rather than the initial plan names.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M05 coexistence and surviving-product inventory

M05 adds `ExperimentalDraw` as an explicit `OffscreenApplication::init` overload;
the original production-render overload remains and is still the default execution
mode. M05-04 registers `draw_offscreen_application` alongside the existing
production examples, and M05-05 adds RGBA8 PNG export in that app. The app is an
accepted interim product, not evidence that supported entry points have already
switched to draw. Inventory both overload branches, draw Engine/offscreen surface,
the new target/resources and `tests/draw/offscreen` in M09-01. M09 must retain or
migrate the requested headless product before retiring production paths, while
removing the temporary experimental selector and making draw canonical under the
accepted M09 plan. M05's camera PNG does not replace full scene/resource IO.

## M01-16 reviewed baseline

Include actual dual DrawableComponent accessors/mode tags, production scene visitors/scene::vk, ComponentFactoryRegistry/DB overloads, strong production versus weak draw TextureCache maps, legacy TextureWidgets native public members, UserInterfaceDelegate production init, application execution branches, manual tests/gpu/tests/draw probes, templates and paired shader deployment. A matching filename does not prove a replacement is implemented or accepted. See [M01 review](../milestone_01_deferred_renderer/review.md).

## M02 retirement inventory

Add `OverlayPass`/`MeshOverlayPass`, `GizmoAndSelectionRenderer` dual init and
draw component caches, `SelectionManager`/`SelectableComponent` mode overloads,
`DrawMaterialEditor`, `DrawTextureWidgets`, `TexturePreview` UI-session registry,
draw `ViewpointAnalyzer` and the distinct draw camera helper overload to the
manifest. Map each production counterpart and caller separately; M02 supplies
the draw path but does not by itself prove production callers migrated or
regressions passed. Retain the draw preview's UI drain/unregister order and
instance gizmo cleanup before Engine release. See [M02 review](../milestone_02_manipulation/review.md).

## M03-03 shared rank-source dependency

Before deleting lib/src/bg2e/render, relocate its pure blue_noise_data.h into the utils source area and update utils/BlueNoise.cpp's private include. M03-03 deliberately shares this single authoritative precomputed rank source while leaving production BlueNoise unchanged. Preserve the bytes/metadata and the new [CPU/atlas sampling contract](../milestone_03_ray_tracing/blue-noise.md); update generation/validation script paths when retiring the legacy directory. This source-only data dependency must not be mistaken for a remaining render API/native dependency.

## M03-09 concrete integration handoff

Use [integration contracts](../milestone_03_ray_tracing/integration.md): live
requested effect settings persist independently of capability status; renderer
mode selects IBL/AO/GI separately from reflections. Existing examples inject
optional disabled FSR and SMAA. FinalProcessor/SMAAProcessor apply final scene AA
after color/reconstruction and before overlays/UI, with standalone borrowed/owned
output and retired-slot contracts. Reuse shared noise and explicit invalidation.
Legacy query shadows retain an eight-pair limit but degrade the entire scene to
neutral visibility with sceneSupported/status on overflow; advanced tables remain
independent and never truncated. Review no-RT/partial support and the manual matrix.
Keep official AreaTex/SearchTex bytes and original licenses in iryku_smaa during
production retirement: draw SMAA now consumes those resources. Do not use spatial
AA or denoised histories as bake averages. Runtime checks remain lead-owned.

## M03-10 reviewed implementation requirement

Keep lib/third_party/iryku_smaa/AreaTex.h and SearchTex.h with original licenses: draw SMAA consumes them. Inventory draw/lib tracing, temporal, denoise and SMAA native profiles plus manual fixtures before deleting production shader/build/package entries; legacy shadow capacity degradation is not advanced table parity.

Source and validation boundaries: [M03 technical review](../milestone_03_ray_tracing/review.md). Include these conditions in this step's implementation and exit checks.

## M06 model-editor coexistence inventory

The [M06 review](../milestone_06_model_edit/review.md) adds the complete
`apps/model_edit/` and `apps/draw/model_edit/` workflows, draw/production DB and
UI overload pairs, `DrawMaterialEditor`, `Uv2SafeReload::regenerateDraw`, integrated
baker ownership, viewpoint helper and `draw_model_edit` bundle/package resources
to the source inventory. Preserve both applications and their settings identities
through M08. M06-06 runtime/package evidence remains unspecified; do not treat the
source-level package rules as proof that production parity gates passed.
