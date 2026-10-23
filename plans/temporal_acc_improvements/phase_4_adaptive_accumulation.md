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

### P4.1 source implementation and interface decision (2026-10-23)

P4.1 is implemented_unverified. TemporalSettings appends the four specified defaults;
setAdaptiveSettings validates the full tuple before mutation and invalidates exactly
once per changed tuple, with no-op equality preserved. Reflection adds the four
fields plus read-only adaptiveAvailable/adaptiveStatus. RenderSettingsPreferences
validates complete proposed pairs before writes; reflected weight setters stage
companion bounds so valid low/high batches work in either order. Direct callers
use the strict atomic setter. Existing catalog property/post-commit resets are
unchanged; no per-frame setters are introduced. Missing old payload fields retain
live/default values. Cached equality/payload and full-reference serialization append
all four fields in identical order after normalThreshold.

Compatibility requires separate optional draw.temporal.v4 and trailing adaptive
interface/prefix configuration. V4's 368-byte uniform is the exact v3 prefix plus
vec4(minimum,reactive,reservedSigma,enabled) at offset 352. V1/v2/v3 retain
208/288/352-byte layouts, original binding maps and their own selected pipelines.
New paired camera scalar/HDR production/diagnostic wrappers opt into the shared
v4-only weighting block. V4 requires v3 support and explicit requested adaptation;
unsupported v4 falls back to nonadaptive v3. Initialization failure is latched
without ordinary retries. The owned renderer provisions v4 independently; replay
and adaptation remain default off. Custom/legacy kernels never receive new bytes
unless they explicitly opt into v4. Per-key metadataMode=3 distinguishes v4;
mode switches restart. Cleanup releases new pipeline/shader/layout owners after
its existing drain. No new image, dependency, synchronization or CMake change.

V4 applies the specified bounded alpha to accepted rolling samples with
reactivity=0; historyClipSigma is stored/serialized but unused until clipping.
Literal saturated freeze skips the override. Geometry rejection still restarts
raw signal/count one. Adaptive-disabled v3 code remains byte-identical after
removing v4-only compile-time blocks; existing temporal scalar/GI/reflection
confidence math is unchanged except oldWeight assignment in opted-in v4. Counts
retain the Phase 3 minimum/increment semantics until P4.3 trust-count adjustment.
No P4.2 neighborhood, P4.3 clipping or P4.4 moments are implemented here.

Executed checks: phase4_source_checks.py (settings/ranges/paired ABI and shader
weight blocks/signature field order/staging permutations/independent alpha/freeze
math, GPL/includes/delimiters), phase3_source_checks.py and phase2_source_checks.py,
exact saved pre-step shared-source comparison with v4 blocks stripped, and
`git diff --check`. All pass as source/reference evidence, not compiler results.
Expanded C++ source fixtures: actual temporal setters/reflection/codec defaults,
no-op and atomic invalidation, valid import orders/round-trip/old fields/invalid
pairs/nonfinite values; cached/full signature changes for each field; optional
v4 all-mask scalar native probe with one-tap alpha .75 versus nonadaptive .5.

| Acceptance | Source/reference | Vulkan native | Metal native |
|---|---|---|---|
| ABI/selection/failure/legacy fallback | reviewed; static checks pass | not_run | not_run |
| Defaults, ranges, no-op/reset, codec/old payloads | source fixtures + independent reference pass | C++ CPU fixture not_run | C++ CPU fixture not_run |
| Cached/full signatures and all new fields | source order checks + native fixture source | not_run | not_run |
| Bounded support weight and literal freeze/disabled parity | independent reference + exact disabled source parity | not_run | not_run |
| Scalar v4 pixels/diagnostic availability | optional all-mask fixture source | not_run | not_run |
| Actual adaptive HDR, queued ownership, freeze/thaw, noisy/visual quality and cost | existing paths preserved; extended native acceptance pending | not_run | not_run |

No native support, quality, noise/lag improvement or performance claim is made.
Defaults stay off. Future authorized host entries are in the temporal/settings
probe READMEs and API doc. Next exact step: P4.2 only. Meaningful signal-change
reactivity still requires P4.2/P4.3; geometric support is not an indirect-light
change detector.

## P4.2 — compute robust current neighborhood statistics

1. Gather a 3x3 current-signal neighborhood at signal resolution. Accept only finite, covered samples with the same nonzero surface token and compatible current depth/normal. Do not use neighboring background or another transparent receiver to set color bounds.
2. For visibility use clamped scalar values. For radiance use nonnegative luminance `dot(rgb,vec3(0.2126,0.7152,0.0722))` for change detection; keep RGB for output. Use epsilon=1e-6 for denominators/variance floors in these new formulas, with identical GLSL/Metal constants.
3. Compute weighted mean, variance=max(secondMoment-mean*mean,0), componentwise min/max, and valid sample count. Initial weights may be uniform for accepted neighbors. Fewer than two valid neighbors disables variance-driven clipping/reactivity for that pixel rather than forcing an arbitrary zero-width range.
4. For GI black is a valid signal when its coverage flag is valid. For reflections, confidence-zero misses are not equivalent to invalid roughness/coverage; only confident samples contribute radiance bounds. Preserve the current confidence channel separately.
5. Use current samples for the neighborhood, not already temporally resolved neighbors, to avoid read/write dependencies and feedback amplification.

### P4.2 source implementation and geometry contract (2026-10-23)

P4.2 is implemented_unverified. Paired temporal_current_statistics.glsl/.metal
helpers are included only by TEMPORAL_ADAPTIVE v4 and called only for accepted,
adaptive, nonfrozen updates. They return internal CurrentStatistics: scalar/
luminance mean, secondMoment, variance, statistic min/max, componentwise color
min/max, sampleCount and varianceValid. Every accepted neighbor has uniform weight
one. The nine signal offsets form a bounded row-major 3x3; out-of-bounds offsets
are skipped without clamping/duplicating edges. Each signal texel-center UV maps
to its own current receiver pixel at extent.xy; all guides and token come from
that receiver. No previous/temporally resolved/output image is read by the helper.

Current-depth compatibility uses existing bindings, without a new current-view
matrix/ABI: require finite D16/D32 receiver and metadata depth in [0,1), agreement
within metadataRejection.x, positive finite inverse-VP homogeneous w > 1e-6, finite
reconstructed world position and distance from the center <=
absolute + relative*length(centerPosition-currentCameraPosition). Current normals
are decoded/normalized and checked against the configured cosine threshold.
Every receiver has the same nonzero token as the center, so different surfaces/
transparent domains cannot set bounds. This metric world-distance gate is a
conservative current-depth consistency test, also restricting lateral separation;
it is not a new NDC threshold, tolerance widening or previous-view depth test.
Thin/grazing/reduced-resolution geometry may leave fewer neighbors. Prefer
insufficient support over relaxing the existing tolerances.

All sampled signal channels must be finite. Visibility clamps to [0,1], with
componentwise scalar bounds replicated across RGB. GI requires alpha>.5 and treats
black as valid. Reflection requires finite roughness <= maxRoughness and clamped
confidence >1e-6 to contribute radiance statistics; confidence-zero misses remain
valid temporal samples in the unchanged main kernel. Radiance statistic is
max(dot(rawRGB,(.2126,.7152,.0722)),0); raw finite RGB supplies component bounds,
while alpha/confidence is neither averaged into luminance nor modified. No RGB
color clamp is applied to the actual signal here.

CURRENT_STATISTICS_EPSILON=1e-6 in both helpers protects denominators, valid normal
lengths, positive homogeneous w and reflection-confidence support. Variance is
max(secondMoment-mean*mean,0), with varianceValid requiring >=2 samples and finite
results. Zero samples return zero-initialized fields/count=0/varianceValid=false;
one sample has bounds but varianceValid=false. P4.3 must test that flag before
variance-driven clipping/reactivity instead of treating the fallback as a
zero-width interval. Its future sqrt(variance+epsilon) remains separate.
Float32 accumulation of finite FP16 input avoids native-half arithmetic/squaring
limits; source tests check the finite HDR maximum and a bright outlier.

P4.2 deliberately does not consume statistics in output/alpha yet. Reactivity is
still zero and P4.1 weights/count/freeze/blend math remain unchanged. Unused helper
results/calls may be optimized away until P4.3 consumes them. No execution or
runtime 3x3 cost is claimed at this source boundary. No diagnostic/statistics image,
new resource binding/uniform/settings/CPU ownership path or persistent allocation
is introduced. When consumed, the bounded source budget is up to nine current
signal reads plus per-candidate token/depth/normal and optional roughness reads;
this is not just nine total texture reads and has not been measured.

Executed evidence: neighborhood_reference.py (independent supplied-world-guide
math, exact mapping, finite/coverage/identity/depth/normal filters, 1x1/edge/odd/
mismatched extents, scalar clamp/known variance, valid-black/missing GI, confident/
miss/roughness-limited reflection, rotated normals, HDR/outlier and insufficient
support); normalized paired helper-body source parity; phase4_source_checks.py
(includes that reference), phase3_source_checks.py, phase2_source_checks.py and
`git diff --check`. Saved pre-step comparisons show unchanged disabled-v3 source
tokens and exact pre-step P4.1 source after removing only helper include/call.
These are reference/source checks, not compilation or shader execution. The CPU
reference supplies independently constructed world guides; it does not execute
the shader inverse-VP reconstruction or prove native projection conventions.

Native acceptance remains not_run on Vulkan and Metal: shader compilation/nested
include deployment, actual receiver-center reconstruction at odd/reduced sizes,
normal-map and transparent-domain rejection, scalar/GI/reflection statistics,
HDR precision, output parity/freeze and profiling. An authorized native statistics
inspection must use an explicit temporary test wrapper that exports the helper's
fields to test storage, or the later P4.3 consumer; unchanged P4.1 output alone
cannot prove that statistics ran or are correct. No such wrapper was compiled or
executed here. Existing fixture source/readmes remain the entry points for future
pixel/baseline regression work. No native support/quality/performance pass claimed.

Next exact step: P4.3 only. Consume varianceValid-gated current statistics for
clipping and initial signal reactivity, preserve confidence and literal freeze,
and add the specified adaptive trust-count cap. Do not add P4.4 moments yet.

## P4.3 — add clipping and initial reactivity

1. Scalar history clipping interval: intersect neighborhood min/max with mean +/- sigma*sqrt(variance+epsilon). With insufficient support, leave history unchanged.
2. For HDR initially clip historical luminance to the analogous interval, scaling historical RGB by the luminance ratio when the old luminance is above epsilon. For near-zero historical luminance, use a bounded componentwise neighborhood clamp. Preserve alpha/confidence. Document that this simple luminance strategy can still retain chromatic artifacts; do not claim perfect specular reconstruction.
3. Define reactivity from unclipped historical luminance versus neighborhood mean: `r = clamp((abs(Lh-mean)-sigmaStd)/(abs(Lh-mean)+sigmaStd+epsilon),0,1)`, where sigmaStd=historyClipSigma*sqrt(variance+epsilon). Insufficient neighborhood support gives r=0. High noise should reduce confidence in detecting a change rather than forcing a reset.
4. Use the P4.1 weight with the clipped historical value. Scalar/GI blend normally. Reflections keep confidence-premultiplied temporal mixing and unpremultiply only when confidence is nonzero. Do not introduce RGB/alpha coupling that darkens valid reflection misses.
5. Effective history count represents trust, not nominal elapsed frames: if alpha > alpha0, cap previous effective count to `max(1,1/max(alpha,epsilon)-1)` before adding the current sample. Bound by maxFrames and preserve shader count storage limits. Record this changed semantic in comments and probe expectations.

### P4.3 source implementation and count/storage contract (2026-10-23)

P4.3 is implemented_unverified. Paired temporal_history_response helpers consume
varianceValid-gated statistics only in accepted/enabled/nonfrozen v4 updates.
The scalar/luminance interval intersects current statistic min/max with
mean +/- sigma*sqrt(variance+1e-6). Empty/nonfinite intersections skip response.
Scalar clips its value; HDR scales RGB by the clipped/original nonnegative
luminance ratio above epsilon and uses current component bounds near zero.
Alpha/confidence is unchanged. Reactivity is computed from the original historical
scalar/luminance using the specified delta/sigmaStd formula before any clipped
value is fed to the existing confidence-preserving blend.

Storage safety refinement: RGB after HDR scaling is bounded to [-65504,65504].
Even bounded finite FP16 input luminance can demand an overflowing saturated color
channel under luminance scaling. This explicit storage-range guard avoids that
new overflow; it can prevent the exact target luminance and does not solve hue
artifacts. Equal-luminance red/blue changes can remain undetected. Native quality/
precision acceptance is pending. Insufficient statistics keeps history unchanged
and reactivity zero; P4.1 minimum/support weighting can still affect such pixels.

Chosen alpha uses alpha0, minimum, geometric support and measured reactivity. If
alpha > alpha0, oldCount=min(oldCount,max(1,1/max(alpha,1e-6)-1)); the existing final
increment then occurs exactly once, bounded by maxFrames. Stored per-pixel count
now means effective trust, can be fractional and is quantized by RGBA16F. CPU
TemporalResult.frameCount remains nominal scene chronology. Valid accepted alpha
one may store count two (minimum old trust one), while geometry rejection restarts
at one and uncovered remains zero. No count readback or new CPU policy exists.

Saturated valid pixels outside the Phase 0 motion window bypass gathering,
clipping, reactivity and trust cap. Default-off/v3 source tokens remain unchanged.
Response/support/floor weighting may keep trust below maxFrames, particularly if
minCurrentWeight > 1/maxFrames, so CPU nominal convergence does not guarantee
saturation/freezing. This follows the required trust-count semantics; no authored
freeze/maxFrames or rolling-window/reset rule is changed. No new ABI, setting,
image/moment storage, renderer/backend dependency, ownership or synchronization.
P4.2 gathering is now used by output and may have real runtime cost; it is bounded
but no timing claim is made.

Executed: history_response_reference.py and phase4_source_checks.py (paired helpers/
weighting, scalar noise/step/insufficient support, freeze/rejection/disabled bypass,
trust quantization, HDR/near-zero/FP16/chromatic limits, reflection confidence),
phase3_source_checks.py, phase2_source_checks.py and git diff --check. Saved pre-step
comparison proves unchanged v3 tokens after stripping only adaptive blocks.
No C++ or shader execution occurred. Native footprint source now discriminates
constant-current clipping, exposes test-only retained count guides, and checks
eight stable samples then trust two vs CPU nominal nine, saturation freeze and
motion thaw. Its v3/default mask expectations remain unchanged.

| Acceptance | Source/reference | Vulkan | Metal |
|---|---|---|---|
| Clipping/reactivity from unclipped history and low support | independent math and source parity pass | not_run | not_run |
| Confidence/valid-black/miss behavior and HDR storage limit | independent math/source pass | not_run | not_run |
| Trust count/half storage/nominal frame distinction | independent reference + C++ fixture source | not_run | not_run |
| Disabled baseline/freeze/thaw/rejection | source tokens/reference + native fixture source | not_run | not_run |
| Compile/deploy, actual stats, queued HDR/transparent/raster behavior | source reviewed | not_run | not_run |
| Stable noise, actual indirect-light response, chromatic artifacts, cost | future P4.5/native scenarios | not_run | not_run |

Replay/adaptation remain default off. No native quality or speed pass claimed.
Next exact step: P4.4 optional moment images/statistics, preserving this
neighborhood-only v4 path and legacy/default behavior. Do not execute P4.5/P4.6
acceptance or benchmarks without their requested source step/native authorization.

## P4.4 — add temporal moments as a separate increment

1. Allocate optional per-history `RG16F` moment images with sampled/storage usage; check device capabilities. If unsupported, retain adaptive neighborhood-only mode with explicit status. No mandatory RG32 requirement; offer higher precision only as a documented optional later optimization.
2. Store first and second moments of a bounded luminance statistic. For HDR use `clamp(log2(1+max(luminance,0)),0,16)` to avoid squaring unbounded HDR into half-float overflow; visibility uses its [0,1] scalar. The second HDR moment is therefore at most 256. Do not mix linear and log-domain variances. Reject nonfinite inputs before calculating moments and document these bounds in code/tests.
3. Reproject moments with the same accepted candidate taps/weights as color. Reject/reset moments when color history rejects. Blend moments with the same effective alpha; frozen pixels retain moments unchanged.
4. Use moment variance to inform change confidence only in its own statistic domain. Transform current neighborhood samples into that domain before comparison. Leave linear-luminance clipping bounds from P4.3 separate. With fewer than four effective samples, use current spatial variance as the conservative estimate; do not trust a newly initialized zero temporal variance.
5. Add layout bindings and dummy images only to the enhanced versioned kernel. Preserve custom v1 compatibility, old returned owners, abandoned recording handling and no-moments fallback. Update pooled binding identity if descriptors differ.
6. Do not share moments between AO, GI, reflected radiance, views, or layers. They track different stochastic signals. Keep memory bounded by existing maxHistories/pruning behavior.

### P4.4 source implementation and moment-domain contract (2026-10-23)

P4.4 is implemented_unverified. Optional trailing momentShaderInterface/prefix
select draw.temporal.v5, requiring available v4 plus RG16F sampled/storage/transfer
capability. The 368-byte v4 uniform is unchanged. New sampled 19/Metal17 and
storage 20/Metal18 bindings carry prior/current RG16F moments; separate paired
camera scalar/HDR production/diagnostic wrappers declare the new version. Old
interfaces receive only their own bindings/bytes. The canonical diagnostic family
can fail independently of core v5. Format/layout/kernel/typed-dummy initialization
failure latches an explicit neighborhood-only v4 fallback without per-frame retry.
The owned renderer provisions v5, but replay/adaptation stay off by default.
An empty external moment interface explicitly retains v4 as a selectable baseline.

One optional full moment image is allocated per active signal/view/layer history
at signal extent. Per-key mode=4, existing sequence/profile/generation/extent
admission and a missing-moment predecessor guard prevent incompatible replay.
Prior/current owners live in History and slot invocations before dispatch; the
pool lease identity includes v5's distinct layout. One typed 1x1 zero-moment image
is created only on successful optional initialization and used only behind the
existing no-history flags. Cleanup releases it and pipelines after the existing
drain. Existing maxHistories/pruning/retirement rules remain; no cross-key sharing,
ordinary wait/readback, settings, CMake or GPU/backend changes are introduced.

A finite current statistic maps visibility to [0,1], HDR to bounded log2(1+L) [0,16].
Squared statistics are <=1/256 respectively, safe in RG16F. Reflection temporal
misses use zero instead of arbitrary zero-confidence RGB; eligible hits retain
unpremultiplied radiance statistics. Moments describe raw input statistics, not
clipped accumulated color. Current accepted neighbors are transformed individually
before moment-domain spatial averaging; linear statistics remain separate.

Moment validation is part of joint candidate validity: nonfinite/out-of-range
pairs reject that color+moment tap locally, leaving identical valid taps/support/
normalized weights for both. Half-quantized second moment need not exceed mean^2;
variance clamps negative roundoff to zero. Current moments initialize on rejected
history and clear on uncovered signal. Accepted moments blend with the exact
color alpha; frozen pixels keep the reprojected old pair without current blending.
Retained image owners are immutable (outputs are newly leased), not overwritten.

V5's reactivity compares previous moment mean against current spatial moment mean.
Below four effective prior samples, only current spatial moment variance is used.
At >=4, max(spatialMomentVariance,temporalMomentVariance) is a conservative noise
estimate. Insufficient current support disables this reactivity. All operands and
sigmaStd stay in the bounded statistic domain; P4.3 linear-luminance clipping uses
its original linear bounds/variance. Trust cap, geometric/minimum weight floors,
alpha/confidence mixing and saturated freeze remain; v4 is unchanged when moments
are absent. Log moments do not solve equal-luminance chromatic artifacts.

Extra image content is 4*S bytes per unique moment image: 8.2944 MB at 1080p or
33.1776 MB at 4K, before predecessors/next frames/queued slots/external/pool/driver
retention. Scalar/HDR history sets become 30/36*S bytes. Do not share or multiply
the metadata-domain estimate per signal, but count moments separately per key.
The pool's existing pixelFormatBytes recognizes RG16F as four bytes; idle retention
limits are not hard live allocation caps. No measured peak/time/speed claim.

Executed/passed: moments_reference.py and Phase 4/3/2 source/reference checks,
paired helper/source contracts, GPL/includes/delimiters and git diff --check.
Saved pre-step comparisons with nested moment branches stripped prove unchanged
v4 reprojection/current-neighborhood tokens. New balanced feature-source utility
keeps v3/v4 baseline assertions valid across nested v5 branches. None compiles or
executes C++/GLSL/Metal. The expanded native footprint source's optional fourth
argument requests v5, compares all scalar mask moment pairs with analytic alpha,
retained owners and exact frozen pairs, while earlier v3/v4 variants remain.

| Acceptance | Source/reference | Vulkan | Metal |
|---|---|---|---|
| RG16F capability, typed dummy, mode/layout/fallback | source audit pass | not_run | not_run |
| Scalar/log-HDR finite bounds and same-domain variance | independent math/source parity pass | not_run | not_run |
| Joint tap validity, same weights/alpha, reset/freeze | independent reference + native fixture source | not_run | not_run |
| Retained owners/per-key isolation/pooled identities | ownership audit and fixture source | queued native not_run | queued native not_run |
| Exact no-moments v4 and v3 fallback | source token parity pass | native fallback not_run | native fallback not_run |
| Native scalar/HDR moments, invalid/drop/resize/mode/half precision | source fixtures/acceptance scenarios recorded | not_run | not_run |
| Noise/lag/lighting/visual quality and cost | P4.5/P4.6 pending | not_run | not_run |

Next exact step: P4.5 regression/visual acceptance source work only. Do not claim
native support/quality on source evidence or enable defaults; compilation/native
execution/benchmarks remain unauthorized until explicitly requested.

## P4.5 — regressions and visual acceptance

Source/native probe cases: unchanged noisy input, scalar step change on a stationary receiver, stable valid-black GI, reflection hit/miss/confidence transitions, single bright outlier, HDR large values without NaN/Inf/half overflow, red-to-blue change with similar luminance (document remaining limit), low-confidence 2x2 footprint, neighborhood with one valid sample, maxFrames=1, disabled adaptation parity, freeze/thaw exact window, dropped history, and unavailable moment format.

Native scenarios when authorized: move an occluder outside a receiver's screen footprint but within AO influence; move an object visible only in a mirror; move a bright indirect contributor; pan camera across fine texture. Compare static noise, visible lag, trails and disocclusion recovery with the same ray count and denoise settings. Do not tune only for one noiseless step fixture.

Acceptance: changed signals respond faster than the nonadaptive rolling baseline without forcing global resets; stable noise remains acceptable. Quantify convergence frames where possible, but report noise/bias/ghosting tradeoffs. No claim of instantaneous physically correct indirect updates.

### P4.5 source acceptance and regression ledger (2026-10-23)

P4.5 is implemented_unverified: regression/reference sources and the reproducible
native/visual protocol are complete; no actual visual acceptance is claimed.
Production C++/headers/shaders were hash-compared to the P4.5 entry snapshot and
remain unchanged. No thresholds, defaults, ray counts, moments policy or rendering
behavior were tuned in this step. P4.6 remains separate.

New adaptive_probe.hpp supplies runAdaptiveTemporalProbe(engine,shaderRoot,
moments=false,forceMomentFailure=false), a stopped synthetic 3x3 compute fixture.
It records matched adaptive and disabled-v3 streams, uses wide synthetic geometry
tolerance to admit a real multi-sample neighborhood, and checks stationary scalar
steps without accumulator-generation reset, noisy temporal input, one same-token
neighbor, valid-black GI, HDR/bright outlier finiteness, reflection hit/miss/fractional
confidence, the known red/blue luminance limit and maxFrames=1. V5 is optional.
The forced-missing moment family tests latched optional failure/v4 recording; it is
not a mock of unsupported native RG16F format capabilities. Actual format fallback
must also be observed on a device/test host that reports that condition.

runQueuedTemporalReprojectionProbe now accepts trailing adaptive=false,moments=false
flags. Original v3 calls keep their inputs/expectations. Adaptive variants select
v4/v5 explicitly, with narrow geometry tolerance admitting one current neighbor;
that case leaves clipping/reactivity off and has an analytic shared baseline
weight >=.25. The fixture still submits two/three slots before waiting, without
claiming overlap. It adds separate retained RG16F means/second moments for scalar,
GI and reflection, independent view/layer statistics and distinct live moment
handles, abandoned-predecessor restart and retained old-owner checks. All inputs,
including extra key signals, are uploaded before queued batches. No readback of
unsubmitted/abandoned output is attempted. Fixture waits/readback remain stopped
manual-test work, never an ordinary-frame production path.

adaptive_sequence_reference.py adds half-quantized temporal streams on top of the
independent neighborhood/clipping/moment references. It covers noisy/stationary
change, bounded HDR/outliers, valid-black/confidence, support, literal freeze and
exact 3*maxFrames settling, abandoned/dropped/gapped history, maxFrames=1, invalid
current samples, unavailable moments, disabled selection and per-key immutable
reference snapshots. It does not execute C++ admission or prove GPU ownership;
real queued-source fixtures supply the corresponding future native tests.

Executed: phase4_source_checks.py (all unit/sequence references and fixture source
GPL/includes/delimiters), phase3_source_checks.py, phase2_source_checks.py,
production-file hash comparison and git diff --check. All passed. In the explicitly
controlled reference-only scalar case (32 samples at .25, then .75, maxFrames=32,
freeze off), the first frame of a three-sample error<=.01 window occurs after 123/1/1
new samples for v3/v4/v5. Alternating deterministic .25/.75 checker noise gives
identical tail RMSE about .005524 across the three reference modes. These numbers
are half-quantized model outputs, not engine/shader/native quality measurements or
runtime benchmarks, and the noiseless flat step benefits directly from clipping.

| Required case | Executable reference / native source entry | Source evidence | Vulkan native | Metal native |
|---|---|---|---|---|
| Unchanged noisy input | adaptive_sequence_reference; runAdaptiveTemporalProbe | reference pass; fixture source | not_run | not_run |
| Stationary scalar step / no global reset | sequence + matched adaptive_probe | reference pass; accumulator-generation assertion source | not_run | not_run |
| Valid-black GI / missing coverage | neighborhood/response/sequence; adaptive + queued probes | reference pass; native assertions source | not_run | not_run |
| Reflection hits/misses/fractional confidence | response/sequence; adaptive + queued probes | reference pass; native assertions source | not_run | not_run |
| Single bright outlier / HDR finite storage | neighborhood/response/sequence; adaptive_probe | reference pass; finite native assertions source | not_run | not_run |
| Similar-luminance red-to-blue limit | response oracle; adaptive_probe | reference records retained chromatic history, not a quality pass | not_run | not_run |
| Low support 2x2 footprint / one current neighbor | footprint/reference/sequence + footprint/queued/adaptive probes | reference pass; mask and insufficient-support native sources | not_run | not_run |
| maxFrames=1 | response/sequence + adaptive_probe | reference pass; source assertion | not_run | not_run |
| Disabled adaptation baseline | exact v3 source parity + matched/queued v3 variants | source/reference pass | not_run | not_run |
| Freeze/thaw and exact settling window | sequence + footprint_probe + existing temporal fixture | reference pass; literal freeze/motion native sources | not_run | not_run |
| Dropped/abandoned/update-only gap | sequence + queued probe all modes | reference pass; native chronology/restart sources | not_run | not_run |
| Moment unavailability | source cap/selection guards + sequence + forced-failure adaptive probe | reference/source pass; forced missing-module fixture source | not_run; native format case pending | not_run; native format case pending |
| Per-signal/view/layer retention / 2–3 slots | sequence + queued v5 probe | immutable reference snapshots; native owner assertions source | not_run | not_run |
| Settings/codec/cached-reference signatures | settings_codec and temporal_invalidation fixtures; source checks | source/range/order references pass | C++ CPU/native fixtures not_run | C++ CPU/native fixtures not_run |

#### Native execution entries and interpretation (not run)

In an explicitly authorized stopped initialized host, use
runAdaptiveTemporalProbe(engine,root) for v4, (engine,root,true) for v5 and
(engine,root,false,true) for a deliberately unavailable optional moment family.
Run runQueuedTemporalReprojectionProbe(engine,root), (...,true) and (...,true,true)
for v3/v4/v5; each covers two and three slots. Retain runTemporalFootprintProbe in
all three variants plus existing temporal, rigid and settings fixtures. No CMake
target or automatic runner is introduced. False/unavailable support is recorded as
unsupported, not pass; format/allocation errors are errors/unsupported with actual
reason, never a successful fallback assertion. Missing RG16F support is distinct
from an intentionally missing module; record which branch actually ran.

These synthetic compute fixtures need no RT/FSR. Real lighting scenarios require
their respective AO/GI/reflection capability, independently on each backend. An
unsupported lighting effect with raster/IBL fallback does not validate that effect's
adaptive acceptance. Native C++/shader compilation, every fixture and all scenarios
below are not_run on both backends.

#### Reproducible visual acceptance protocol (not run)

Compare v3 nonadaptive rolling, v4 neighborhood-only and v5 moment-assisted output
at identical internal/signal/output extents, active signal/view/layer settings,
ray counts, denoiser, exposure, light/environment and camera/object trajectories.
Export authored settings and identify shader/source build, backend/device/driver,
actual formats and capability/fallback status. Fix an exposed stochastic seed if
available; otherwise record that exact stochastic replay is unavailable and repeat
captures rather than assuming deterministic noise. Include fallback paths as
separate runs, not disguised enhanced-path passes.

Use freezeAtLimit=false for primary response/noise comparisons, and explicit
freeze/thaw runs for that preference. Warm a stable scene before each identical
motion event; do not reset the history merely to make a variant look cleaner.
Predeclare changed-receiver and unaffected-background ROIs, converged reference,
error metric and common tolerance above the measured stationary noise floor.
Count rendered scene contributions and report the first and confirmation frame
of a three-consecutive-frame window inside tolerance;
presentation/UI-only frames do not count. Record lag, residual noise/bias,
chromatic error, ghost/trail footprint and local rejection alongside convergence
frames. Do not choose a different tolerance per variant after observing results.
Current token/motion/LocalHistory diagnostics can explain correspondence in a
separate diagnostic run; they do not expose adaptive alpha/variance/trust in
production yet (those views belong to P4.6). CPU nominal frameCount cannot replace
pixel/ROI evidence or the per-pixel trust value.

| Visual scene | Controlled change and observation | Acceptance / explicit remaining limit | Vulkan | Metal |
|---|---|---|---|---|
| AO influence outside receiver footprint | Move an occluder outside the receiver's own screen footprint but within AO influence, stationary camera/receiver | Receiver keeps identity while changed AO responds faster than nonadaptive rolling; unaffected background convergence survives repeated transforms | not_run | not_run |
| Reflected-only object | Move an object seen only in a reflective receiver, keeping the receiver below maxRoughness | Compare hit/miss confidence and trail/response without global resets; virtual reflection/FSR motion is still approximate | not_run | not_run |
| Indirect bright contributor | Move a bright indirect contributor while observing a stationary receiving wall, including black-to-lit and lit-to-black | Faster response with acceptable stable noise/bias; no instantaneous physical-correctness claim | not_run | not_run |
| Fine texture camera pan | Repeat a camera pan over fine material/geometry at reduced signal extents | No invalid foreground/background mixing; report aliasing, over-rejection and flicker rather than broadening tolerances | not_run | not_run |
| Alpha/transparent/depth edge | Move thin/cutout/transparent foreground across a distant wall, same active layer ordering | Preserve exact receiver-domain metadata; no old foreground radiance in uncovered wall; transparent/refraction FSR motion remains approximate | not_run | not_run |
| Same-luminance color change / HDR outlier | Similar-luminance red/blue change and a single high-HDR contributor | Quantify residual chromatic lag, clipping/storage saturation and noise tradeoff; luminance-only method does not guarantee detection | not_run | not_run |
| Freeze preference / motion renewal | Saturate a valid pixel, edit signal while frozen, move repeatedly and stop | Literal frozen output before motion, rolling during and exactly 3*maxFrames later scene records; trust may stay unsaturated under continued response | not_run | not_run |

Global generation staying unchanged is necessary but not sufficient. A native
quality pass requires the actual pixels/ROI behavior, acceptable stationary noise
and changed-signal response with equal work settings. Record remaining noise,
precision, chromatic or motion artifacts as limits. No defaults or tuning choices
are changed here; cost/adaptive diagnostic source review is P4.6 next.

## P4.6 — cost and handoff

Neighborhood-only adaptation adds up to nine current taps per pixel plus arithmetic, potentially repeated across signals. Moments add one RG16F image per retained history, approximately 8.29 MB at 1080p before in-flight/pool retention, plus history sampling/writing. Do not add more rays as part of this phase. Debug views: current weight, reactivity, variance and effective count.

Review paired shaders, settings export/import/reference parity, exact disabled baseline, freeze semantics and confidence output. Record completed versus unverified work, API defaults and pointer P5.1 in progress.md.

### P4.6 final source/diagnostic/cost review (2026-10-23)

Phase 4 source implementation is complete, implemented_unverified. P5.1 is next.
No native support, visual acceptance, tuned defaults or measured cost is implied.

#### Optional metric capture contract

Append CurrentWeight/Reactivity/Variance/EffectiveCount enum values 6–9, preserving
old values. A trailing diagnoseAdaptive=false input requests a separate optional
canonical v4/v5 metrics family; both capture request flags together throw before
recording. Families have the same 368-byte uniform plus only their own storage
21/Metal19 binding, no acceptance output. Old/core/custom kernels/layouts are
unchanged. New display uses independent pipeline/layout with the existing 16-byte
uint4 resource ABI. Display or per-version capture initialization failure is
latched independently, without disabling v4/v5 core or ordinary-frame retries.
Read-only availability/status are separate from requested enabled state and exact
current capture availability; a missing selected v5 capture can be amber even if
v4 capture support exists. Renderer availability also checks the metadata pass.

Only the latched signal and exact layer request capture. Current recording owner,
scene sequence and metadata token owner must match before display consumes it.
Surface-view classification stays restricted to old IDs 3–5. A missing capture
binds the existing typed float zero image with the availability bit off; display
is amber rather than reading stale statistics. All invocations retain capture/
display/metadata owners before dispatch through existing slot retirement. Debug
selection does not affect metadataMode, authored history signature or reset policy.

RGBA metric encoding is actual alpha, actual reactivity, variance/(1+variance),
post-response trust/maxFrames. Values are bounded, half-quantized visualizations,
not numeric telemetry. V4 variance is linear spatial variance; v5 uses the same
bounded scalar/log-HDR variance selected for reactivity (spatial below four prior
samples, max(spatial,temporal) afterwards). Domains must not be compared as if
identical units. Frozen/rejected/no-statistics paths report variance/reactivity
zero, which means not evaluated. Covered restarts have alpha one/count one;
saturated valid freeze alpha zero/count max. Uncovered captures are all zero.
The view shader selects a channel to grayscale, writes alpha one, and masks zero
tokens/nonfinite selected samples black; missing capture remains amber.

All bookkeeping is TEMPORAL_METRICS-only and does not assign any color, moment,
count, weight or acceptance state. Comparison to saved P4.5 source with that branch
removed is token-identical. Prior unit references explicitly strip the feature to
retain v3/v4/v5 baseline checks. Capture changes only optional output/storage and
profiling paths; rendering with diagnostics Off adds no new per-frame capture or
display allocation/dispatch, though cached/in-flight old owners can remain.

#### Analytical cost and measurement status

Let S=one signal pixel count, R=one receiver-domain pixel count, D=2/4 private
depth bytes. Values are tightly packed unique image contents, not resident totals.

| Image set | Bytes | Decimal MB at S/R=1080p |
|---|---|---|
| V3/v4 scalar history (signal+position+normal/count) | 26*S | 53.9136 |
| V3/v4 HDR history | 32*S | 66.3552 |
| Additional v5 moment image, separate per key | 4*S | 8.2944 |
| One selected metric capture (or LocalHistory capture, mutually exclusive) | 8*S | 16.5888 |
| One requested diagnostic display | 8*R | 16.5888 |
| Shared per-domain metadata (token/motion/previous geometry/private depth) | (32+D)*R | 70.5024/74.6496 |

V5 scalar/HDR histories total 30/36*S. One selected metric capture plus display
adds 33.1776 MB at full 1080p, before retention. Three v5 signals at full 1080p
plus one D32 metadata domain total 286.1568 MB for one unique generation; a single
metric capture/display raises that illustration to 319.3344 MB. These are not
peak measurements or recommended settings. At 4K all these image terms quadruple;
reduced signal extents shrink S terms while receiver metadata/display retain R.
Count each unique metadata image once across effects sharing that exact domain;
moments remain per signal/view/layer. Include all current/predecessor/queued slot/
exported owners, separate pools and their high-water idle images, uniforms/bindings,
dummies and driver alignment. Idle-retention limits do not cap live allocations.
Optional capture pipelines/layouts/display are prepared at initialization; their
unmeasured startup/driver allocation cost is separate from frame image estimates.

Source work budget is up to nine raw-current neighbors with per-neighbor token,
depth/metadata-depth/normal and optional material reads, not just nine texture
fetches. V5 adds up to four prior RG16F moment reads plus one moment write per
output pixel, log-domain neighborhood arithmetic and bounded statistic updates.
Debug capture adds one RGBA16F write plus receiver display reads/writes. No rays,
subgroups, native half arithmetic or new RT requirement are introduced. Caches,
rejection support and backend lowering determine actual bandwidth/time; do not
assume 4x/9x or a millisecond benefit from fewer resets.

Keep existing effect.temporal.record CPU/renderer GPU scopes. Existing
diagnosticCapture now includes admitted metric as well as LocalHistory capture;
diagnosticDisplay and renderer.temporalDiagnostic include its display. Capture
measures the decorated pass, not isolated coloring. Disable diagnostics and
reference-signature serialization for production comparisons. Use matched v3,
v4/v5 and fallback workloads with identical rays/resolutions/denoise/trajectories;
record warm-up, device/backend/driver, support/dropped timestamp samples, active
keys and retained image estimates. Do not sum nested timing labels. Actual timings,
allocation high-water and native quality/cost comparisons are not_run; P5.4 retains
the authorized measurement procedure. No tuning or default changes were made.

#### Final review and evidence

| Area | Source/reference evidence | Vulkan | Metal |
|---|---|---|---|
| Paired v4/v5 statistics, clipping, reactivity, moments | prior references retained; metric-disabled tokens unchanged | native not_run | native not_run |
| Settings validation/codec/cached-full order | source/range/staging checks and existing C++ fixture sources | C++ native not_run | C++ native not_run |
| Legacy/custom layout and default-off behavior | canonical guard, separate layouts, trailing fields, feature stripping/source review | native not_run | native not_run |
| Freeze/trust/confidence/black/misses | references plus retained native sources | native not_run | native not_run |
| Optional numeric metrics / exact domain / stale capture | new reference/ABI/paired regions + C++ assertions | native not_run | native not_run |
| Capture output parity, toggles, cold/frozen/thaw, missing capture | ordinary adaptive control and native fixture assertions | not_run | not_run |
| Ownership/chronology/pruning | previous/next capture owners, pool lease identity, same recording/token/sequence gate reviewed | queued native not_run | queued native not_run |
| Memory/fetch cost | explicit analytical image/source budgets | measurements not_run | measurements not_run |
| Lighting/noise/chromatic/transparent/FSR limitations | P4.5 scenario matrix remains open | visual not_run | visual not_run |

Executed/passed: diagnostic_reference.py (enum/encodings/channel mapping, paired
metric regions, ABI/current-key/capture guards, analytical costs, GPL/includes/
delimiters), phase4_source_checks.py, phase3_source_checks.py,
phase2_source_checks.py, saved shader token comparison and git diff --check.
These are static/independent reference checks, not compiler or runtime results.
Native source entry runAdaptiveTemporalProbe(engine,root,false,false,true) checks
v4 numeric capture, (...,true,false,true) v5; a matched uncaptured adaptive control
checks color/guides/moments within FP16 tolerance. The fixture deliberately creates
four views on two streams, unlike the one-view renderer. It checks missing capture,
mutual exclusion before recording, cold/frozen/thaw values, count normalization,
no unrequested capture image and toggled-capture chronology. All C++ fixtures and
GLSL/Metal compilation, native formats/pixels, combined binding limits, visuals,
noise/lag/bias and actual costs remain not_run.

#### Defaults, limits and P5.1 handoff

Keep replay=false, adaptiveEnabled=false, minCurrentWeight=.03125,
reactiveCurrentWeight=.25, historyClipSigma=3 and diagnostic Off. There is no native
noise/lag evidence for release tuning. Preserve Phase 0 no-global-transform-reset,
full transform-sensitive identity and literal freeze/3*maxFrames rolling window.
Effective trust can remain below saturation under continued response/support/floor;
nominal CPU frame count is not a freeze/pixel correctness proof.

Remaining limits include strict-neighborhood over-rejection on thin/grazing/reduced
geometry, stochastic bias/noise tradeoffs, same-luminance chromatic lag, FP16
precision/storage-bound saturation, approximate nonuniform mapped normals,
unsupported reflected/deforming receiver transforms, approximate reflection/
transparent/refraction motion and changing lighting after actual freeze. Moments
are heuristic confidence, not exact transport-change detection. No release,
performance or native acceptance claim is implied by Phase 4 source completion.

Next exact step P5.1 only: audit every producer/consumer and caller against the
current v1–v5/metric/default/fallback/ownership/settings contracts, including
opaque/transparent and FSR, camera/lifecycle changes and remaining global reset
callbacks. Read Phase 5; do not silently execute P5.2–P5.6 or compile/benchmark.
