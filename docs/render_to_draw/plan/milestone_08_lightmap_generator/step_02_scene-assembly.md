# M08-02 — Bake model/prefab/context assembly and UV2

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M08-01 and accepted M06/M07 DB contracts.

## Scope and deliverable

- `lib/include/bg2e/render/StandaloneBakeSceneAssembler.hpp`
- `lib/src/bg2e/render/StandaloneBakeSceneAssembler.cpp`
- `lib/include/bg2e/bake/StandaloneBakeSceneAssembler.hpp`
- `lib/include/bg2e/geo/GenerateUv2AtlasModifier.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/render/StandaloneBakeSceneAssembler.hpp](../../../../lib/include/bg2e/render/StandaloneBakeSceneAssembler.hpp), [lib/src/bg2e/render/StandaloneBakeSceneAssembler.cpp](../../../../lib/src/bg2e/render/StandaloneBakeSceneAssembler.cpp), [lib/include/bg2e/geo/GenerateUv2AtlasModifier.hpp](../../../../lib/include/bg2e/geo/GenerateUv2AtlasModifier.hpp).

## Implementation work

1. Reimplement assembly in bake using pure schema/file IO plus high-level draw resource load where needed. Preserve context/model/prefab root rules, target/source identities and optional load-target-resource behavior.
2. Keep UV2 atlas generation/validation in geo, at the same coordinated waitIdle/rebuild boundary as editor/bake examples.
3. Capture complete geometry/material/environment/light inputs without viewport tool/gizmo contamination; preserve joints/metadata and required target eligibility rules.
4. Do not create a parallel parser or ResourceManager; reuse actual component factory/load/save contracts from application milestones.

## Verification and exit conditions

- Model and prefab batches select the intended targets and stable output identities, with correct source paths and UV2 metadata.
- Missing resources/invalid context/unbakeable targets fail with meaningful diagnostics before partial output publication.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M08-03 preflight protects these source identities and writes model copies. M09 neutral DB/factory contracts must retain their final resource readiness behavior.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-07 actual DB baseline

[db-levels.md](../milestone_01_deferred_renderer/db-levels.md) distinguishes raw document/reference IO from resource realization. Headless assembly may read references without a graphics context, then loadResources on its intended draw Engine. Unknown custom components require explicit schema/constructor support or raw document handling. Batch outputs must use the complete engine-aware resource export implemented in M06/M07; saveSceneReferences does not write baked images or GPU-authored material contents.

The [M02 review](../milestone_02_manipulation/review.md) confirms viewport gizmos
are optional scene components with separate instance-owned GPU drawables and
must not enter the bake scene's render queue/TLAS or output model. Selection IDs
are transient editor identity, not stable batch target identity. UV-only edits
and same-topology reloads need explicit bake-generation invalidation even when
Selectable IDs remain valid; topology replacement additionally refreshes
selection/resource mappings.

## M03-01 tracing checkpoint dependency

The [advanced tracing design](../milestone_03_ray_tracing/rt-design.md) was accepted by project-lead continuation. [M03-02](../milestone_03_ray_tracing/rt-gpu-data.md) delivers primitive arrays/stages; the draw snapshot builder and bake infrastructure remain pending. After M03 implementation/review, reuse the public draw::pipeline tracing snapshot, packed geometry/material data, bounded image/sampler resources and shared cutout predicate. Supply explicit UV-space world-position/normal/coverage surfaces without a camera RendererDeferred. Check effective capacities and required tracing support before a job; invalidate scene/baker generations after material/texture/topology edits. Keep bake sample accumulation separate from rendered-view histories and FSR state. Do not duplicate a private table/shading family or silently bake with truncated/unsupported resources.

## M04-02 actual replacement and surface inputs

Reuse [implemented UV surfaces/reload](../milestone_04_lightmaps/uv-surfaces.md). generateUv2AndReload returns an independent loaded Drawable for explicit stopped adoption; refresh scene/tracing generations and ownership. Surface inputs need no camera. Four MRTs plus R8 compute coverage preserve 32-byte compatibility and UV-v=0 top orientation. Charts are local to prepared geometry and material seams; standalone filtering/accumulation must consume those identities, not infer them from black pixels or submesh IDs.


## M04-07 reviewed implementation requirement

Assemble unbaked immutable sources before starting target jobs. UV2 generation returns an independent replacement drawable and requires queue/tracing/target refresh. If material maps are adopted between targets, advance source generation and rebuild snapshots; alternatively defer all adoption until every target completes, as draw_lightmaps does. Transparent display objects are not automatic supported receivers or occluders; preserve the explicit tracing inclusion policy.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).
