# M03 — Complete advanced ray tracing, histories and denoise

[Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Review protocol](../review-protocol.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Entry prerequisites:** Accepted M01/M02 products and reviews. Advanced RT design choices are not approved by the earlier query-shadow implementation.

M02 supplies typed weak selection identities, synchronous submission-local RGBA8 picking, draw viewpoint D32 readback, partial CPU/live material edits and application-owned invalidation. These are editing/application facilities, not an advanced RT material ABI or render history. See [M02 review](../milestone_02_manipulation/review.md). M03-01 remains a lead-owned explicit backend/material-addressing decision.

## Working product

The existing `examples/draw/10_deferred_renderer` and `examples/draw/11_manipulation` expose supported RTAO/RTGI, reflections and their temporal/denoise controls through live reflected settings. No new example is created. Vulkan and Metal use explicit equivalent backend shader implementations; no-RT raster behavior remains valid.

## Scope exclusions

- No integrated/standalone baking or offscreen application product yet; expose reusable inputs for M04 without implementing bake orchestration.
- No new example target, renderer family, shader translation system, forward renderer or MSAA.
- No forced native RT pipeline emulation contract for Metal; the existing compute/intersection model is reviewed before selection.

## Ordered steps

| Step | Deliverable |
|---|---|
| [M03-01](step_01_rt-design-checkpoint.md) | Deferred decision checkpoint for advanced RT backends |
| [M03-02](step_02_rt-gpu-data.md) | GPU tracing stages and shader data access selected by checkpoint |
| [M03-03](step_03_blue-noise-utils.md) | Generic BlueNoise data and GPU resource in utils |
| [M03-04](step_04_rtao.md) | Public RTAO implementation and fallback |
| [M03-05](step_05_rtgi.md) | Public RTGI and equivalent backend kernels |
| [M03-06](step_06_rt-reflections.md) | Public traced reflections and resolution semantics |
| [M03-07](step_07_temporal.md) | Rendered-view histories and temporal accumulation |
| [M03-08](step_08_denoise.md) | Public scalar/HDR denoise and reusable surface inputs |
| [M03-09](step_09_existing-example-integration.md) | Integrate effects and verify existing examples only |
| [M03-10](step_10_review.md) | Review M03 backend contracts and prepare bake/headless reuse |
| [M03-11](step_11_documentation.md) | Update maintained documentation cumulatively through M03 |

## Milestone exit gate

- The final [documentation step](step_11_documentation.md) updates maintained `doc/` through this milestone and is accepted before the next milestone; no future implementation is documented as available.

- Backend tracing design is recorded against current GPU docs and actual query/resource table code before implementation.
- AO/GI/reflections support the current production quality/settings/material-cutout behavior with appropriate neutral fallbacks.
- Public effect injection, separate scalar/HDR histories and denoise operate with correct rendered-scene/layer/view sequences.
- Both existing examples run with effect toggles/edits/resize/pause; disabling RT does not allocate/use unsupported tracing state.
- The existing SMAA spatial-AA facility is preserved as an optional public final-processing component; it is not MSAA and does not change the default scaling path.
- M03 review updates M04–M10, especially shared pass inputs, bake reuse, headless accumulation and settings serialization.

Provide the project lead with applicable Vulkan/Metal, no-RT and SDK-unavailable checks. The lead compiles/tests after each step; the agent performs static inspection and does not run builds/runtime checks unless explicitly instructed. Their instruction to continue accepts the preceding delivered step under the [execution protocol](../execution-protocol.md), without repeated confirmation or mandatory test artifacts. Record who checked what and leave unspecified coverage unspecified. Preserve production regression references through M08.

## Required code-versus-plan review

The technical review is the penultimate step; the cumulative documentation update is the final required step. Create `review.md` only after execution with actual revisions/contracts/results. Compare the code against every pending milestone and update affected summaries and step files, including dependencies, paths and validation. The review step lists milestone-specific comparisons; the documentation step closes the milestone under the cumulative documentation policy. Do not rewrite completed history to conceal a design change.

## M01 review handoff

Reuse public M01 pipeline/environment/query primitives, canonical/dense identity mapping, live codec and optional SDK FSR. Do not promote the eight-pair alpha table to an arbitrary RT material ABI. The project lead owns M03-01 decisions; general arrays, object/effect histories, denoise and optional SMAA remain real work. See [M01 review](../milestone_01_deferred_renderer/review.md) for source evidence, classifications and verification limits. Milestone order and product gate remain unchanged.

## Documentation closure order

The technical review now hands off to [M03-11](step_11_documentation.md), not directly to the next milestone. Earlier next-milestone handoff statements are historical and superseded by this order. See [documentation policy](../documentation-policy.md).

## M03-01 checkpoint delivery

[rt-design.md](rt-design.md) records the source comparison and recommended native execution, packed geometry, bounded resource arrays, cutout/transparency and mutation/history contracts. The project lead accepted the recommendation by continuation to M03-02. No new GPU API, shader or effect is delivered by M03-01. Pending steps and M04/M08 consumers carry this dependency. See [rt-gpu-data.md](rt-gpu-data.md) for the delivered M03-02 primitive implementation and validation limits.

## M03-03 implemented noise boundary

[blue-noise.md](blue-noise.md) records CPU rank views/sampling, Device-backed shared RGBA8 2D atlas residency and paired native helpers, with hash fallback and caller-owned sample sequencing. General physical image arrays are not introduced by this layout. M03-04 and later effects/bake use these actual contracts.

## M03-05 actual GI handoff

[rtgi.md](rtgi.md) records public RTGlobalIllumination, native Vulkan raygen/miss/closest-hit/any-hit with depth one, Metal compute queries, shared material tables and explicit receivers. RGBA16 output is incoming diffuse radiance/pi before receiver albedo with alpha validity; invalid zero restores diffuse IBL. RendererDeferred selects IBL/AO/GI independently of retained settings, preserves specular IBL and supports scaled GI inputs. History/filter/UI and baked presence policy remain pending. Reuse this exact normalization and matching-capacity snapshot ownership.

## M03-06 actual reflection handoff

Use [rt-reflections.md](rt-reflections.md): public RTReflections reuses retained scene tables and camera/explicit surfaces, with receiver roughness and camera position. Quality scales display output independently of G-buffer/internal/FSR extent. RGBA16 RGB is mean geometry-hit radiance before receiver BRDF, unpremultiplied; alpha is finite accepted hits/actualSamples. Preserve fractional confidence and rejected masks separately from GI binary validity and AO scalar visibility. The lighting pass blends prefiltered IBL with traced radiance using confidence and applies split-sum BRDF once. FullscreenPass supports an optional independent Metal sampler map; reflection texture16 shares nearest sampler0. Histories/filter/UI remain pending.

## M03-07 actual temporal handoff

Use [temporal.md](temporal.md): public TemporalAccumulator returns owned sampled scalar/HDR output or borrowed raw pass-through; SurfaceInputs/TemporalView expose inputs without private renderer access. Keys separate signal/view/layer, sceneFrameNumber advances actual work, maxFrames bounds per-pixel arithmetic/rolling/frozen accumulation. GI binary validity and reflection fractional confidence remain distinct. Renderer::invalidateHistories coordinates SDK and engine histories; resetUpscaler forwards for existing adapters. Exact extraction/settings signatures detect identity/value changes; in-place uploads/custom shaders and same-camera teleports require app notification. Object motion conservatively resets until previous-object vectors are added. Fresh history images and per-slot retained versions avoid overwrite; same ordered queue/submission discipline is required.

## M03-08 actual filter handoff

Use [denoise.md](denoise.md): RendererDeferred injects/exposes DenoiseFilter and resolves tracing -> temporal -> spatial -> lighting. Inputs reuse SurfaceInputs; output keeps scalar R16/HDR RGBA16 and independent source extent. GI validity and reflection confidence are explicit; filtered RGB stays unpremultiplied and never replaces temporal history. Camera thresholds use linear view Z, explicit profiles use world distance. Virtual settingsType/settingsInstance accessors support live catalog binding; M03-09 must register real controls/preferences and avoid copied settings values. Unsupported profiles return borrowed source; fresh output owners and per-slot descriptors remain alive through completion.

## M03-10 technical review delivery

[Actual technical review](review.md) records source contracts, corrected UI integration, example-specific active GI/reflection defaults, verification limits and edited pending M04–M10 steps. Await lead acceptance, then execute M03-11 cumulative maintained documentation before M04.

## Final M03 step delivered

[M03-11 documentation closure](documentation.md) updates maintained references,
tutorials and app/build guidance cumulatively through M03. This is the last step
in the ordered table; M03-10 is the accepted technical review. Await M03-11 lead
acceptance before entering M04-01; no additional M03 implementation is scheduled.

## Default renderer policy amendment (project lead direction, 2026-10-19)

The project lead later requested enabled GI mode and enabled reflections as the
global renderer defaults, including `draw_model_edit`. `RendererConfiguration`
now defaults to `GlobalIllumination`; default `RTGISettings` and
`RTReflectionSettings` are enabled. Explicit pass/settings/configuration still
override these values, and runtime availability/fallback remain separate from the
requested defaults. This supersedes only the earlier disabled-default policy;
the original M03 implementation history is preserved in its review and progress
record. See [current RTGI API](../../../../doc/api/draw/RTGlobalIllumination.md)
and [current reflection API](../../../../doc/api/draw/RTReflections.md).
