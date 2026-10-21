# M08 — Standalone lightmap_generator on draw/bake

[Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Review protocol](../review-protocol.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Entry prerequisites:** Accepted M04–M07 bake/pixel/scene/headless application reviews and current CLI feature inventory.

M02's viewport selection, `SelectionItem` IDs, UI preview handles and overlays
are editor-only lifetimes. Standalone assembly/batch must own bake-target and
source generations, refresh geometry/material snapshots after UV2 reload, and
exclude manipulation tool meshes from export/tracing. Loaded draw clone support
does not imply complete GPU-authored image export. See [M02 review](../milestone_02_manipulation/review.md).

## Working product

The [M07 review](../milestone_07_bg2e_composer/review.md) supplies accepted scene
root/clone/parser/complete-IO and app-job contracts. M07-08 documentation acceptance
still gates entry. Composer's serial integrated batches and per-item exports do
not implement standalone scheduling or protected whole-batch publication. The
lead checked the selection correction; other backend/package detail is unspecified.

A runnable `apps/draw/lightmap_generator` uses windowless draw/GPU execution and public bake StandaloneBakerContext/SceneAssembler/Batch/output resources. It preserves required model/prefab map-generation, UV2 and accumulation workflows without a viewport renderer or production Engine; stricter required-target failure, typed HDR and complete-generation publication are explicit behavior changes recorded in product-validation.md.

## Scope exclusions

- No reuse of integrated window frame scheduling as a substitute for standalone context state.
- No move of UV2 generation from geo or duplicate rendering-effect algorithms inside the app.
- No retained production CLI dependency in the new executable; the original target remains reference until M09.

## Ordered steps

| Step | Deliverable |
|---|---|
| [M08-01](step_01_standalone-context.md) | Public standalone bake context and scene generation |
| [M08-02](step_02_scene-assembly.md) | Bake model/prefab/context assembly and UV2 |
| [M08-03](step_03_batch-output.md) | Bake batch/output writer and protected complete exports |
| [M08-04](step_04_cli-application.md) | Build apps/draw/lightmap_generator and configuration flow |
| [M08-05](step_05_product-validation.md) | Standalone CLI parity, multiple targets and deployment |
| [M08-06](step_06_review.md) | Review M08 and prepare exhaustive production removal |
| [M08-07](step_07_documentation.md) | Update maintained documentation cumulatively through M08 |

## Milestone exit gate

- The final [documentation step](step_07_documentation.md) updates maintained `doc/` through this milestone and is accepted before the next milestone; no future implementation is documented as available.

- Standalone preparation/generation/update/cleanup and target validation work through bake over headless draw/GPU resources.
- Model/prefab/context assembly, UV2 and scene/material/environment/light capture match actual existing behavior.
- Bake-specific batch/output orchestration lives in bake; generic file/image/scene IO remains in DB.
- Byte/float maps and optional model copies preserve metadata, dilation and protected-input/preflight behavior.
- CLI settings/configuration/backend/errors/output contracts are tested; unsupported required RT baking fails explicitly.
- M08 review updates the exhaustive removal gate and final documentation plan.

Provide the project lead with applicable Vulkan/Metal, no-RT and SDK-unavailable checks. The lead compiles/tests after each step; the agent performs static inspection and does not run builds/runtime checks unless explicitly instructed. Their instruction to continue accepts the preceding delivered step under the [execution protocol](../execution-protocol.md), without repeated confirmation or mandatory test artifacts. Record who checked what and leave unspecified coverage unspecified. Preserve production regression references through M08.

## Required code-versus-plan review

The technical review is the penultimate step; the cumulative documentation update is the final required step. Create `review.md` only after execution with actual revisions/contracts/results. Compare the code against every pending milestone and update affected summaries and step files, including dependencies, paths and validation. The review step lists milestone-specific comparisons; the documentation step closes the milestone under the cumulative documentation policy. Do not rewrite completed history to conceal a design change.

## M01 review handoff

Depend on actual M05 windowless context and M03/M04 reusable effects/bake profiles, not private RendererDeferred targets. Batch/output/preflight/atomic save semantics are still absent from M01 raw-reference IO and raw transfer. See [M01 review](../milestone_01_deferred_renderer/review.md) for source evidence, classifications and verification limits. Milestone order and product gate remain unchanged.

## Documentation closure order

The technical review now hands off to [M08-07](step_07_documentation.md), not directly to the next milestone. Earlier next-milestone handoff statements are historical and superseded by this order. See [documentation policy](../documentation-policy.md).

## M03-01 tracing checkpoint dependency

The [advanced tracing design](../milestone_03_ray_tracing/rt-design.md) was accepted by project-lead continuation. [M03-02](../milestone_03_ray_tracing/rt-gpu-data.md) delivers primitive arrays/stages; the draw snapshot builder and integrated bake primitives are implemented. Reuse the public draw::pipeline tracing snapshot, packed geometry/material data, bounded image/sampler resources and shared cutout predicate. Supply explicit UV-space world-position/normal/coverage surfaces without a camera RendererDeferred. Check effective capacities and required tracing support before a job; invalidate scene/baker generations after material/texture/topology edits. Keep bake sample accumulation separate from rendered-view histories and FSR state. Do not duplicate a private table/shading family or silently bake with truncated/unsupported resources.

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


## M05 headless reuse boundary

The generic draw Engine/offscreen surface and serial completed-frame lifecycle are implemented in M05. Reuse them where appropriate for windowless ownership, but M08 still supplies standalone bake job state, scene assembly, UV-specific sampling/coverage, typed LightmapPixels and complete-batch preflight/publication. The M05 exact-N camera average and verified RGBA8 PNG are separate products and do not establish bake batch semantics.

## Current engine preservation requirements — 2026-10-21

Preserve R01–R14 from the source reconciliation: alpha/default/import behavior,
material and native-binding revisions, persistent table/acceleration generations,
completion-aware pools, exact temporal invalidation, conservative transparent
regions, narrow typed update drain elision, CPU/GPU profiling and current complete
save error/publication semantics. Do not reimplement an older version of these
features to satisfy an original milestone description.

Read the [source reconciliation](../code-reconciliation-2026-10-21.md); current engine code overrides historical implementation assumptions. No native/performance result is implied by this source baseline.

## M08-01 delivery

[Standalone context contract](standalone-context.md) implements the headless serial
scheduler over public bake primitives, with explicit scene generations, one live
target, synchronous completed samples and stale/cancelled/failed publication guards.
M08-01 is accepted by lead continuation; backend/scenario coverage is unspecified.
Assembly and batch output are delivered below; CLI remains pending. Existing
optimized draw/gpu implementations are unchanged.

## M08-02 delivery

[Standalone assembly](scene-assembly.md) now prepares independent camera-less
model/prefab/context candidates with shared schema/factory/CPU parsers, complete
root ownership, enabled-target/source identities and staged independent UV2.
Optional target GPU loading and overlap warnings are explicit; context geometry
is source-only. M08-03 must complete dependency protection/preflight and output
publication, including OBJ MTL/custom-resource discovery gaps (completed below).
No runtime matrix is inferred; M08-02 is accepted by lead continuation.

## M08-03 delivery

[Standalone batch/output](batch-output.md) implements serial frozen jobs,
protected file/component inventories, RGB8/exact RGB32F encoding, independent
complete model copies and a final authoritative manifest pointing to a complete
unique generation. Previous generations are preserved; cancellation/failure never
publishes partial success. Production writer/native rendering algorithms and
CMake remain unchanged. M08-02 is accepted by continuation (backend coverage
unspecified); M08-03 is accepted by lead continuation; backend/scenario coverage is unspecified.
M08-04 delivery follows below.

## M08-04 delivery

[Draw CLI](cli-application.md) adds draw_lightmap_generator with model/prefab
arguments, typed live bake configuration and CLI-last overrides, backend choice,
AO/GI typed outputs, UV2/copy controls and deterministic cancellation/error status.
It reuses OffscreenApplication plus standalone assembly/batch without camera
rendering. Existing production CLI/native algorithms remain unchanged; only the
new app target and its registration change CMake. Shared paired shader refresh
and distribution discovery remain authoritative. M08-03 is accepted by
continuation; M08-04 awaits lead validation. Next is M08-05 product validation.

## M08-05 delivery

[Product validation record](product-validation.md) compares actual legacy/new
workflows and documents deliberate stricter failure, output-layout and typed HDR
changes. A read-only independent PNG/native-map/BG2-material inspector has ten
passing synthetic CPU tests; actual native output, geometry reload, deployment
and optimized regression checks remain lead-owned with explicit P01–P11 status.
M08-04 is accepted by continuation, backend/scenario coverage unspecified. No
native/production/CMake implementation changes or engine/build/package execution
were made in M08-05. Next is M08-06 review after lead acceptance.

## M08-06 technical review delivery

[Review](review.md) records actual standalone states/lifetimes/source generations,
assembly/UV2, public pass/settings ownership, typed writer/manifest/clone behavior,
CLI/config/deployment identities, product status and R01–R14 preservation.
[Retirement impact](removal-impact.md) provides 1338 per-file candidates including
316 direct textual legacy matches; these are not automatic deletions. Shared
transitive headers, native UI seams, templates, specialized legacy writer and
final umbrella/accessor decisions are assigned to concrete M09 steps. Pending
M10 audits and root contracts now use actual M08 source. No code/native algorithm,
shader/CMake implementation or production source is changed by the review.
M08-05 is accepted by continuation; hardware/scenario details remain unspecified.
Next after review acceptance is M08-07 cumulative maintained documentation, not M09.

## M08-07 documentation delivery

[Coverage/source ledger](documentation.md) records 9 new and 22 updated maintained
pages for actual standalone APIs, generic file/archive IO, C++ tutorial, complete
CLI/config/resource/publication guide and architecture. Current save bool/parents/
hardlink fallback and production/new writer distinctions are corrected; indexes
and affected cumulative guidance are connected. No source/CMake/shader change or
native execution occurred. M08-06 is accepted by continuation; M08-07 awaits lead
acceptance. Next is M09-01 inventory under the normal transition gate, with the
review's original evidence limits and production source retained until retirement.
