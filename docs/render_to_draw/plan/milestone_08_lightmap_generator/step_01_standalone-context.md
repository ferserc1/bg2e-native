# M08-01 — Public standalone bake context and scene generation

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** Accepted M04/M05/M07 reviews.

## Scope and deliverable

- `lib/include/bg2e/render/StandaloneBakerContext.hpp`
- `lib/src/bg2e/render/StandaloneBakerContext.cpp`
- `lib/include/bg2e/bake/StandaloneBakerContext.hpp`
- `lib/include/bg2e/bake/StandaloneLightmapBaker.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/render/StandaloneBakerContext.hpp](../../../../lib/include/bg2e/render/StandaloneBakerContext.hpp), [lib/src/bg2e/render/StandaloneBakerContext.cpp](../../../../lib/src/bg2e/render/StandaloneBakerContext.cpp).

## Implementation work

Consume [M07's actual review](../milestone_07_bg2e_composer/review.md): own a
headless scene/job lifecycle rather than Composer's AppDelegate/import queue.
One live integrated baker must be released before its successor; preserve that
ownership lesson without binding standalone samples to window presentation slots.
Freeze source/settings identities and retain output owners through completion;
cancel/scene replacement may not publish or revive a discarded job.

1. Implement standalone Created/Initialized/SceneReady/Cleaned lifecycle and explicit scene preparation using a windowless draw engine/device, independent of a camera RendererDeferred.
2. Capture environment/light/material/geometry data and prepare isolated TLAS/resources using public draw/bake building blocks.
3. Bind baker instances to scene generation, reject stale/duplicate/invalid updates and define synchronous submission/completion for one accumulation sample.
4. Preserve target validation, root ownership, cleanup and producer coordination. No-RT hardware reports unsupported required baking instead of silently outputting unbaked maps.

## Verification and exit conditions

- A prepared context can bake multiple targets and invalidates prior bakers after a new scene generation.
- No main camera/window/UI or hidden production renderer is required to create resources; exceptions/cleanup do not leave active work.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M08-02/M08-04 use these types. M09 deletes the legacy standalone implementations only after equivalence is evidenced.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-08 standalone environment resources

Reuse the public EnvironmentGenerator/tool selections and validated snapshots from [environment-tools.md](../milestone_01_deferred_renderer/environment-tools.md). Load/generate before active recording, retain shared images/samplers through submissions and release all owners before Engine teardown. Empty source intentionally selects default sky; failed HDR input throws and preserves a previous published snapshot. Do not rebuild these algorithms privately in standalone bake.

## M01-13 reconstruction and settings

Read [upscaling.md](../milestone_01_deferred_renderer/upscaling.md). Reuse live resolution/upscaler settings identities where output reconstruction is exposed, with explicit drained lifecycle/reset boundaries in standalone contexts. SDK FSR temporal state is not a bake accumulation context or engine TemporalAccumulator.

## M01-16 reviewed baseline

At the original M01 review, draw::Engine had no offscreen overload. M05-02 now provides the actual headless context; use its generic surface/slot access and lifetime rules. Do not instantiate RendererDeferred to access private targets or reinterpret FSR/camera accumulation as bake accumulation. Use public environment snapshots/collections and M03-selected surface/material addressing. See [M01 review](../milestone_01_deferred_renderer/review.md).

## M03-01 tracing checkpoint dependency

The [advanced tracing design](../milestone_03_ray_tracing/rt-design.md) was accepted by project-lead continuation. [M03-02](../milestone_03_ray_tracing/rt-gpu-data.md) delivers primitive arrays/stages; the draw snapshot builder and integrated bake primitives are implemented. Reuse the public draw::pipeline tracing snapshot, packed geometry/material data, bounded image/sampler resources and shared cutout predicate. Supply explicit UV-space world-position/normal/coverage surfaces without a camera RendererDeferred. Check effective capacities and required tracing support before a job; invalidate scene/baker generations after material/texture/topology edits. Keep bake sample accumulation separate from rendered-view histories and FSR state. Do not duplicate a private table/shading family or silently bake with truncated/unsupported resources.

## M03-03 shared noise resource

Use the implemented [utils noise contract](../milestone_03_ray_tracing/blue-noise.md) for CPU sample data and shared Device-backed residency. Its tiles are logical layers in a color 2D atlas, not physical image-array layers. Bake jobs choose/reset explicit uint32 sample keys independently of presentation slots and retain image/sampler/metadata owners; disabled sampling uses the declared hash alternative without an unbound texture. No bake-specific duplicate noise data or generator is needed.

## M03-04 actual surface bridge

Reuse [RTAO contracts](../milestone_03_ray_tracing/rtao.md): SurfaceInputs::WorldPositionCoverage uses signed RGBA16 normals, RGBA32 world positions and R8 coverage, without a camera. TracingSceneResources produces retained bounded geometry/material snapshots independently of RendererDeferred. Caller owns completed-slot discipline, surface/output lifetimes and renderedSample sequencing. Integrated UV accumulation/presence is implemented; standalone scene/job scheduling is this milestone’s work.

## M03-05 actual HDR GI bridge

[RTGI handoff](../milestone_03_ray_tracing/rtgi.md) provides Device-backed RTGlobalIllumination with camera/explicit SurfaceInputs, retained matching-capacity TracingSnapshot and logical LightUniforms/EnvironmentResources. RGBA16 RGB is incoming diffuse radiance/pi before receiver albedo; alpha is validity. Accumulation/export must preserve that normalization and distinguish valid black from invalid zero. Empty scenes currently return invalid fallback; bake orchestration must explicitly define its environment-only policy.

## M03-08 explicit-surface filtering

[Public DenoiseFilter](../milestone_03_ray_tracing/denoise.md) consumes signed normals/world positions/R8 coverage without a fake camera and uses world-distance/normal rejection. Decide chart/coverage isolation and whether filtering is appropriate for the bake signal; no chart ID or exact bake average is supplied by M03-08. Keep accumulation and filtered output distinct, retain owners through completion and preserve GI validity/reflection confidence normalization.

## M03-10 reviewed implementation requirement

Consume public WorldPositionCoverage and matching-capacity snapshots directly, not camera renderer internals. Validate required AO/GI primitive/material-array/format profiles before publishing a job; unsupported required baking fails explicitly even though interactive raster fallback exists. Own slots, rendered sample keys, frozen scene generations and completion-safe cancellation.

Source and validation boundaries: [M03 technical review](../milestone_03_ray_tracing/review.md). Include these conditions in this step's implementation and exit checks.

## M04-01 names and ownership

[Delivered bake contracts](../milestone_04_lightmaps/contracts.md) define abstract BakerContext/LightmapBaker, shared live target settings, typed pixels and generation/progress semantics. StandaloneBakerContext, SceneAssembler and Batch are forward declarations only; implement them in this milestone without a production alias. Concrete integrated execution must not depend on them.


## M04-03 actual execution handoff

Reuse the delivered Device-backed LightmapBakeExecutor for explicit UV surfaces and public draw AO/GI snapshot recording. It does not submit or accumulate. Preserve matching capacity, fresh per-recording snapshot wrappers with eligible immutable generation reuse, required-effect failures, immutable environment owner copies and completed-slot lifetime. IntegratedBakerContext is now a concrete draw Engine scheduler with one live target/in-flight frame; standalone should provide its own scheduler over the executor rather than borrow a viewport renderer or extend the production render context.

See [integrated execution](../milestone_04_lightmaps/integrated-context.md).


## M04-07 reviewed implementation requirement

Reuse LightmapBakeExecutor, UvTemporalAccumulator and LightmapCompositionPass as isolated public recording/output owners. IntegratedBakerContext remains draw-Engine/window-lifecycle-bound and serial; its example's synchronous submission is not a headless initializer. M05 must deliver genuine windowless ownership first. Keep explicit per-job completion/abandonment, immutable source/environment ownership and required-capability failures; do not create RendererDeferred to borrow private state.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).


## M05-02 windowless context availability

The [headless Engine overload](../milestone_05_offscreen/headless-execution.md)
now initializes Instance/OffscreenSurface/Device without a window or UI. Use its
generic surface/slot access and stopped ownership boundaries directly where useful;
StandaloneBakerContext still needs its own bake job/configuration/lifecycle and
must not instantiate DefaultOffscreenApplicationDelegate/RendererDeferred merely
to borrow rendering internals. The camera-frame application driver is not a UV
bake scheduler or batch writer. Factory-owned Backend leases reject competing
executions; a shared active Instance may not be initialized or cleaned twice.

## M05-03 settings and accumulation boundary

[Headless settings/scene averaging](../milestone_05_offscreen/accumulation-settings.md)
now defines direct live import, explicit-document persistence bypass, resolution
overrides and unknown-ID diagnostics/catalog extensions. Reuse those settings
semantics for standalone setup. The default scene delegate's frames count and
CPU linear camera-image average are not UV update counts/coverage/confidence;
keep the public bake scheduler and UvTemporalAccumulator independent.


## M05 camera-image versus bake-job boundary

M05's public headless `OffscreenApplication` owns a serial scene-frame driver and
`DefaultOffscreenApplicationDelegate` optionally owns exact-N camera accumulation.
The M05 example's RGBA8 PNG is only its completed camera output. `StandaloneBakerContext`
still needs independent scene/baker generations, preparation/update/cleanup,
required-effect capability validation and job-level output ownership. Reuse the
headless Engine/context API and GPU completion discipline; do not use
`config.frames`, FrameAccumulator, or camera PNG readback as UV samples/coverage.

The [M06 review](../milestone_06_model_edit/review.md) confirms the integrated
context borrows an initialized draw Engine and is bound to windowed frame slots,
one live baker and one in-flight bake frame. It is not the standalone context.
Reuse public `LightmapBakeExecutor`, `UvTemporalAccumulator` and composition
primitives where their contracts fit, then implement a separate headless scheduler
with its own scene/job generations, completion/abandonment, required-effect
validation and cleanup while the Device remains alive.
## Later tracing-capacity handoff

Consume [the M07 16384 correction](../milestone_07_bg2e_composer/tracing-capacity-16384.md)
without restoring small compiled profiles. AO/GI native shaders use current v3 and exact
device-clamped capacities; shared snapshots must match each effect's actual
capacity. Preserve object-local materials and warning/neutral-texture table
overflow. Missing required native execution remains distinct from texture-table
degradation; do not relabel unavailable execution as a successful bake.

## M07-08 source reconciliation

At HEAD `78943d5820e69363c942d71136f479a0df87128e`, current source supersedes the
historical v2 table handoff: AO/GI/reflection use native v3 with Vulkan scene set1,
persistent material arrays set2 (bindings5/6), specialization ID0 and unchanged
Metal record buffers8/9. Direct query shadows use `draw.query.softShadows.v5`,
Vulkan invocation set0, scene set1 and persistent alpha set2 (bindings16/24),
Metal records5/6 and reserves (2,1,10). AO/GI/reflection reserves are respectively
(4,2,13), (5,3,15), (6,3,16). Exact device-clamped capacity up to 16384, neutral
pair overflow and object-local ownership remain. Consume current maintained
[tracing reference](../../../../doc/api/draw/TracingSceneResources.md) and source,
not historical ABI labels. This is static reconciliation, not a new native test.

## Current engine preservation requirements — 2026-10-21

R03–R09/R11: reuse the existing public LightmapBakeExecutor and snapshot APIs;
new job/slot wrappers may reuse matching immutable material/shadow tables and only
completed acceleration generations. A new UV sample is not a requirement for a
new full TLAS/native-array build. Exact recording completion, BLAS build generation
and source/target/settings validity are independent; never reuse pending/aborted
work. Preserve image/sampler native-generation/residency checks and neutral
capacity fallback; unsupported required AO/GI still fails. Do not mutate geometry
or materials to improve cache hits. Keep headless scene/job state independent of
RendererDeferred, presentation slots, display signatures and temporary pool state.

Retain existing profiling counters/scopes in reused primitives. The synchronous
sample completion required here is workload ownership, not permission to add GPU
waits for timing collection. Destructive scene/context cleanup still coordinates
producers and drains. Typed final-color wait elision does not apply to bake edits.

Read the [source reconciliation](../code-reconciliation-2026-10-21.md); current engine code overrides historical implementation assumptions. No native/performance result is implied by this source baseline.
