# M03-10 — Review M03 backend contracts and prepare bake/headless reuse

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M03 product and checkpoint evidence.

## Scope and deliverable

- `docs/render_to_draw/plan/milestone_03_ray_tracing/review.md`
- `docs/render_to_draw/plan/milestone_03_ray_tracing/rt-design.md`
- `docs/render_to_draw/plan/milestone_04_lightmaps/summary.md`
- `docs/render_to_draw/plan/milestone_05_offscreen/summary.md`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

## Implementation work

1. Compare final code with the accepted RT design: shader stages, table layouts, non-opaque candidates, limits/capabilities and backend differences.
2. Verify public effect/surface/configuration/shader reuse and map actual types to bake UV passes/context in M04/M08; revise those steps wherever assumptions were wrong.
3. Compare temporal sample/reset/motion behavior and settings codec with M05 accumulation/export and M06/M07 live editing. Update all pending M04–M10 contracts.
4. Record image/behavior evidence and gaps per backend, and list improvements intentionally not implemented rather than silently call them parity.

## Verification and exit conditions

- The report links implemented backend code and explains actual history/denoise/cutout behavior with product evidence.
- Pending bake/headless/app steps consume real interfaces and account for all missing/approved changes.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

No baking work starts with a vague promise that camera passes are reusable; the review must name the compatible input profiles or schedule concrete adaptations.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## Revised final handoff

This technical review is followed by [M03-11 — Cumulative documentation](step_11_documentation.md) in the same milestone. On review delivery, progress points to that documentation step, with normal lead acceptance required. The review does not close the milestone or authorize the following milestone by itself. Reconcile its report with the documentation handoff after that final step.

## M03-01 checkpoint handoff — accepted by lead continuation

Audit the implementation against the accepted [M03-01 checkpoint](rt-design.md): actual stage/SBT handling, enabled nonuniform indexing, Metal argument-buffer residency, effective capacity profiles, packed index bases/normal transforms, all six material semantics, candidate rejection, scene error versus unavailable status and public invalidation. Record deviations and lead-reported backend coverage. Confirm M04/M08 can reuse the public tracing snapshot/surface/filter contracts without RendererDeferred, and update their real pending steps before handoff.

## M03-02 actual primitive contract

Read [rt-gpu-data.md](rt-gpu-data.md). Use ShaderStage::AnyHit, appended anyHitShader, ResourceBinding::additionalStages and explicit metalArgumentBuffer/argument IDs. Set complete arrays through setSampledImages/setSamplers; choose matching 32/128/256 native shader profiles from logical-device resourceArrayCapabilities after reserving other bindings. Metal arrays currently support compute/RT color 2D sampled resources only, with retained source owners and retired/drained updates. Arbitrary draw geometry/material snapshot construction remains M03-04; the primitive probe's tiny material record is not the engine ABI. No buffer-address or array graphics-stage infrastructure is implied.

## M03-03 actual noise input

Reuse [utils::BlueNoise](blue-noise.md): CPU layer-major rank data and an explicitly shared RGBA8 color 2D atlas, with uint4 metadata and paired bg2eBlueNoise2/bg2eHashNoise2 helpers under draw/lib. Logical layers are stacked in Y; physical arrayLayers is one. Do not copy production sampler2DArray bindings or assume general image-array support. Effects own rendered-view/sample keys and disabled/missing-noise variants without texture access; the utility does not advance presentation/slot state or reserve tracing-table capacity implicitly. Retain image/sampler owners and metadata snapshots through completion.

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
