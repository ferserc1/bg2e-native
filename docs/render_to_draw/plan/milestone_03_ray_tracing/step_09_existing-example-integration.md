# M03-09 — Integrate effects and verify existing examples only

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M03-04–M03-08.

The [M02 review](../milestone_02_manipulation/review.md) records the actual safe
edit boundary: MaterialModifier does not own invalidation; DrawMaterialEditor
queues weak/generation-checked edits and exposes `onChanged(Renderer&)`; the
application resets the currently affected history and requests redraw. Adapt this
to effect-specific histories, including partial failures, geometry replacement,
selection/resource invalidation and no-RT behavior. The review's continuation
acceptance does not supply new runtime/backend evidence; retain the lead checks
below as explicit validation rather than assuming them from source inspection.

## Scope and deliverable

- `lib/include/bg2e/draw/RendererDeferred.hpp`
- `examples/draw/10_deferred_renderer/src/main.cpp`
- `examples/draw/11_manipulation/src/main.cpp`
- `lib/include/bg2e/draw/RenderSettingsPreferences.hpp`
- `lib/include/bg2e/render/deferred/SMAAProcessor.hpp`
- `lib/include/bg2e/draw/pipeline/SMAAProcessor.hpp`
- `shaders/src/draw/smaa/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

## Implementation work

1. Attach optional AO/GI/reflection/temporal/denoise public objects via renderer/layer configuration and actual GPU capability checks.
2. Register effect settings dynamically, export/import stable options and expose implemented debug signals; reset on MaterialModifier/gizmo changes.
3. Restore effect controls deferred by M01 in its existing example and expose the same catalog in manipulation. No third example or duplicate application loop.
4. Verify all combinations relevant to production behavior, unsupported/disabled fallbacks and default versus FSR output; retain no-RT controls and clone/material regression fixtures.
5. Preserve the existing spatial-AA facility by reimplementing public SMAA processing/final-output components with paired shaders, lookup resources and required GPU formats. Make it an optional compatible pipeline component with its own accessor/reflected controls; the default stays simple resolution reduction. This is separate from explicitly excluded MSAA and adds no new example target.

## Verification and exit conditions

- Both examples exercise AO/GI switching, reflections, shadow controls, temporal/denoise, resize/pause/material edits and settings import on available backends.
- Primitive/effect unavailable states produce accurate UI and usable rendering, without forced FSR/full-RT capability coupling.
- The public optional SMAA component operates on no-RT raster output and can be selected/injected without hard-coding FSR as the only final processor.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M04/M05 products reuse this full scene renderer and its actual effect input/reset/convergence contracts.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-14 reflected effect UI

Use [GraphicsSettingsWindow](../milestone_01_deferred_renderer/settings-ui.md) and real catalog extensions for implemented effect settings. New targets appear through per-frame descriptor enumeration and safe queued application; do not recreate closed effect controls or a copied configuration model. Preserve explicit availability/status and catalog generations on resource replacement.

## M01-15 actual example

Extend target draw_deferred_renderer at examples/draw/10_deferred_renderer and the eventual manipulation product; do not create a new advanced-RT target. [working-example.md](../milestone_01_deferred_renderer/working-example.md) and the example README list retained/deferred reference capabilities. Current FSR is camera-only, query occluders exclude transparency, and debug modes show the last processed layer. Restore implemented effects through real catalog extensions.

## M03-01 checkpoint handoff — accepted by lead continuation

Demonstrate the accepted [M03-01](rt-design.md) effective capability/profile status in both existing examples. Queue material/geometry/light/environment edits through the new tracing/history invalidation contract. Exercise scenes beyond eight image/sampler pairs, deliberate capacity errors, cutout rear hits and separate query-only/no-RT behavior; do not claim advanced support from aggregate rayTracingSupported. Keep the documented blended-secondary-transport boundary visible in validation notes.

## M03-04 actual implementation handoff

Use [rtao.md](rtao.md): public RTAmbientOcclusion, SurfaceInputs camera/explicit profiles and TracingSceneResources/TracingSnapshot now exist. Reuse dense IDs, retained material tables, scalar R16 output and explicit renderedSample keys. AO has independent quality dimensions via ExtentDomain::Scaled. No history/filtering or visible controls are implemented here. Legacy SoftShadowPass retains its separate eight-cutout-pair limit; reconcile that consumer during integration/review. Native pipeline GLSL users may include tracing.glsl without DRAW_TRACE_QUERIES; compute-query users define it first.

## M03-05 actual GI handoff

[rtgi.md](rtgi.md) records public RTGlobalIllumination, native Vulkan raygen/miss/closest-hit/any-hit with depth one, Metal compute queries, shared material tables and explicit receivers. RGBA16 output is incoming diffuse radiance/pi before receiver albedo with alpha validity; invalid zero restores diffuse IBL. RendererDeferred selects IBL/AO/GI independently of retained settings, preserves specular IBL and supports scaled GI inputs. History/filter/UI and baked presence policy remain pending. Reuse this exact normalization and matching-capacity snapshot ownership.

## M03-06 actual reflection handoff

Use [rt-reflections.md](rt-reflections.md): public RTReflections reuses retained scene tables and camera/explicit surfaces, with receiver roughness and camera position. Quality scales display output independently of G-buffer/internal/FSR extent. RGBA16 RGB is mean geometry-hit radiance before receiver BRDF, unpremultiplied; alpha is finite accepted hits/actualSamples. Preserve fractional confidence and rejected masks separately from GI binary validity and AO scalar visibility. The lighting pass blends prefiltered IBL with traced radiance using confidence and applies split-sum BRDF once. FullscreenPass supports an optional independent Metal sampler map; reflection texture16 shares nearest sampler0. Histories/filter/UI remain pending.

## M03-07 actual temporal handoff

Use [temporal.md](temporal.md): public TemporalAccumulator returns owned sampled scalar/HDR output or borrowed raw pass-through; SurfaceInputs/TemporalView expose inputs without private renderer access. Keys separate signal/view/layer, sceneFrameNumber advances actual work, maxFrames bounds per-pixel arithmetic/rolling/frozen accumulation. GI binary validity and reflection fractional confidence remain distinct. Renderer::invalidateHistories coordinates SDK and engine histories; resetUpscaler forwards for existing adapters. Exact extraction/settings signatures detect identity/value changes; in-place uploads/custom shaders and same-camera teleports require app notification. Object motion conservatively resets until previous-object vectors are added. Fresh history images and per-slot retained versions avoid overwrite; same ordered queue/submission discipline is required.

## M03-08 actual filter handoff

Use [denoise.md](denoise.md): RendererDeferred injects/exposes DenoiseFilter and resolves tracing -> temporal -> spatial -> lighting. Inputs reuse SurfaceInputs; output keeps scalar R16/HDR RGBA16 and independent source extent. GI validity and reflection confidence are explicit; filtered RGB stays unpremultiplied and never replaces temporal history. Camera thresholds use linear view Z, explicit profiles use world distance. Virtual settingsType/settingsInstance accessors support live catalog binding; M03-09 must register real controls/preferences and avoid copied settings values. Unsupported profiles return borrowed source; fresh output owners and per-slot descriptors remain alive through completion.

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
