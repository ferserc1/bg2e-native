# Phase 4: adaptive accumulation and signal-change response

Dependency: Phase 3 implemented. Entry: P4.1. Exit: P5.1.

## Target and limits

Keep geometry-valid history when useful, but increase the current sample's weight when the signal changes. This handles AO/GI/reflection changes on stationary receivers that object motion vectors cannot identify. Adaptation is a denoising heuristic, not exact detection of all light transport changes. Do not change rays per pixel or introduce a denoiser SDK.

Implement basic confidence/clipping first (P4.1–P4.3), then add moments (P4.4). Preserve a selectable nonadaptive baseline and the Phase 0 fallback. Default enhanced adaptation stays off until the phase source work is complete; native quality validation and its absence must be reported before selecting release defaults in Phase 5.

## P4.1 — define settings and bounded weighting

1. Extend TemporalSettings with defaulted fields: `adaptiveEnabled=false`, `minCurrentWeight=0.03125f`, `reactiveCurrentWeight=0.25f`, `historyClipSigma=3.0f`. Validate finite ranges: weights in [0,1], reactiveCurrentWeight >= minCurrentWeight, sigma in [0,10]. Add `setAdaptiveSettings` or equivalent validated setters; changes invalidate histories once, never per frame.
2. Register fields in EffectSettings reflection, then add them to both cached settings comparison/serialization and the full reference serialization in exactly matching order. Keep old preference payloads readable using defaults; reject invalid new values through existing settings infrastructure. Update tests/draw/settings_codec probe cases.
3. For accepted history define `N = min(effectivePreviousCount,maxFrames)` and baseline current weight `alpha0 = 1/min(N+1,maxFrames)`. Preserve the existing result when adaptation is disabled.
4. During a permitted rolling update, choose `alpha = clamp(max(alpha0,minCurrentWeight,(1-geometricConfidence)*reactiveCurrentWeight,reactivity*reactiveCurrentWeight),0,1)`. Reactivity is defined below; absence of meaningful statistics gives reactivity=0. Geometry rejection forces alpha=1 and history count=1.
5. A saturated, geometrically valid pixel outside the Phase 0 window with freeze preference true remains frozen. Adaptation/clipping must not modify its color/moments. Freeze is a literal user preference after the window; do not silently promise continued lighting response. Global light/material edits still reset through existing policy.

## P4.2 — compute robust current neighborhood statistics

1. Gather a 3x3 current-signal neighborhood at signal resolution. Accept only finite, covered samples with the same nonzero surface token and compatible current depth/normal. Do not use neighboring background or another transparent receiver to set color bounds.
2. For visibility use clamped scalar values. For radiance use nonnegative luminance `dot(rgb,vec3(0.2126,0.7152,0.0722))` for change detection; keep RGB for output. Use epsilon=1e-6 for denominators/variance floors in these new formulas, with identical GLSL/Metal constants.
3. Compute weighted mean, variance=max(secondMoment-mean*mean,0), componentwise min/max, and valid sample count. Initial weights may be uniform for accepted neighbors. Fewer than two valid neighbors disables variance-driven clipping/reactivity for that pixel rather than forcing an arbitrary zero-width range.
4. For GI black is a valid signal when its coverage flag is valid. For reflections, confidence-zero misses are not equivalent to invalid roughness/coverage; only confident samples contribute radiance bounds. Preserve the current confidence channel separately.
5. Use current samples for the neighborhood, not already temporally resolved neighbors, to avoid read/write dependencies and feedback amplification.

## P4.3 — add clipping and initial reactivity

1. Scalar history clipping interval: intersect neighborhood min/max with mean +/- sigma*sqrt(variance+epsilon). With insufficient support, leave history unchanged.
2. For HDR initially clip historical luminance to the analogous interval, scaling historical RGB by the luminance ratio when the old luminance is above epsilon. For near-zero historical luminance, use a bounded componentwise neighborhood clamp. Preserve alpha/confidence. Document that this simple luminance strategy can still retain chromatic artifacts; do not claim perfect specular reconstruction.
3. Define reactivity from unclipped historical luminance versus neighborhood mean: `r = clamp((abs(Lh-mean)-sigmaStd)/(abs(Lh-mean)+sigmaStd+epsilon),0,1)`, where sigmaStd=historyClipSigma*sqrt(variance+epsilon). Insufficient neighborhood support gives r=0. High noise should reduce confidence in detecting a change rather than forcing a reset.
4. Use the P4.1 weight with the clipped historical value. Scalar/GI blend normally. Reflections keep confidence-premultiplied temporal mixing and unpremultiply only when confidence is nonzero. Do not introduce RGB/alpha coupling that darkens valid reflection misses.
5. Effective history count represents trust, not nominal elapsed frames: if alpha > alpha0, cap previous effective count to `max(1,1/max(alpha,epsilon)-1)` before adding the current sample. Bound by maxFrames and preserve shader count storage limits. Record this changed semantic in comments and probe expectations.

## P4.4 — add temporal moments as a separate increment

1. Allocate optional per-history `RG16F` moment images with sampled/storage usage; check device capabilities. If unsupported, retain adaptive neighborhood-only mode with explicit status. No mandatory RG32 requirement; offer higher precision only as a documented optional later optimization.
2. Store first and second moments of a bounded luminance statistic. For HDR use `clamp(log2(1+max(luminance,0)),0,16)` to avoid squaring unbounded HDR into half-float overflow; visibility uses its [0,1] scalar. The second HDR moment is therefore at most 256. Do not mix linear and log-domain variances. Reject nonfinite inputs before calculating moments and document these bounds in code/tests.
3. Reproject moments with the same accepted candidate taps/weights as color. Reject/reset moments when color history rejects. Blend moments with the same effective alpha; frozen pixels retain moments unchanged.
4. Use moment variance to inform change confidence only in its own statistic domain. Transform current neighborhood samples into that domain before comparison. Leave linear-luminance clipping bounds from P4.3 separate. With fewer than four effective samples, use current spatial variance as the conservative estimate; do not trust a newly initialized zero temporal variance.
5. Add layout bindings and dummy images only to the enhanced versioned kernel. Preserve custom v1 compatibility, old returned owners, abandoned recording handling and no-moments fallback. Update pooled binding identity if descriptors differ.
6. Do not share moments between AO, GI, reflected radiance, views, or layers. They track different stochastic signals. Keep memory bounded by existing maxHistories/pruning behavior.

## P4.5 — regressions and visual acceptance

Source/native probe cases: unchanged noisy input, scalar step change on a stationary receiver, stable valid-black GI, reflection hit/miss/confidence transitions, single bright outlier, HDR large values without NaN/Inf/half overflow, red-to-blue change with similar luminance (document remaining limit), low-confidence 2x2 footprint, neighborhood with one valid sample, maxFrames=1, disabled adaptation parity, freeze/thaw exact window, dropped history, and unavailable moment format.

Native scenarios when authorized: move an occluder outside a receiver's screen footprint but within AO influence; move an object visible only in a mirror; move a bright indirect contributor; pan camera across fine texture. Compare static noise, visible lag, trails and disocclusion recovery with the same ray count and denoise settings. Do not tune only for one noiseless step fixture.

Acceptance: changed signals respond faster than the nonadaptive rolling baseline without forcing global resets; stable noise remains acceptable. Quantify convergence frames where possible, but report noise/bias/ghosting tradeoffs. No claim of instantaneous physically correct indirect updates.

## P4.6 — cost and handoff

Neighborhood-only adaptation adds up to nine current taps per pixel plus arithmetic, potentially repeated across signals. Moments add one RG16F image per retained history, approximately 8.29 MB at 1080p before in-flight/pool retention, plus history sampling/writing. Do not add more rays as part of this phase. Debug views: current weight, reactivity, variance and effective count.

Review paired shaders, settings export/import/reference parity, exact disabled baseline, freeze semantics and confidence output. Record completed versus unverified work, API defaults and pointer P5.1 in progress.md.
