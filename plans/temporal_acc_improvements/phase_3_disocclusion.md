# Phase 3: robust disocclusion and validated neighborhood reprojection

Dependency: Phase 2 implemented. Entry: P3.1. Exit: P4.1. Do not add adaptive ray generation, stochastic history search, motion dilation or global camera-motion resets.

## Target

Replace the enhanced nearest-history sample with a validated bilinear footprint. Reject uncovered, different-surface, occluded and inconsistent candidates separately. Preserve Phase 2 rigid-object correspondence and Phase 0 fallback. Produce confidence for later adaptive weighting.

## P3.1 — specify exact footprint sampling

1. For previous UV and history extent E calculate `q = uv * E - 0.5`, integer base=floor(q), fraction=fract(q). The four candidates are base+(0,0),(1,0),(0,1),(1,1); weights are the standard bilinear products.
2. Reject out-of-bounds taps individually. Do not clamp their coordinates and count duplicate edge samples. Reject UV/clip correspondence outside the valid projected domain before sampling.
3. Fetch each candidate's own signal, position, normal/count and corresponding token. Map the candidate's texel-center UV into retained metadata resolution; never assume receiver and signal extents match. Do not filter integer token images.
4. Reject nonfinite values, absent coverage, invalid count, unequal/zero token, failed expected previous-position/depth check, and failed expected previous-normal check before including signal values.
5. Sum valid weights W. If W <= 1e-6 reject history. Otherwise renormalize valid weights to sum 1. This prevents mixing invalid foreground/background even when only one tap survives.

### P3.1 contract v1 (specified 2026-10-23)

`draw.temporal.footprint.v1` names the algorithm contract, not a registered shader
interface or a new public configuration value. P3.1 is specification/oracle work;
production `draw.temporal.v3` remains nearest sampling until subsequent integration.
V1/v2/custom consumers keep their existing behavior. No new bindings, uniform ABI,
image allocation, renderer defaults or ownership rules are introduced here.

Inputs are finite projected previous UV, positive history extent E and retained
previous-token extent M. Before floor/integer conversion require finite clip,
clip.w > 1e-8, 0 <= clip.z <= clip.w and 0 <= uv.xy < 1. Current receiver
coverage, expected previous geometry, current nonzero token and current metadata
versus receiver-depth agreement remain the Phase 2 preconditions. Invalid inputs
restart locally; never substitute clamped UV or search another surface.

Enumerate offsets in order (0,0), (1,0), (0,1), (1,1). Skip zero-weight and
out-of-bounds candidates before fetching. For candidate integer p, fetch signal,
stored position and stored normal/count at p. Fetch its integer token at
`floor(((p + 0.5) / E) * M)` using an unfiltered integer read. Use retained
metadata dimensions, not current receiver dimensions or the original previousUV.
Multiple different signal taps can legitimately map to one metadata texel; their
history coordinates are still distinct. Do not merge or duplicate edge taps.

Each positive-weight tap must pass finite signal/position/normal/count, stored
position.w > 0.5, count >= 1, nonzero equal token, previous-world distance and
expected previous normal cosine predicates. P3.1 retains Phase 2's normalized
expected normal, stored normal convention and world tolerance
`absolute + relative * length((previousView * expectedPoint).xyz)`; validate the
finite expected camera point and tolerance before candidate evaluation. P3.2 adds
its separate positive previous-view depth predicate; this specification does not
claim that predicate already executes. Never multiply an invalid NaN by zero and
then add it to a valid result.

Return raw valid support W and the accepted candidate records with weights w/W;
W <= 1e-6 returns no accepted records. Preserve raw W independently from normalized
weights for later confidence. No signal reduction or count update is defined by
this step: P3.3 owns scalar/GI/reflection reduction, minimum count, confidence and
freeze integration. In particular, component-wise RGBA averaging is not a valid
placeholder for reflection reduction. The oracle returns candidate records rather
than applying such a placeholder.

Executable reference: `python3 tests/draw/temporal/footprint_reference.py` (Python
standard library only). It covers exact center/fractional coordinates, four corner
orders, odd/1x1 extents, mismatched higher/lower metadata resolutions, unclamped
edges, off-screen/invalid projection gate, all 16 survivor masks, finite/coverage/
count/token/geometry/normal rejection, valid black and tiny-support rejection.
It verifies the mathematical contract, not C++ compilation or GPU shader behavior.
Native shader parity, positive view-depth integration, signal-specific semantics,
diagnostics and visual/performance acceptance remain later steps.

## P3.2 — refine geometric checks conservatively

1. Keep existing absolute and relative thresholds as tunable floors. Add a depth-space check against expected previous view depth so a different visible surface in front of the same reprojection is rejected.
2. Define positive depth as `-previousViewPosition.z` under the current RH camera convention. Compare candidate previous position transformed by previous view with the expected previous point transformed by the same view. Both values must be finite and positive.
3. Initial depth tolerance: use the existing world-space position tolerance as the depth tolerance too; keep the full previous-world-position distance check. Do not silently loosen tests for motion magnitude. This is conservative and easy to compare with Phase 2.
4. Add optional footprint-aware tolerance only after this baseline: expose a separately named setting defaulting to zero. Calculate a bounded world-space pixel footprint from neighboring valid same-token expected-previous positions. Maximum added tolerance must be capped and documented. Never use derivatives across different tokens to enlarge tolerance.
5. For the initial normal test retain the configured cosine threshold against expected previous normal. Very thin geometry may still reject often; prefer local reconvergence over contaminating neighboring surfaces.

### P3.2 implementation boundary (2026-10-23)

The paired v3 kernels now call `validPreviousGeometry` for the existing nearest
candidate. This reusable candidate predicate transforms both stored and expected
previous world positions by the same retained previousView, requires finite full
camera positions and positive -Z depths, compares their depth difference against
the unchanged absolute/relative world tolerance, and retains full world-distance,
coverage/count and normal tests. Negative/nonfinite tolerance rejects locally.
It is shared by scalar/HDR and production/acceptance wrappers through their
existing includes; no shader ABI or binding changes are needed.

The optional footprint-aware tolerance in item 4 is deliberately deferred: this
baseline adds zero tolerance and no new setting. No neighbor derivatives or motion
magnitude can widen rejection. A later optional feature must separately specify its
setting, same-token footprint estimate and cap before implementation.

P3.3 must wire this predicate into each bilinear candidate, move previous-token
validation from previousUV to each candidate center using actual retained token
texture dimensions, and implement signal-specific reduction atomically. P3.2 does
not activate a partially reduced footprint or add unused four-tap GPU reads.
This refines the prior resume pointer's candidate-integration wording: production
nearest remains until correct P3.3 reduction is available.

`footprint_reference.py` now applies the positive previous-view depth predicate to
every oracle candidate and covers rotated/translated previous views, zero/negative
camera depth, foreground mismatch, tolerance boundary, NaN/Inf and independent
world-distance rejection. It checks paired shader predicate parity and unchanged
blend/count/freeze/output suffix. Synthetic native source fixtures use an RH
camera at world Z=1 with compensating projection translation, preserving their
previous world/clip coordinates and expected pixels; queued inverse VP includes
that projection. Native fixtures remain uncompiled/unrun.

Future authorized acceptance: run runTemporalReprojectionProbe and
runQueuedTemporalReprojectionProbe on Vulkan/Metal, then test matching tokens at
positive matching depth, foreground depth outside tolerance, zero/negative view
depth and rotated previous camera. Expect local restart on rejected pixels with
no global history generation change. No native acceptance is claimed here.

## P3.3 — combine signal, counts and confidence correctly

1. For scalar visibility use weighted valid scalar history. For GI retain valid-black/coverage semantics. For reflections spatially combine confidence-premultiplied RGB and confidence, then recover unpremultiplied RGB only when confidence is nonzero. Do not average unpremultiplied reflection colors indiscriminately.
2. Set conservative effective history length to the minimum count among contributing taps with nonzero weight, bounded by maxFrames; do not average long counts into a freshly restarted pixel. Current accepted sample increments that count exactly once.
3. Define geometric confidence as clamp(W,0,1) for the initial implementation. It is valid footprint support, not statistical probability. A near-zero contribution is rejected using the W threshold above.
4. Supply confidence to Phase 4 through an internal enhanced-kernel variable/structure. Do not add a permanent confidence image unless another consumer needs it. Diagnostic output allocation must be opt-in and included in memory reporting.
5. Freeze remains conditional on current valid correspondence. A disoccluded pixel restarts regardless of authored freeze, and Phase 0's active update window still overrides saturation freezing. Do not freeze a rejected or uncovered pixel because a nominal frame count is high.

### P3.3 source implementation (2026-10-23)

The paired built-in v3 kernels now implement the full validated 2x2 footprint.
Current metadata depth remains a receiver gate; previous token validation moves
into the candidate loop and uses actual retained texture dimensions. Each tap
passes P3.2 geometry plus finite signal validation before accumulation. GI alpha
must exceed .5; valid black survives. Reflection confidence is clamped to [0,1],
premultiplied for spatial accumulation and unpremultiplied only at nonzero
confidence (>1e-8). Zero-confidence misses remain valid support/count contributors.

W <= 1e-6 restarts locally. Otherwise normalize signal weights, retain minimum
contributing count capped by maxFrames and increment once through the original
temporal blend. Internal geometricConfidence is clamp(W,0,1), zero on rejection;
it is reserved for Phase 4, not yet an adaptive blend multiplier or exported image.
The existing temporal reflection confidence blend, freeze uniform and rolling
window remain unchanged. Overflow/nonfinite spatial results reject conservatively.
No new image, ABI, setting, renderer default, ownership or synchronization change.

Source/math acceptance: footprint_reference.py now checks scalar partial support,
invalid bright foreground exclusion, minimum count versus long histories, count
cap, saturated freeze/rolling/rejection, valid-black/missing GI and reflection
mixed confidences/misses plus temporal confidence composition. It checks normalized
GLSL/Metal footprint parity and preserved temporal blend/output suffix. The Phase 2
source checker now starts suffix equality at oldWeight, allowing the intended
minimum-count replacement. These are reference/source checks, not shader execution.
Existing native fixtures and all pixel/visual/performance acceptance remain not_run.
P3.4 will add detailed rejection reasons and synthetic/native footprint scenarios;
no P3.4 diagnostic formats or reason colors are introduced in this step.

## P3.4 — diagnostics and probes

Define mutually understandable diagnostic reasons: no predecessor, unsupported correspondence, off-screen/behind-camera, uncovered, token mismatch, position/depth mismatch, normal mismatch, invalid signal, accepted partial footprint, accepted full footprint. If multiple failures exist, use the first in that fixed order for debug coloring; aggregate counters may overlap if documented.

Add synthetic probe inputs for every footprint corner; half-texel and integer-center positions; 1x1/odd extents; one/two/four surviving taps; same token across a depth discontinuity; different token at same position/normal; newly uncovered black GI; reflective misses; NaN/Inf; rotated object normals; transparent edge coverage; metadata/history extent mismatch. Test exact candidate selection and weights with a CPU oracle.

Native visual scenarios when authorized: thin poles over distant wall, slowly translating alpha-cutout geometry, grazing-angle plane, moving foreground along depth, reduced-resolution effects, camera rotation, fast camera/object motion. Verify no old foreground contribution bleeds into newly uncovered wall. Remaining aliasing and stochastic noise are allowed; report them instead of arbitrarily broadening tolerances.

### P3.4 source completion (2026-10-23)

Built-in v3 LocalHistory now uses ordered gray/violet/blue/black/magenta/red/
yellow/white rejection colors and cyan/green accepted partial/full support.
The precise mapping and evaluated-stage precedence are recorded in
../../doc/api/draw/TemporalAccumulator.md. Full support means W >= 1-1e-6, not
necessarily four nonzero taps. Accepted support overrides failed neighbors;
otherwise the minimum evaluated reason wins. Token-rejected diagnostic taps
inspect coverage so uncovered precedes identity. Missing captures remain amber.
No aggregate counters or extra persistent images are introduced. All bookkeeping
is compile-time diagnostic-only; stripped production shader tokens match P3.3.
V1/v2/custom families and display ABI remain unchanged.

`footprint_reference.py` adds diagnostic priority/color checks, combined coverage/
identity/geometry/normal/signal failures, rotated expected normal and edge-like
uncovered bright foreground. It retains the P3.1–P3.3 coordinate/reduction matrix.
New `footprint_probe.hpp` provides runTemporalFootprintProbe, a stopped native
source fixture with all 16 scalar token masks at midpoint reprojection, odd/reduced
extents, normalized pixel expectations and every reason/acceptance color. Current
NaN/positive/negative infinity, uncovered receivers, invalid expected geometry,
off-screen projection, normal and same-token geometry rejects are explicit inputs.
RT/FSR are unnecessary. Immediate submissions/readback remain confined to the
fixture. Existing queued source fixtures cover HDR/valid-black/misses/owners.

Checks executed: standard-library footprint reference and Phase 2 source checks;
paired source geometry/footprint/color parity; GPL/includes/delimiter review;
production-token comparison against saved P3.3 source with diagnostics stripped;
git diff --check. All passed. These checks do not compile or execute native code.
Native matrix: scalar mask/color fixture, existing HDR/queued fixtures, real
rasterized alpha/transparent edges, 1x1/odd backend pixels, rotated normals,
thin-pole/depth/grazing/reduced-resolution/camera/fast-motion visual scenarios:
Vulkan not_run, Metal not_run. Probe host instructions and limits are in
../../tests/draw/temporal/README.md. Compilation, format support, native pixels,
visual quality and timing remain unverified; no default is enabled. Next: P3.5.

## P3.5 — cost review and handoff

Use existing `effect.temporal.record` timing and add opt-in diagnostic timing labels without per-pixel CPU readback. Compare nearest enhanced sampling against 2x2 at identical resolution, signal settings and scene. Disable debug outputs and signature-reference validation for timing.

Expect up to four candidate histories instead of one; caching may reduce bandwidth but do not assume a 4x or fixed millisecond runtime. No new mandatory persistent image is needed for the base algorithm. Keep only bounded early-outs; no unbounded neighborhood search.

Review GLSL/Metal parity, all no-history paths, reflection confidence math, count semantics, invalid coordinate handling and retained ownership. Record actual executed/not_run checks and pointer P4.1 in progress.md.

### P3.5 final source/cost review (2026-10-23)

Phase 3 is implemented_unverified (P3.1 specified reference; P3.2–P3.5 source
complete). No compiler, native backend, visual or performance acceptance is implied.
The next independent source step is P4.1; all native requirements below remain open.

Timing now preserves existing CPU and renderer GPU `effect.temporal.record` scopes.
Only admitted opt-in acceptance captures add CPU/GPU
`effect.temporal.diagnosticCapture`; only admitted display work adds
`effect.temporal.diagnosticDisplay`. Existing renderer.temporalDiagnostic remains.
Capture covers the decorated temporal recording/dispatch, not an isolated marginal
cost of coloring; display covers the existing display allocation/bind/dispatch.
GPU scopes use the existing timestamp collector and layer/detailed filtering. They
introduce no waits, readbacks, counters or capability retries. Off/unavailable/
missing-input paths do not create these diagnostic scopes. Profiling still needs
the existing CPU collection/GPU pass-timing switches and backend support; a missing
timing sample is unsupported/dropped, never a zero-cost measurement. Nested scope
durations must not be added as independent frame costs.

#### Source acceptance audit

| Area | Reviewed invariant | Source result | Native result |
|---|---|---|---|
| Paired kernels | Same four-tap coordinates, geometry predicates, confidence reduction and reason colors | pass: reference/source audit | Vulkan/Metal not_run |
| No history/fallback | CPU chronology/format/mode/extent admission; raw restart; v1/v2/custom untouched | reviewed; source assertions pass | not_run |
| Coordinates | Strict projected domain before conversion; individual bounds; no clamped duplicate taps; actual retained token extent | pass: oracle/source | not_run |
| Geometry | Finite expected/stored guides; positive previous-view depth and unchanged world/normal tolerance | pass: oracle/source | not_run |
| Signal | Valid-black GI and confidence-zero reflection miss remain valid; invalid values never enter sums | pass: oracle/source | not_run |
| Count/freeze | Minimum valid nonzero-weight count, one increment, bounded count, reject restarts, exact motion-window override | pass: oracle/source; existing CPU chronology reviewed | not_run |
| Ownership | Previous/next and metadata owners retained before dispatch; acquired slot release; abandoned predecessor exclusion; external output ownership | reviewed in accumulator/pool/renderer; no Phase 3 ownership edits | queued native not_run |
| Work/capabilities | One bounded 4-tap loop; no new mandatory persistent image, RT, SDK, atomic or subgroup requirement | pass: source | formats/deployment not_run |
| Diagnostics | Optional existing capture/display only; fixed evaluated-reason precedence; production tokens unaffected by reason blocks | pass: source/reference | colors/timestamps not_run |

Executable review entry: `python3 tests/draw/temporal/phase3_source_checks.py`.
It runs the footprint reference, source invariants and analytical byte accounting.
Also run `python3 tests/draw/temporal_surface/phase2_source_checks.py` and
`git diff --check`. These passed on 2026-10-23; none executes C++/GLSL/Metal.
The P3.5 implementation diff adds timing scopes only; all Phase 3 shader algorithms,
accumulator recording/lifecycle/reset/blend/owner behavior remain as at P3.4.

#### Analytical image and fetch costs

Let S be pixels at a particular signal extent, R pixels at one receiver-domain
extent and D=2/4 bytes for private D16/D32 depth. Values below describe one unique
set of tightly packed image contents, not measured GPU allocations or feature totals.

| Unique image set | Bytes | MB at 1920x1080 | MB at 3840x2160 |
|---|---|---|---|
| Scalar history: R16 + RGBA32 position + RGBA16 normal/count | 26*S | 53.9136 | 215.6544 |
| One HDR history: RGBA16 + same guides | 32*S | 66.3552 | 265.4208 |
| One metadata domain: token/motion/expected position/normal/private depth | (32+D)*R | 70.5024 / 74.6496 | 282.0096 / 298.5984 |
| Metadata subset held by History (excludes motion) | (28+D)*R | 62.208 / 66.3552 | 248.832 / 265.4208 |
| Optional acceptance capture for one selected key | 8*S | 16.5888 | 66.3552 |
| Optional diagnostic display | 8*R | 16.5888 | 66.3552 |

Metadata is shared by effects in the same exact domain; the History subset is
already part of the complete metadata-domain row and must not be added again.
Opaque and each active transparent layer/view may have distinct domains. Different
signals can have different S. For illustration only, AO+GI+reflection all at 1080p
and one D32 domain occupy 261.2736 decimal MB for one unique generation of those
sets; one selected acceptance plus display raises that to 294.4512 MB. This is
not a peak/resident total or a recommended setting. Quarter-pixel-count effect
images reduce their signal terms, not receiver metadata terms.

Inventory unique image handles across current histories, predecessor/next
invocations, acquired/in-flight slots, renderer/correspondence/FSR owners, exported
outputs and pool caches. Ordered submissions can retain several generations;
externally held outputs can outlive pool entries. Count shared metadata once.
Use temporal poolStatistics and existing temporary.retainedImageBytes/
retainedUniformBytes/retainedImages/allocation counters for their own pool only;
those are estimates, not a global unique allocation census. Sample high-water over
the whole run and include the separate metadata pool, other active signals/layers,
uniform/binding allocations, 1x1 dummies and driver alignment overhead. The pool's
512 MiB idle-retention limit is not a hard bound on live/in-flight/external memory.
No new telemetry/readback is introduced to estimate these costs.

A fully valid candidate requires signal+position+normal/count+token reads: 30
nominal bytes for scalar, 36 for HDR. Four candidates give up to 120/144 nominal
bytes per output pixel for this subset, versus 30/36 for one; common current guides,
metadata, writes, texture cache transactions and backend behavior are excluded.
Token rejection can exit earlier. Diagnostic-only token-rejection coverage reads
add work, so diagnostics must be off for production timing. These are source fetch
budgets, not bandwidth measurements or a prediction of 4x runtime. Phase 3 adds
no mandatory image compared with Phase 2 and no rays.

#### Reproducible future native comparison (not executed)

After explicit compilation/execution authorization, deploy native kernels for the
same current versioned ABI on each available backend. Compare the four-tap kernel
with a separate temporary shader-root nearest control: replace the footprint base
with floor(previousUV*historyExtent), use fraction=(0,0), weight=1 and one candidate.
Keep candidate-center token mapping, P3.2 geometry, thresholds, signal/count math,
metadata producer and all bindings unchanged. This is a controlled nearest-enhanced
baseline, not the v2 fallback or an assertion of exact historical P2 pixel parity.
Do not change the production shader or CMake to run the comparison. Retain each
shader's source/build identity in the measured report.

Use identical scene/camera/object trajectories, internal and signal extents,
ray/noise/denoise settings, active view/layer counts, scene-frame count and
freeze/rolling policy. Warm both pipelines/caches; reset histories consistently
and collect stable and moving intervals separately. Disable LocalHistory and all
other debug displays/captures, full-signature reference validation and extra CPU
reference checks. Use the existing CPU/GPU profiler, record device/backend/driver,
GPU timestamp availability/drops, pooling state and allocations. Compare frame
and effect.temporal.record distributions (median/p95), unique-image/high-water
estimates, static noise and newly uncovered pixels. Inspect the detailed capture/
display labels only in a separate diagnostic run, without subtracting nested
intervals to invent a marginal shader time.

| Future acceptance | Entry/scenario | Vulkan | Metal |
|---|---|---|---|
| Compile/deploy/format support | Current paired kernels, scalar/HDR/diagnostics and retained formats | not_run | not_run |
| Footprint/diagnostic pixels | runTemporalFootprintProbe | not_run | not_run |
| Rigid/HDR/count/ownership | runTemporalReprojectionProbe, runQueuedTemporalReprojectionProbe and existing temporal fixtures | not_run | not_run |
| Thin/alpha/transparent/grazing/depth edges | P3.4 visual scenarios, reduced resolution, camera rotation/fast motion | not_run | not_run |
| Static background convergence | Repeated transforms with replay enabled and fallback separately | not_run | not_run |
| Timing and high-water memory | Matched nearest/four-tap experiment above, diagnostics off | not_run | not_run |
| Optional diagnostic timestamp support | Separate capture/display run | not_run | not_run |

Known limits: strict geometric thresholds can reduce support on thin/grazing and
reduced-resolution geometry, causing local reconvergence/aliasing. Nonuniform
mapped normals remain approximate, mirrors/deformation unsupported; reflected
content/transparent FSR motion remains approximate. Geometry-valid stationary
receivers can still retain stale changing indirect illumination. No runtime,
quality, performance or memory-saving claim; replay remains default off.
P4.1 may now define default-off adaptive settings/bounded weights; do not implement
P4.2–P4.6 or moments in that step, or reinterpret this source review as native
acceptance. P4 must preserve the validated nonadaptive footprint as its baseline.
