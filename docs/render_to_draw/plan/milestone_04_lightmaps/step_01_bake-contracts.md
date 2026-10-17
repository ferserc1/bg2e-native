# M04-01 — Bake namespace and public configuration/pixel contracts

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** Accepted M03 review.

## Scope and deliverable

- `lib/include/bg2e/render/LightmapSettings.hpp`
- `lib/include/bg2e/render/LightmapBaker.hpp`
- `lib/include/bg2e/bake/LightmapSettings.hpp`
- `lib/include/bg2e/bake/LightmapPixels.hpp`
- `lib/include/bg2e/bake/BakerContext.hpp`
- `lib/include/bg2e/bake/all.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/render/LightmapSettings.hpp](../../../../lib/include/bg2e/render/LightmapSettings.hpp), [lib/include/bg2e/render/LightmapBaker.hpp](../../../../lib/include/bg2e/render/LightmapBaker.hpp).

## Implementation work

1. Define all new map-baking configuration/resources/results under bake, including target/base contexts, UV preview and eventual standalone assembly/batch declarations as needed.
2. Specify LightmapPixels byte/float RGB formats, size/orientation/color semantics and LightmapSettings source-of-truth accessors/configuration. UI/persistence enumerate actual bake object settings, not duplicate values.
3. Expose compatible public draw::pipeline effect/surface/filter object injection and explicit ownership in bake configuration structs.
4. Record the complete legacy-to-bake mapping, including private executor/UV/composition/accumulation responsibilities, without moving UV2 geometry preprocessing.

## Verification and exit conditions

- Public declarations can express AO/GI and future map generation inputs without native Vulkan descriptors or a mandatory RendererDeferred.
- No implementation depends on future standalone APIs merely to run an integrated bake.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M04-03/M04-04 implement these values/lifetimes. M08 completes StandaloneBakerContext/SceneAssembler/Batch in bake, not DB or draw.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-08 reusable environment tools

Standalone/IBL bake inputs reuse public draw::environment generation services and shared EnvironmentResources described in [environment-tools.md](../milestone_01_deferred_renderer/environment-tools.md). Generation is synchronous at a stopped-producer boundary and independent of RT support; preserve linear radiance, PI-inclusive irradiance, authored roughness mips and BRDF A/B conventions. External retained images keep shared owners, and Engine/device outlive every bake user.

## M03-01 tracing checkpoint dependency

The [advanced tracing design](../milestone_03_ray_tracing/rt-design.md) was accepted by project-lead continuation. [M03-02](../milestone_03_ray_tracing/rt-gpu-data.md) delivers primitive arrays/stages; the draw snapshot builder and bake infrastructure remain pending. After M03 implementation/review, reuse the public draw::pipeline tracing snapshot, packed geometry/material data, bounded image/sampler resources and shared cutout predicate. Supply explicit UV-space world-position/normal/coverage surfaces without a camera RendererDeferred. Check effective capacities and required tracing support before a job; invalidate scene/baker generations after material/texture/topology edits. Keep bake sample accumulation separate from rendered-view histories and FSR state. Do not duplicate a private table/shading family or silently bake with truncated/unsupported resources.

## M03-04 actual surface bridge

Reuse [RTAO contracts](../milestone_03_ray_tracing/rtao.md): SurfaceInputs::WorldPositionCoverage uses signed RGBA16 normals, RGBA32 world positions and R8 coverage, without a camera. TracingSceneResources produces retained bounded geometry/material snapshots independently of RendererDeferred. Caller owns completed-slot discipline, surface/output lifetimes and renderedSample sequencing. Bake accumulation/presence policy is still this milestone’s work.

## M03-05 actual HDR GI bridge

[RTGI handoff](../milestone_03_ray_tracing/rtgi.md) provides Device-backed RTGlobalIllumination with camera/explicit SurfaceInputs, retained matching-capacity TracingSnapshot and logical LightUniforms/EnvironmentResources. RGBA16 RGB is incoming diffuse radiance/pi before receiver albedo; alpha is validity. Accumulation/export must preserve that normalization and distinguish valid black from invalid zero. Empty scenes currently return invalid fallback; bake orchestration must explicitly define its environment-only policy.

## M03-07 temporal boundary

[Temporal handoff](../milestone_03_ray_tracing/temporal.md) supplies independent effect/view/layer histories, bounded nominal and per-pixel convergence, rolling/frozen settings and explicit reset generation. It is not an exact UV bake average or a requested-frame scheduler. Select actual submitted scene samples, retain confidence semantics, freeze scene state and distinguish nominal frameCount from pixel counts restarted by rejection. Explicit stationary surfaces can use same-UV correspondence; define the bake signal separately.

## M03-08 explicit-surface filtering

[Public DenoiseFilter](../milestone_03_ray_tracing/denoise.md) consumes signed normals/world positions/R8 coverage without a fake camera and uses world-distance/normal rejection. Decide chart/coverage isolation and whether filtering is appropriate for the bake signal; no chart ID or exact bake average is supplied by M03-08. Keep accumulation and filtered output distinct, retain owners through completion and preserve GI validity/reflection confidence normalization.

## M03-10 reviewed implementation requirement

Name SurfaceInputs::WorldPositionCoverage and explicit R16 visibility / RGBA16 incoming diffuse radiance-per-pi plus validity. Specify baked RGB multiplier/presence independently of material AO; establish albedo/PI normalization once. Define matching-capacity TracingSnapshot ownership and required-effect capability failure before creating outputs.

Source and validation boundaries: [M03 technical review](../milestone_03_ray_tracing/review.md). Include these conditions in this step's implementation and exit checks.
