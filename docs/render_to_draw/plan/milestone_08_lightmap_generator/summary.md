# M08 — Standalone lightmap_generator on draw/bake

[Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Review protocol](../review-protocol.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Entry prerequisites:** Accepted M04–M07 bake/pixel/scene/headless application reviews and current CLI feature inventory.

M02's viewport selection, `SelectionItem` IDs, UI preview handles and overlays
are editor-only lifetimes. Standalone assembly/batch must own bake-target and
source generations, refresh geometry/material snapshots after UV2 reload, and
exclude manipulation tool meshes from export/tracing. Loaded draw clone support
does not imply complete GPU-authored image export. See [M02 review](../milestone_02_manipulation/review.md).

## Working product

A runnable `apps/draw/lightmap_generator` uses windowless draw/GPU execution and public bake StandaloneBakerContext/SceneAssembler/Batch/output resources. It matches the current CLI map-generation, target validation, UV2, accumulation, output and overwrite behavior without a viewport renderer or production Engine.

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

The [advanced tracing design](../milestone_03_ray_tracing/rt-design.md) was accepted by project-lead continuation. [M03-02](../milestone_03_ray_tracing/rt-gpu-data.md) delivers primitive arrays/stages; the draw snapshot builder and bake infrastructure remain pending. After M03 implementation/review, reuse the public draw::pipeline tracing snapshot, packed geometry/material data, bounded image/sampler resources and shared cutout predicate. Supply explicit UV-space world-position/normal/coverage surfaces without a camera RendererDeferred. Check effective capacities and required tracing support before a job; invalidate scene/baker generations after material/texture/topology edits. Keep bake sample accumulation separate from rendered-view histories and FSR state. Do not duplicate a private table/shading family or silently bake with truncated/unsupported resources.

## M03 reviewed dependency

[M03 technical review](../milestone_03_ray_tracing/review.md) is the actual source/ownership/signal/settings baseline. Its pending-plan impact table and the concrete additions in this milestone's step files supersede historical statements that explicit profiles or effect controls are still unimplemented. Keep exact averaging, chart isolation, required-job capabilities and actual validation evidence distinct from interactive fallback/history behavior.
