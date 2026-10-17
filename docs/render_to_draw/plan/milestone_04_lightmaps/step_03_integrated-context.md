# M04-03 — Integrated context and shared AO/GI bake execution

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M04-01/M04-02 and M03 effects.

## Scope and deliverable

- `lib/include/bg2e/render/IntegratedBakerContext.hpp`
- `lib/src/bg2e/render/IntegratedBakerContext.cpp`
- `lib/include/bg2e/bake/IntegratedBakerContext.hpp`
- `lib/include/bg2e/bake/LightmapBakeExecutor.hpp`
- `shaders/src/bake/rtao_uv/`
- `shaders/src/bake/rtgi_uv/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/render/IntegratedBakerContext.hpp](../../../../lib/include/bg2e/render/IntegratedBakerContext.hpp), [lib/src/bg2e/render/IntegratedBakerContext.cpp](../../../../lib/src/bg2e/render/IntegratedBakerContext.cpp).

## Implementation work

1. Implement public IntegratedBakerContext/IntegratedLightmapBaker using experimental command/frame contexts and bake-owned snapshots/TLAS/bindings where isolation is required.
2. Validate prepared command, slot, target scene generation and one-sample-per-frame rules. Borrow/share explicit environment resources with lifetime checks, rather than initialize a second renderer.
3. Reuse public draw AO/GI implementations/configurations with the UV profile and explicit backend shader variants. Allow compatible application replacements.
4. Define edit/cancel/reset behavior and retain sources until submitted work completes. UI/editor gizmos never contaminate the traceable bake scene.

## Verification and exit conditions

- Prepared-frame mismatches, duplicate updates, removed targets and scene edits are rejected or reset according to the declared contract.
- An integrated bake runs alongside scene presentation without overwriting raster/selection descriptor or tracing resources.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M04-06 and M06/M07 use these lifecycle hooks. M08 implements a different standalone scheduler over the same public bake execution pieces.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-09 geometry and material extraction

Use the draw queue/visitor and binding contracts in [bindings-visitors.md](../milestone_01_deferred_renderer/bindings-visitors.md), including node/drawable/submesh transforms, hidden branches, retained resource snapshots and per-acquired-slot mutable data. Material HasAOTexture means AO presence only; add actual baked-lightmap metadata/flags rather than treating every AO texture as baked lighting. Geometry/texture changes occur at stopped boundaries and require fresh extraction/rebinding.

## M01-10 reusable raster stages

Reuse public GBuffer/DeferredLayer/FinalLayer/RenderTarget/FullscreenPass from the [actual graph](../milestone_01_deferred_renderer/raster-layers.md), with native binding/packing and acquired-slot ownership intact. G-buffer channel5 is currently white; AO presence never activates baked data. Add compatible actual bake multipliers here without duplicating a private lighting family. Tone/color correction occurs once after linear HDR composition. Standalone UV/bake orchestration stays in bake; renderer environment generation requires a safe lifecycle/pre-acquire boundary.

## M01-11 alpha visibility boundary

M01-11 adds [compute cutout-aware shadow visibility](../milestone_01_deferred_renderer/query-shadows.md), a dense instance/UV/alpha-only table with eight distinct image/sampler pairs and generic native opacity flags. Bake reuses the actual arbitrary RT material path chosen by M03, not this restricted table as a complete bake shading model. Preserve UV-set/scale/linear-alpha/cutoff and exact submesh ranges, actual source-size/sample semantics, per-submission owners and non-opaque accepted-candidate filtering. No bake or arbitrary material addressing was added in M01-11.

## M01-16 reviewed baseline

RendererDeferred does not expose private G-buffer targets as a bake API. Reuse public surface/effect profiles from M03 (or add the minimal borrowed contract with slot/generation lifetime); use EnvironmentGenerator::resources() or explicit configuration.environment shared snapshots. Current neutral baked RGB is not real map application: RGB multiplier/UV metadata and invalidation still belong to this milestone. See [M01 review](../milestone_01_deferred_renderer/review.md).

The [M02 edit/selection review](../milestone_02_manipulation/review.md) adds
application-owned generation checks and stopped-boundary `MaterialModifier`
updates, but it does not provide bake invalidation. Integrated contexts must
subscribe to or receive explicit source-edit/reload notifications for material,
UV/topology, environment and scene replacement; invalidate in-flight generations
and refresh queue/bindings before accepting another sample. `SelectionItem` and
viewpoint candidate IDs are editor-local and do not establish bake identity.

## M03-01 tracing checkpoint dependency

The [advanced tracing design](../milestone_03_ray_tracing/rt-design.md) was accepted by project-lead continuation. [M03-02](../milestone_03_ray_tracing/rt-gpu-data.md) delivers primitive arrays/stages; the draw snapshot builder and bake infrastructure remain pending. After M03 implementation/review, reuse the public draw::pipeline tracing snapshot, packed geometry/material data, bounded image/sampler resources and shared cutout predicate. Supply explicit UV-space world-position/normal/coverage surfaces without a camera RendererDeferred. Check effective capacities and required tracing support before a job; invalidate scene/baker generations after material/texture/topology edits. Keep bake sample accumulation separate from rendered-view histories and FSR state. Do not duplicate a private table/shading family or silently bake with truncated/unsupported resources.

## M03-03 shared noise resource

Use the implemented [utils noise contract](../milestone_03_ray_tracing/blue-noise.md) for CPU sample data and shared Device-backed residency. Its tiles are logical layers in a color 2D atlas, not physical image-array layers. Bake jobs choose/reset explicit uint32 sample keys independently of presentation slots and retain image/sampler/metadata owners; disabled sampling uses the declared hash alternative without an unbound texture. No bake-specific duplicate noise data or generator is needed.

## M03-04 actual surface bridge

Reuse [RTAO contracts](../milestone_03_ray_tracing/rtao.md): SurfaceInputs::WorldPositionCoverage uses signed RGBA16 normals, RGBA32 world positions and R8 coverage, without a camera. TracingSceneResources produces retained bounded geometry/material snapshots independently of RendererDeferred. Caller owns completed-slot discipline, surface/output lifetimes and renderedSample sequencing. Bake accumulation/presence policy is still this milestone’s work.

## M03-05 actual HDR GI bridge

[RTGI handoff](../milestone_03_ray_tracing/rtgi.md) provides Device-backed RTGlobalIllumination with camera/explicit SurfaceInputs, retained matching-capacity TracingSnapshot and logical LightUniforms/EnvironmentResources. RGBA16 RGB is incoming diffuse radiance/pi before receiver albedo; alpha is validity. Accumulation/export must preserve that normalization and distinguish valid black from invalid zero. Empty scenes currently return invalid fallback; bake orchestration must explicitly define its environment-only policy.

## M03-08 explicit-surface filtering

[Public DenoiseFilter](../milestone_03_ray_tracing/denoise.md) consumes signed normals/world positions/R8 coverage without a fake camera and uses world-distance/normal rejection. Decide chart/coverage isolation and whether filtering is appropriate for the bake signal; no chart ID or exact bake average is supplied by M03-08. Keep accumulation and filtered output distinct, retain owners through completion and preserve GI validity/reflection confidence normalization.

## M03-10 reviewed implementation requirement

Use TracingSceneResources(device, capacity), prepare(queue), retained TracingSnapshot and effect prepareSnapshot with compatible capacity/device. Call snapshot build only in its one recording/submission; discard after aborted recording. Schedule begin/prepare/record against completed slots, retain source/environment owners on cancellation and use explicit renderedSample keys. No RendererDeferred private target dependency.

Source and validation boundaries: [M03 technical review](../milestone_03_ray_tracing/review.md). Include these conditions in this step's implementation and exit checks.
