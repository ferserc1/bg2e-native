# M08-02 — Bake model/prefab/context assembly and UV2

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M08-01 and accepted M06/M07 DB contracts.

## Scope and deliverable

Preserve M06's nonfatal UV2 overlap policy: validation for baking uses
`UvAtlasOverlapPolicy::Warn`, reports diagnostics and never skips/aborts a target
solely for overlapping triangle interiors or chart collisions. Retain structural
and finite-coordinate/geometry checks.

- `lib/include/bg2e/render/StandaloneBakeSceneAssembler.hpp`
- `lib/src/bg2e/render/StandaloneBakeSceneAssembler.cpp`
- `lib/include/bg2e/bake/StandaloneBakeSceneAssembler.hpp`
- `lib/include/bg2e/geo/GenerateUv2AtlasModifier.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/render/StandaloneBakeSceneAssembler.hpp](../../../../lib/include/bg2e/render/StandaloneBakeSceneAssembler.hpp), [lib/src/bg2e/render/StandaloneBakeSceneAssembler.cpp](../../../../lib/src/bg2e/render/StandaloneBakeSceneAssembler.cpp), [lib/include/bg2e/geo/GenerateUv2AtlasModifier.hpp](../../../../lib/include/bg2e/geo/GenerateUv2AtlasModifier.hpp).

## Implementation work

Honor the M07 lead correction: unregistered JSON component types warn/skip while
the remaining graph loads. Do not reintroduce strict unknown-type rejection in
assembly. Registered malformed/resource failures and required bake target checks
remain distinct; raw JSON retention is not implied by typed save.

Reconcile [M07 review](../milestone_07_bg2e_composer/review.md): preserve complete
multi-root containers and detach Scene back-pointers through releaseSceneRoot;
unwrap only the synthetic single-root level. Do not import Composer's camera-required
Open policy into bake prefabs. Keep chain joints, exact custom component/projection
clone contracts and isSceneGeometry helper exclusion. Use parsed-data DB overloads,
not Composer's worker/HTTP classes, and freeze snapshots after UV2 replacement.

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

The [advanced tracing design](../milestone_03_ray_tracing/rt-design.md) was accepted by project-lead continuation. [M03-02](../milestone_03_ray_tracing/rt-gpu-data.md) delivers primitive arrays/stages; the draw snapshot builder and integrated bake primitives are implemented. Reuse the public draw::pipeline tracing snapshot, packed geometry/material data, bounded image/sampler resources and shared cutout predicate. Supply explicit UV-space world-position/normal/coverage surfaces without a camera RendererDeferred. Check effective capacities and required tracing support before a job; invalidate scene/baker generations after material/texture/topology edits. Keep bake sample accumulation separate from rendered-view histories and FSR state. Do not duplicate a private table/shading family or silently bake with truncated/unsupported resources.

## M04-02 actual replacement and surface inputs

Reuse [implemented UV surfaces/reload](../milestone_04_lightmaps/uv-surfaces.md). generateUv2AndReload returns an independent loaded Drawable for explicit stopped adoption; refresh scene/tracing generations and ownership. Surface inputs need no camera. Four MRTs plus R8 compute coverage preserve 32-byte compatibility and UV-v=0 top orientation. Charts are local to prepared geometry and material seams; standalone filtering/accumulation must consume those identities, not infer them from black pixels or submesh IDs.


## M04-07 reviewed implementation requirement

Assemble unbaked immutable sources before starting target jobs. UV2 generation returns an independent replacement drawable and requires queue/tracing/target refresh. If material maps are adopted between targets, advance source generation and rebuild snapshots; alternatively defer all adoption until every target completes, as draw_lightmaps does. Transparent display objects are not automatic supported receivers or occluders; preserve the explicit tracing inclusion policy.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).

## M07-02 scene/prefab ownership handoff

Scene::releaseSceneRoot transfers a complete schema/draw scene root and clears
its former Scene back-pointer/caches. Retain the detached container for multi-root
files; unwrap exactly one synthetic level for a single editable/prefab root.
Node lookup now supports registered component subclasses through their base
interface, and clones reject shared/owned/sliced component copies. Camera
projections have an independent virtual clone contract. Reuse these ownership
contracts when assembling standalone sources; the Composer's camera-required
Open policy is app-specific and must not reject camera-less bake prefabs.
See [M07-02 actual handoff](../milestone_07_bg2e_composer/scene-io-cloning.md).


## M07-04 CPU parser handoff

[Composer's implemented import boundary](../milestone_07_bg2e_composer/async-import-service.md)
adds db::readGltf/GltfDocument and db::readObj/ObjMesh plus stopped draw realization
of their results and existing Bg2Mesh CPU data. Reuse these DB entry points where
assembly needs them. They do not provide a batch scheduler, filesystem snapshot,
job cancellation or protected output publication. Keep factories/GPU work with
the initialized standalone context and do not depend on Composer's HTTP/job classes.
## Later resource-table policy

Preserve independently owned/importable/exportable scene-object materials.
[M07 tracing tables](../milestone_07_bg2e_composer/tracing-capacity-16384.md) are
execution bindings with a 16384/device-clamped ceiling, not a global material
catalog or a requirement to deduplicate objects/materials before scene assembly.
Keep the explicit warning/neutral-texture overflow policy.

## Current engine preservation requirements — 2026-10-21

R01–R04/R14: preserve cutoff default 0, effective threshold 0.02 (equality tests),
MASK import exactly 0.5 and other glTF modes 0; do not restore the earlier default
or infer cutout from transparency. Preserve independent editable clone/import/export
ownership while sharing only valid derived immutable representations. Freeze after
UV2 realization; actual geometry changes refresh BLAS/build/source identities.
Retain material/texture revision propagation, helper exclusion and default blended
tracing exclusion. CPU schema/parser separation is already implemented.

Read the [source reconciliation](../code-reconciliation-2026-10-21.md); current engine code overrides historical implementation assumptions. No native/performance result is implied by this source baseline.

## M08-01 delivered execution contract

[Standalone context](standalone-context.md) now borrows an initialized headless draw
Engine, with explicit initialize/prepareScene and one private serial slot. Reuse
that scheduler; do not initialize a second Engine or borrow a viewport renderer.
prepareScene takes a loaded retained root and optional EnvironmentResources;
otherwise it generates the selected authored/default environment through public
tools. The caller performs graph updates and UV2 realization first. Release the
previous baker before createBaker; stale scene jobs cannot reset into a new scene.
Each update completes exactly one synchronous sample; readPixels is available only
for current completed validated output. Context cleanup precedes Engine cleanup;
returned GPU outputs/targets must also be released inside the Engine lifetime.
No standalone assembly, batch writer or CLI is supplied by M08-01.

## Delivered contract

See [scene-assembly.md](scene-assembly.md) for the implemented
StandaloneBakeSceneAssembler API, private-candidate/source ownership, initial CPU
UV2 generation before upload, schema-only versus target-resource loading, stable
hierarchy identities, validation and remaining M08-03 dependency-discovery duties.
The production assembler is retained as a reference and is not modified.
