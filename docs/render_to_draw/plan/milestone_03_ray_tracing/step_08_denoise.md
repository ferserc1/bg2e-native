# M03-08 — Public scalar/HDR denoise and reusable surface inputs

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M03-04–M03-07.

## Scope and deliverable

- `lib/src/bg2e/render/deferred/DenoiseFilter.cpp`
- `lib/include/bg2e/draw/pipeline/DenoiseFilter.hpp`
- `shaders/src/draw/denoise/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/src/bg2e/render/deferred/DenoiseFilter.cpp](../../../../lib/src/bg2e/render/deferred/DenoiseFilter.cpp).

## Implementation work

1. Implement compatible scalar/HDR bilateral denoise with kernel radius, depth/normal thresholds and sigma accessors. Preserve ordering relative to temporal/effect/composition outputs.
2. Expose shader/configuration/resource replacement and explicit depth/normal/surface input requirements for application renderers and later bake reuse.
3. Ensure dispatch dimensions, storage formats, image borders and sampled/storage state transitions are correct in explicit GLSL/Metal implementations.
4. Avoid a hard-wired dependency on the camera GBuffer class where a reusable surface profile suffices; choose no filtering when unsupported rather than a wrong-format output.

## Verification and exit conditions

- Noise reduces without smearing declared depth/normal discontinuities or corrupting HDR energy; odd extents and borders are valid.
- A compatible custom filter can be supplied and settings enumeration reads its actual accessor values.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M04 decides whether/how bake accumulation uses these filters based on its UV profile. Do not duplicate private kernels to bypass incompatible inputs.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M03-01 checkpoint handoff — accepted by lead continuation

Consume typed scalar AO and linear HDR GI/reflection signals and explicit surface validity from the accepted [M03-01 design](rt-design.md). Filter resources are per effect/view/layer and extent; retain owners through completion. UV-world-position bake profiles work without camera reconstruction/motion. Filter scratch, temporal history and exact bake accumulation are distinct resources; neither GI validity nor reflection confidence may be inferred from black pixels.

## M03-04 actual implementation handoff

Use [rtao.md](rtao.md): public RTAmbientOcclusion, SurfaceInputs camera/explicit profiles and TracingSceneResources/TracingSnapshot now exist. Reuse dense IDs, retained material tables, scalar R16 output and explicit renderedSample keys. AO has independent quality dimensions via ExtentDomain::Scaled. No history/filtering or visible controls are implemented here. Legacy SoftShadowPass retains its separate eight-cutout-pair limit; reconcile that consumer during integration/review. Native pipeline GLSL users may include tracing.glsl without DRAW_TRACE_QUERIES; compute-query users define it first.

## M03-05 actual GI handoff

[rtgi.md](rtgi.md) records public RTGlobalIllumination, native Vulkan raygen/miss/closest-hit/any-hit with depth one, Metal compute queries, shared material tables and explicit receivers. RGBA16 output is incoming diffuse radiance/pi before receiver albedo with alpha validity; invalid zero restores diffuse IBL. RendererDeferred selects IBL/AO/GI independently of retained settings, preserves specular IBL and supports scaled GI inputs. History/filter/UI and baked presence policy remain pending. Reuse this exact normalization and matching-capacity snapshot ownership.

## M03-06 actual reflection handoff

Use [rt-reflections.md](rt-reflections.md): public RTReflections reuses retained scene tables and camera/explicit surfaces, with receiver roughness and camera position. Quality scales display output independently of G-buffer/internal/FSR extent. RGBA16 RGB is mean geometry-hit radiance before receiver BRDF, unpremultiplied; alpha is finite accepted hits/actualSamples. Preserve fractional confidence and rejected masks separately from GI binary validity and AO scalar visibility. The lighting pass blends prefiltered IBL with traced radiance using confidence and applies split-sum BRDF once. FullscreenPass supports an optional independent Metal sampler map; reflection texture16 shares nearest sampler0. Histories/filter/UI remain pending.

## M03-07 actual temporal handoff

Use [temporal.md](temporal.md): public TemporalAccumulator returns owned sampled scalar/HDR output or borrowed raw pass-through; SurfaceInputs/TemporalView expose inputs without private renderer access. Keys separate signal/view/layer, sceneFrameNumber advances actual work, maxFrames bounds per-pixel arithmetic/rolling/frozen accumulation. GI binary validity and reflection fractional confidence remain distinct. Renderer::invalidateHistories coordinates SDK and engine histories; resetUpscaler forwards for existing adapters. Exact extraction/settings signatures detect identity/value changes; in-place uploads/custom shaders and same-camera teleports require app notification. Object motion conservatively resets until previous-object vectors are added. Fresh history images and per-slot retained versions avoid overwrite; same ordered queue/submission discipline is required.

## M03-08 delivered contract

[denoise.md](denoise.md) records public DenoiseFilter, reusable inputs, per-invocation ownership, independent scalar/HDR capability profiles, exact signal/confidence math, ordering, metadata and native ABI. Project lead compilation/runtime validation is pending.
