# Manual temporal validation

Current native evidence (2026-10-23): runTemporalProbe passed on Metal and Vulkan/
MoltenVK; scoped S10/F0 passed raster/AO-temporal assertions on Metal, and Vulkan
passed raster checks with AO unavailable. Library/paired shaders compiled. See the
[run record](../../../plans/temporal_acc_improvements/validation_2026_10_23/README.md)
for exact scope; historical not_run statements below describe earlier handoffs.
Other native scenarios/SDK/benchmarks remain pending.

Include probe.hpp in an existing temporary draw test application and call `bg2e::draw::tests::runTemporalProbe(engine, shaderRoot)` with a stopped initialized draw Engine. No CMake target is added. Compile engine and paired shaders first. A false result means unavailable temporal formats/kernels, not a passing accumulation test. No agent execution is claimed.

The fixture includes a CPU camera reference (jitter alone gives zero unjittered motion, previous-minus-current pixel direction) and actual scalar/HDR kernels on explicit and camera-depth receivers. It alternates two completed slots and simulates presentation gaps while scene samples remain consecutive. Numerical assertions cover independent AO/GI/reflection means, confidence-weighted unpremultiplied reflected RGB, retained earlier output, bounded rolling and frozen convergence, external generations, position/normal rejection, effect/view/layer separation, invalid/valid-black GI, eligible reflection misses versus roughness rejection, and disabled borrowed pass-through. Pause chronology is a CPU no-record check; it is not a claim of application pause validation.

Lead runtime scenarios on Vulkan and Metal:

- Run the fixture with diagnostics and non-RT devices where temporal formats are available. Compile all camera/world scalar/HDR profiles. Check unavailable shader/format fallback to raw signals.
- In existing deferred/manipulation examples test static convergence, continuous camera reprojection, camera switch/explicit cuts, same-camera teleports with invalidateHistories, depth disocclusion and normal edges. Validate native Y/jitter conventions at varied sizes and FSR scale.
- Verify resize, output-relative reflection quality, AO/GI quality, mode/enable/noise/temporal settings and FSR/upscaler replacement resets. SDK state and engine effect histories must remain separate.
- Edit transforms/topology/mesh/material/texture/light/environment at the queued safe boundary, including partial failure. Identity-changing edits are detected; in-place image uploads/custom shader edits require invalidateHistories. Transform-only edits must preserve global history generation and FSR state. Move an object after freezing: every active history rolls during motion and for exactly 3 * maxFrames subsequent scene records, then restores the authored freeze preference.
- Check independent transparent layers and stable node/submesh identities. Duplicate persistent correspondence must be diagnosed when active. Test maxHistories and stale-key pruning, multiple public view IDs, sequence gaps, duplicate records and backward sequence rejection.
- Pause/retain scene color while UI/presentation continues, perform overlay/pick-only work, then resume. Only actual sceneFrameNumber samples may advance frame counts or noise. No retained image should mutate.
- Exercise asynchronous ordered submissions across slots before completion, old returned owners, reset while earlier work is in flight, abandoned recording plus invalidate, cleanup before Device destruction, custom compatible subclasses/kernels and unsupported fallback. Never call beginFrame on an unretired slot or use another queue without explicit synchronization.
- Validate confidence at partial reflective edges: valid black, misses and roughness rejection must remain distinct. Nominal frameCount is not proof of per-pixel convergence after rejection. Freeze mode must retain a saturated valid pixel while allowing rejection to restart it.

Phase 0 regression additions exercise the actual scalar/GI/reflection kernels at a saturated receiver, six rolling tail records with maxFrames=2 and refreezing on the seventh, motion renewal, independent view/layer keys, skipped keys/sequences, update-without-record, explicit/external resets, and unsubmitted/abandoned motion recordings. A probe subclass reads recorded CPU countdown state; it adds no production telemetry API. Boundary limits 1 and 1024 check the complete tail and final frozen pixel. The 1024 case submits 3072 one-pixel tail frames; this is a correctness fixture, not a benchmark. Shared low/high inputs limit retained input allocations in that loop. Ordinary three-argument beginFrame callers default to no motion; custom overrides must accept and forward its new motion argument.

The original no-motion freeze and immutable retained-output assertions remain. Pixel readback and immediate submissions are intentionally confined to this stopped manual fixture. None of the new scenarios has been compiled or run by the agent; unavailable formats/kernels are unsupported, never a pass.

Accepted Phase 0 limits: moving receivers may trail, and stationary receivers may retain stale AO, GI, reflected radiance and shadows as other objects move. FSR still has camera-only vectors and can trail moving objects or glass. The finite rolling window reduces stale frozen lighting without guaranteeing exact convergence. Explicit application/editor callbacks may still request global invalidation; distinguish those from renderer transform policy during interactive validation. Record actual hardware/backend, diagnostics and scenarios checked.

Optional P1.4 fixture: call `runTemporalMetadataProbe(engine, shaderRoot)` in a stopped,
initialized host after explicitly authorizing compilation/execution. It requests
`draw.temporal.v2`; false means unsupported, not pass. It checks odd receiver extent
3x3 against ceil-half-resolution signal 2x2, local token rejection, zero tokens,
current depth disagreement and previous out-of-bounds rejection. No native run has
been performed. This is separate from the legacy `runTemporalProbe` contract.

P1.5 extends the metadata fixture with actual acceptance-mask colors (including
maxFrames=1) and malformed-pair rejection. Broader raster/chronology probes and
manual acceptance instructions are in [temporal_surface/README.md](../temporal_surface/README.md).
All additions remain source-only, with no native run performed.

## P2.3 rigid consumer source fixture

Include `reprojection_probe.hpp` and call `runTemporalReprojectionProbe(engine, shaderRoot)`
only in an authorized stopped initialized host. False means v3 support unavailable,
not success. It provisions v2+v3 and checks transport on odd 3x3 receivers / 2x2
signals while camera translation exceeds the legacy threshold, receiver depth changes
and current normal differs from expected previous normal. It also checks local token,
previous-validity, expected normal, previous-UV and current-depth rejection, retained
output owners, v2/v3 mode switches, explicit cuts and projection changes.

`runTemporalMetadataProbe` retains its v2 behavior and adds partial/no-token, wrong
format and wrong extent previous-surface pair rejection, even without v3 opt-in.
C++/shader compilation and all fixtures remain not_run on Vulkan/Metal. These are
source assertions, not native results. Run the existing scalar/GI/reflection/freeze
fixtures as well; real-raster queued slots, jittered camera motion and visual/HDR
acceptance remain pending. No new build-system entry or automatic runner is provided.

## P2.5 queued v3 source fixture

`queued_reprojection_probe.hpp` provides runQueuedTemporalReprojectionProbe(engine,
shaderRoot). Run only in an explicitly authorized stopped initialized host. All
source inputs are uploaded before recording; the first two/three scene commands
are submitted in order before any fixture wait/readback. Later reuse waits only at
fixture acquisition boundaries. No assertion claims the GPU remained incomplete or
that real overlap was measured. Unsupported v3/display/formats return false.

It checks scalar/GI/reflection transport with simultaneous receiver/camera motion,
analytic means and separately accumulated reflection confidence, valid-black GI,
rolling past maxFrames=4 while freeze remains authored true, retained outputs,
abandoned command rejection, an update-only gap, new/zero tokens and motion/previous-
position/normal displays. Missing selected input and wrong format are checked; invalid
token displays must remain black. Its half-float tolerance is verified independently
by the Python source/math checker. Receiver and signal extents are 3x3 and 2x2.
All waits, immediate submissions, uploads and readbacks are fixture-only.

This fixture and all C++/shader sources remain uncompiled/unexecuted. It uses synthetic
receiver guides and is not visual acceptance of a rasterized moving cube, mapped
normals, actual AO/GI/reflection, transparency or FSR. The Phase 2 plan P2.5 table
records the remaining native acceptance scenarios. No CMake target/runner is added.

## P3.4 footprint regression sources

Run the independent standard-library oracle with
`python3 tests/draw/temporal/footprint_reference.py`. It covers four corners and all
16 survivor masks; fractional/half-texel and integer-center coordinates; odd/1x1
extents; one/two/four surviving taps; higher/lower metadata resolution; strict UV
bounds; same-token depth discontinuities; different tokens at identical geometry;
NaN/Inf; rotated expected normals; uncovered edge-like guides; valid-black GI;
reflection confidence/misses; count/freeze behavior and ordered rejection reasons.
Source parity checks compare GLSL/Metal geometry, footprint reduction and colors.
This executes reference math and source assertions, never production C++ or shaders.

Include `footprint_probe.hpp` and call
`bg2e::draw::tests::runTemporalFootprintProbe(engine, shaderRoot)` in an explicitly
authorized, stopped initialized host. It needs no RT/FSR. It requests built-in
v2/v3 and acceptance kernels, uses odd 3x3 receivers with reduced 2x2 scalar history,
and tests all 16 previous-token masks at the exact four-tap midpoint. Distinct
scalar values discriminate normalized valid sampling from nearest/invalid mixing.
It asserts first-frame gray, rejected-token magenta, partial cyan/full green, normal
yellow, geometry red, unsupported violet, off-screen blue, uncovered black and
NaN/positive/negative infinity white. A missing optional capture reports false
(amber display); unavailable v3 also reports false. Device format/allocation errors
must be reported as unsupported/errors, never successful acceptance.

All waits/uploads/immediate submissions/readbacks are fixture-only. This header is
not wired to a build target; no CMake changes or automatic compilation are implied.
C++/shader compilation and Vulkan/Metal execution of this fixture remain not_run.
The existing queued HDR fixture supplies additional GI/reflection/count/retained
owner sources, also unrun. Synthetic coverage does not verify actual transparent
raster blending or alpha-cutout coverage. The new reference fixtures cover 1x1 and
corner domains mathematically; native 1x1/rotated/transparent/HDR/raster scenarios
still require an authorized host. See the API's v3 reason-color table.

When authorized, run both backends where available with a thin pole against a
wall, translating alpha-cutout geometry, a grazing plane, depth-moving foreground,
reduced-resolution effects, camera rotation and fast camera/object movement.
Keep replay explicitly enabled for enhanced paths, exercise fallback separately,
and disable authored scene animation for convergence comparison. Newly uncovered
wall pixels must not contain old foreground radiance; static background history
must survive transform-only motion. Record support/rejection colors alongside
pixels, global generation, backend/formats and scene settings. Report aliasing and
noise rather than widening tolerances. No visual or performance pass is claimed.

## P3.5 audit and future cost run

`python3 tests/draw/temporal/phase3_source_checks.py` executes the footprint oracle,
source-contract audit and analytical byte accounting. It needs only Python's
standard library. It does not compile or run the engine/shaders. The Phase 2
source checker remains an additional regression check.

The Phase 3 plan's P3.5 section records the completed source audit, per-unique-image
memory formulas, profiler labels and exact matched nearest/four-tap experiment.
Use effect.temporal.record with diagnostics/reference checks off for production
cost; diagnosticCapture/diagnosticDisplay are for a separate optional run. Missing
GPU samples are unsupported/dropped, not a zero-time pass. Include active signal/
view/layer extents and retained/in-flight/external/pool high-water images, counting
shared metadata once. All native fixture, deployment, visual, timing and measured
allocation acceptance remains not_run on Vulkan/Metal. Explicit authorization is
required before compiling or executing the native comparison. Source handoff: P4.1.

## P4.1 source/reference and optional native weighting checks

`python3 tests/draw/temporal/phase4_source_checks.py` verifies default/range contracts,
paired v4 weighting/ABI/wrappers, cached/reference field order and independent
bounded-weight/freeze math. Existing Phase 2/3 checks still exercise the disabled
v3 baseline after excluding v4-only preprocessor blocks. These are source/reference
checks, not compilation or execution of setters/shaders.

For future authorized native work, run runTemporalFootprintProbe(engine,shaderRoot)
for v3 then runTemporalFootprintProbe(engine,shaderRoot,true) for v4. The latter
requests v4, enables adaptive weights (minimum=0, reactive=1), and distinguishes
one-tap support alpha=.75 from the old .5 on the second frame; two/four-tap cases
retain the baseline .5. Missing v4 or its capture reports unsupported/false.
The existing all-mask/color/rejection assertions remain. These fixtures are not
noise/visual/performance acceptance and remain uncompiled/unrun.

The settings codec fixture checks actual reflected/default/legacy/invalid tuple
source cases. The temporal invalidation fixture adds every adaptive setting in
cached and full-reference modes, including requested adaptation on injected legacy
kernels. Its existing full/cached fingerprint comparisons remain. Both fixtures
remain uncompiled/unexecuted. Freeze/thaw, disabled parity, maxFrames=1 and bounded
weight math are covered by the independent reference; full native adaptive scalar/
GI/reflection/freeze/queued acceptance remains P4.5/native work.

P4.1 implements only settings and bounded support-based weighting. Reactivity is
zero, sigma is reserved, and there are no current-neighborhood statistics, clipping,
moments or changed trust-count semantics yet. Next source step: P4.2.

## P4.2 current-neighborhood reference

Run `python3 tests/draw/temporal/neighborhood_reference.py`, or the phase4 source
checker which also invokes it. The supplied-world-guide reference tests bounded
3x3 selection, 1x1/corner support, odd and mismatched receiver mapping, exact scalar
mean/variance/clamping, rejected bright neighbors, identity/depth/normal/nonfinite
filters, rotated normals, valid-black GI, missing GI, confidence-zero/low-confidence
and roughness-limited reflection bounds, HDR maximum/outlier and fewer-than-two
support. An orthographic receiver-center case distinguishes five-neighbor from
one-neighbor support at conservative metric tolerances. The same script compares
normalized GLSL/Metal helper bodies. This executes independent reference math and
source assertions; it does not run the engine or shader reconstruction.

Helpers use only raw current samples and current receiver guides. They are called
in v4's accepted adaptive/nonfrozen branch, but their result is not consumed until
P4.3, so native compilers may optimize it out. P4.1 output parity does not establish
statistics correctness. Future explicitly authorized native inspection needs a
temporary wrapper exporting CurrentStatistics to test storage or the later P4.3
consumer. Check count, validity, mean/second/variance and bounds at odd/reduced
sizes, mixed-token/depth/normal/alpha/transparent boundaries and HDR values on
both available backends. All shader/native/visual/performance checks remain
not_run. No native test wrapper, new production diagnostic image or CMake target
is introduced here. P4.3 is next; P4.4 moments remain separate.

## P4.3 response and effective trust

Run `python3 tests/draw/temporal/history_response_reference.py`; the Phase 4 source
checker also runs it. Independent cases cover known scalar steps and noisy bounds,
sigma zero/empty intersection, insufficient support, unclipped reactivity, exact
frozen/disabled/rejected bypass, maxFrames=1, geometric/floor-driven fractional
trust, valid black GI, near-black HDR fallback, FP16 maximum scaling, retained
same-luminance chromatic artifacts and reflection confidence hit/miss transitions.
Paired response-helper and v4 integration source checks accompany the math.

Future authorized native run: call runTemporalFootprintProbe(engine,shaderRoot,true).
P4.3 now clips the constant-current four-neighbor fixture to .75; the prior P4.1
one-tap alpha-only pixel expectation is superseded. All geometric mask/reason tests
remain. The test-only subclass exposes retained normal/count guides, then checks
eight stable samples, a changed signal with effective trust two while CPU nominal
frameCount is nine, literal saturated freeze, and motion-driven rolling response
with authored freeze unchanged. The ordinary false/default argument still uses
v3's original normalized-history expectations.

None of these C++/shader fixtures has been compiled/run here. Native scalar/HDR
precision, alpha/confidence, half-quantized fractional trust, actual freeze/thaw
windows, queued ownership, transparent/normal-mapped/odd-resolution behavior and
noise/lag/chromatic/storage-bound tradeoffs remain acceptance work. Stats now
influence v4 output, so their collection can no longer be assumed optimized out;
measure its up-to-nine candidate current-guide/signal fetch cost when authorized.
No new diagnostic/moment image is allocated. Next source step: P4.4 moments only.

## P4.4 moment reference and optional native fixture

Run `python3 tests/draw/temporal/moments_reference.py`, also invoked by the Phase 4
source checker. Independent math covers visibility/log-HDR bounds and finite half
storage, rejection of NaN/Inf, mean(log) versus log(mean), same-tap/color/moment
renormalization for all masks, invalid-moment joint rejection, reset/uncovered/
frozen/same-alpha updates, early spatial variance and mature max(spatial,temporal)
variance. Source checks audit v5 layout/capability/dummy/owner selection, paired
moment reactivity and wrappers. Branch stripping now handles nested v5 conditionals
so earlier v4/v3 source comparisons still test their exact baseline paths.

Future authorized native host: runTemporalFootprintProbe(engine,shaderRoot,true,true)
requests optional v5 and returns false if the core moment path is unavailable.
It reads test-only retained moment owners to compare scalar means/second moments
against every mask's normalized taps and effective alpha, checks older output
owners remain unchanged, and verifies exact frozen .25/.0625 pairs. The true/false
and default variants still exercise v4 and v3 respectively. No source wrapper
silently substitutes v4 and calls it a successful moment test.

All C++/shader compilation and execution remains not_run. Verify both available
backends, no-moments fallback, HDR log/half rounding and hit/miss transitions,
invalid/dropped moments, queued two/three slots and independent keys, resize/reset/
mode transitions, freeze/thaw, actual lighting/noise/visual behavior and peak memory
when authorized. One RG16F moment image costs 4*signalWidth*signalHeight bytes;
include all unique retained/in-flight/pool images instead of reporting that as a
total. No new ordinary-frame readback or CMake target was added. Next: P4.5.

## P4.5 adaptive and queued regression sources

Run `python3 tests/draw/temporal/adaptive_sequence_reference.py` or the Phase 4
source checker. It composes the independent neighborhood/response/moment oracles
into half-quantized temporal streams: scalar/noise/step, HDR/outlier, black GI,
reflection confidence/misses, one-neighbor support, freeze/exact settling window,
drop/gap/invalid samples, maxFrames=1, disabled/fallback mode and immutable per-key
reference states. Its printed convergence counts/RMSE are model-only; no shader,
C++ admission or GPU lifetime is executed or validated by those numbers.

New `adaptive_probe.hpp` supplies runAdaptiveTemporalProbe(engine,root) for v4,
(...,true) for v5 and (...,false,true) for forced optional-moment module failure.
The synthetic 3x3 shader fixture compares matched adaptive/disabled streams,
checks noisy and stationary scalar behavior without accumulator generation changes,
valid-black GI, HDR/bright outlier finiteness, reflection hit/miss/confidence,
known similar-luminance chromatic history and maxFrames=1. Its geometry tolerance
intentionally admits a neighborhood, unlike the narrow queued fixture. Forced
missing module is not a fake unsupported format; capture the actual native reason.

The queued fixture now accepts trailing adaptive/moments bools. Call
runQueuedTemporalReprojectionProbe(engine,root), (...,true) and (...,true,true).
Original v3 callers retain the baseline assertions. V4/v5 use narrow geometry with
one current neighbor, so clipping/reactivity stays off and expected baseline
weights remain analytic. V5 adds retained scalar/log-HDR moment pairs, independent
view/layer values and image owners, and abandoned-history restart. Inputs are
uploaded before two/three queued submission batches; no actual GPU overlap is
promised, and no abandoned output is read back.

These C++ headers remain uncompiled/unrun. Synthetic compute requires no RT/FSR;
real AO/GI/reflection visuals require their own capabilities. False means unsupported,
not pass; errors/format failures must be recorded with actual reasons. Keep the
existing footprint, settings codec and invalidation fixtures in the native run.
The Phase 4 plan P4.5 contains the full source/native matrix, exact host entries,
controlled visual scenes and convergence/noise acceptance protocol. All native
format/pixel/HDR/queue/visual checks remain not_run; no CMake target, production
shader or default was changed. Next source step is P4.6, not automatic native work.

## P4.6 adaptive numeric diagnostics and source handoff

Run `python3 tests/draw/temporal/diagnostic_reference.py` or the Phase 4 checker.
It verifies stable enum values, canonical metric layouts/bindings, separate current
capture selection, paired metric source, finite encodings, cold/frozen/count
semantics and analytical image bytes. Previous Phase 2/3/4 references also pass;
source comparisons with TEMPORAL_METRICS disabled preserve the P4.5 v4/v5 tokens.
No native compiler or runtime is executed by these scripts.

Future authorized host: runAdaptiveTemporalProbe(engine,root,false,false,true)
for v4 metrics or (...,true,false,true) for v5 metrics. The fifth bool enables a
third ordinary-adaptive control and numeric assertions. The probe compares
color/normal/moment output within FP16 tolerance, checks four display channels,
missing captures on disabled adaptation, no extra metric allocation on the control,
cold first-sample encoding, post-response trust, frozen/thawed updates and capture
on/off chronology. Both capture kinds requested together must throw before recording.
If the exact selected optional family is absent, the fixture reports unsupported;
false is not a pass. (...,false,true,true) combines forced missing moment-family
fallback with v4 metric inspection. All variants remain uncompiled/unrun.

CurrentWeight/Reactivity/Variance/EffectiveCount append renderer view IDs 6–9.
Capture RGBA stores alpha, reactivity, variance/(1+variance) and count/maxFrames;
the separate display selects one channel into grayscale with opaque alpha.
V4 uses linear spatial variance; v5 uses scalar/log moment-domain response variance.
Frozen/rejected/insufficient-support variance zero means not evaluated. Amber means
no current matching capture; token-zero or nonfinite selected data is black.
Use the API doc's encoding/availability contract when interpreting these images.

This native fixture intentionally requests four displays on two streams and a
parity control. It does not represent renderer overhead, which is one selected
signal/layer capture plus one display. Capture costs 8*S and display 8*R content
bytes per unique image; include predecessors, queued slots, retained outputs,
pool high-water and driver overhead. Off adds no new per-frame image/dispatch,
but old owners/pool cache can persist. Use existing profiler intervals with debug
and signature-reference checks disabled for future matched production timing;
all timing/native allocation measures remain not_run.

Phase 4 source work is implemented_unverified, defaults remain off, and its plan
contains the final cost/acceptance/limits audit. Next exact source step: P5.1
producer/consumer integration audit, not automatic compilation or release tuning.

P5.1 integration source guard (no build or GPU execution):

```sh
python3 tests/draw/temporal/integration_source_checks.py
```

This guards typed model-editor transform edits versus conservative general edits,
opaque/transparent metadata ownership, camera-depth enhancement gating and recorded
scene chronology. The producer/consumer and reset audit is recorded in
`plans/temporal_acc_improvements/phase_5_integration_and_tuning.md` under P5.1.
Passing these textual guards does not establish editor interaction, queued GPU
execution, submission-failure recovery or either backend's native acceptance.

## P5.2 reproducible validation protocol

This section is the validation protocol; earlier numbered sections are
historical source handoffs. Native rows began as **Vulkan not_run / Metal not_run**;
S10 now links its scoped authorized result below. Other rows remain not_run. Preparing this protocol authorizes no build, shader compilation, native
execution or benchmark. Use the existing deferred/manipulation examples or a stopped
initialized draw test host after separate explicit authorization; add no CMake target.

Use a fixed perspective camera looking at a matte wall and a movable cube, with a
second unchanged cube to one side. Add a thin alpha-cutout card, two overlapping
transparent panes and a static reflective plane for the relevant rows. Save the
scene, camera matrices, light/environment setup and initial transforms with the run
record. Disable unrelated animation and auto-exposure changes. Use a deterministic
motion schedule measured in **rendered scene records**, never mouse/presentation
frames: warm up for maxFrames samples; move the cube for 16 samples; stop for exactly
3*maxFrames samples; inspect the next sample; repeat with motion renewed midway
through the tail and once after refreezing. Translate by a fixed saved displacement
per sample; rotate by a saved angle and scale between saved positive finite bounds.
Keep the camera near/far, material revisions, lighting, rays and noise settings fixed
unless that row explicitly changes them. Observe model-editor gizmo and numeric
matrix paths separately from scripted queued transforms; record explicit app resets.

Compare these matched configurations, starting each from a known explicit reset:

| ID | Configuration / purpose |
|---|---|
| F0 | Metadata replay off; temporal on; adaptation off. Phase 0 fallback must preserve global history on transforms; local trails are expected. |
| L2 | Replay on with v2 consumer in a stopped host; local token validation, legacy camera rejection. |
| R3 | Replay on, v3 available, adaptation off; camera-depth rigid reprojection and four-tap validation. |
| A4 | R3 plus adaptive enabled, v4 available, moments intentionally absent; save actual adaptive tuple. |
| M5 | R3 plus adaptive enabled and v5 moments available; same tuple/workload as A4. |
| U | Each applicable renderer configuration with upscaler off, supported SDK on, and SDK unavailable. Report status/fallback independently. |

Do not describe requested modes as active modes: record availability and selected
interface. The default accumulator can select v5 whenever available; to isolate A4,
use its optional-moment failure fixture or omit the optional moment family in the
stopped host configuration. L2 is likewise a host configuration, not an invented
renderer UI switch. World-profile/custom-v1 checks stay separate from R3–M5.

For every visual row run native internal size and scale 0.5 at output 1001x701
(record the actual internal size), low/high supported effect quality, maxFrames
1/32/1024 and freeze false/true. Cover F0, R3, A4 and M5 where supported; do not
silently skip an unsupported combination. Use 1x1 and odd 3x3/2x2 synthetic probes
for boundary math. Keep quality/ray counts fixed within each matched comparison.
The maxFrames=1024 tail is 3072 subsequent scene records; a short run cannot pass it.

Diagnostics below abbreviate Surface tokens (T), Local history (H), Motion (M),
Expected previous position/normal (P/N), CurrentWeight (W), Reactivity (R), Variance
(V) and EffectiveCount (C). Select each signal (visibility, diffuse, reflected)
and the exact layer; layer zero is opaque, transparent layer is the current queue's
`(uint64_t(objectId)<<32)|submeshIndex`. Save a clean ordinary image plus separate
raw-HDR diagnostic captures. Diagnostics bypass temporal upscaling, so FSR image
acceptance requires a separate diagnostics-off run. H reason colors are profile
specific; use the [API contract](../../../doc/api/draw/TemporalAccumulator.md).
Amber means unavailable capture, not accepted history. V=0 can mean not evaluated;
CPU nominal frameCount alone cannot demonstrate per-pixel trust/convergence.

The matrix's reset column means RendererDeferred::historyGeneration change, checked
alongside accumulator/count/pixel evidence. `No` excludes the initial setup reset.
Legacy camera rejection can restart individual histories without a global reset.
Acceptable artifacts describe scoped limits, not permission to pass unrelated noise,
wrong geometry, nonfinite output, stale owners or broken chronology. Each row needs
its own Vulkan and Metal result; both columns remain not_run here.

| ID / action | Global reset | Local expectation / acceptable artifact | Required evidence | Vulkan | Metal |
|---|---|---|---|---|---|
| S01 static scene, jitter only | No | Valid pixels converge; jitter alone has zero unjittered motion; cold/uncovered pixels restart. Adaptive floor may prevent literal count saturation. | H/M/C, clean convergence sequence | not_run | not_run |
| S02 cube translation, unchanged neighbor | No | R3–M5 transport valid rigid receivers; reject new coverage/token mismatch. F0 trails accepted; unchanged background history survives globally. | T/H/M/P/N, W/C, generation | not_run | not_run |
| S03 cube rotation and nonuniform positive scale | No | Expected previous geometry/normals follow submitted transform; incompatible geometry rejects. Deformation is outside this contract. | M/P/N/H, retained neighbor pixels | not_run | not_run |
| S04 foreground reveals wall; thin alpha-cutout sweeps wall | No | Reject old foreground at newly revealed pixels; holes remain uncovered or show wall identity. Thin-edge aliasing/noise may remain; foreground radiance leaking through accepted wall is a failure. | T/H/P, cutout vs GBuffer depth, clean edge crops | not_run | not_run |
| S05 camera pan/rotation; repeat with moving cube | No | R3–M5 reproject compatible geometry; off-screen/disoccluded pixels reject. F0/L2 may locally restart for camera motion. FSR opaque vectors use previous-minus-current pixel direction. | M/P/N/H, ordinary FSR on/off | not_run | not_run |
| S06 explicit camera cut/switch; resize/projection change | Yes for explicit cut/switch/resize; projection may reject locally | No stale prior view/extent; first valid sample restarts. Use invalidateHistories for same-camera teleport. | Generation, H, actual view/extent and SDK reset status | not_run | not_run |
| S07 fixed wall receiver, AO changes from moving cube | No | Geometry may still accept unchanged receiver. Rolling tail/adaptation reduces stale visibility, but no exact occluder influence rejection is promised. | H/W/R/V/C plus AO time series | not_run | not_run |
| S08 fixed receiver, indirect light changes from moving cube | No | No exact transport dependency; valid black GI stays valid. Residual lag/chromatic ghosts may remain after finite tail. | Diffuse H/W/R/V/C and HDR finiteness | not_run | not_run |
| S09 moving cube reflected in static mirror | No | Receiver motion does not represent virtual reflected motion. Reflection lag is accepted as a known limit; hit/miss/confidence must remain distinct. | Reflected H/W/R/V/C, clean mirror time series | not_run | not_run |
| S10 opaque + two transparent layers; glass moves | No for transforms | Each layer uses its own guides/history; opaque FSR metadata stays opaque. Refraction/composited motion approximation may trail; cross-layer ownership bleed is a failure. | T/H/M for each exact layer, ordinary FSR run | Scoped F0 raster pass; AO unavailable, SDK/rest not_run | Scoped F0 raster/AO-temporal pass; SDK/rest not_run |
| S11 insert/remove/reorder membership; replace material/texture/light/environment | Yes for effective identity/content signature changes | First samples restart across old layer IDs. Stable correspondence does not waive global membership reset. In-place unreported uploads need explicit invalidation. | Generation + cached/reference signature probe, T/H | not_run | not_run |
| S12 freeze/tail/renewal, all sizes/qualities/limits above | No for transforms | Authored freeze/maxFrames unchanged; roll during motion and exactly 3*maxFrames subsequent records, then allow freeze only at saturated valid per-pixel trust. Partial support/reactivity can keep count low. | H/W/C, per-key tail fixture, exact scene indices | not_run | not_run |
| S13 presentation-only pause; update-only; two/three slots | No solely for presentation/slot use | No sample/tail consumption without a recorded key. A missing scene sequence restarts that key. Correspondence resolves submitted prior, not latest update. | Queued probe, scene/presentation counters, retained output pixels | not_run | not_run |
| S14 metadata off/unavailable, custom incompatible GBuffer, unsupported deformation | No for transform-only | F0/legacy fallback remains available. Rigid replay cannot track skinning/morph/custom vertices; trails/local rejects accepted, exact deformation transport not claimed. | Availability/status, generation, ordinary output; H only if available | not_run | not_run |
| S15 no moments; custom v1 kernel; explicit world surface | No solely for transform; one local restart on mode change | Missing v5 falls to v4; custom v1 receives only v1 layout; world uses v1/v2. Local profile changes restart. Enhanced diagnostics may be unavailable. | Forced-failure probe, interface/status, settings codec, H/W/V if supported | not_run | not_run |
| S16 unsupported/failed SDK | No solely for cube transform | Unsupported upscaler uses spatial fallback; missing opaque-motion support uses camera-only vectors. Glass/object trails may remain in fallback. | Actual SDK/motion status, ordinary FSR-disabled comparison | not_run | not_run |
| S17 duplicate/invalid identity; zero token | No solely for identity validation failure | Diagnose ambiguity, never alias another object's history. Invalid metadata is unavailable/uncovered/rejected per profile; fallback can trail. | Correspondence fixture, status, T/H | not_run | not_run |
| S18 abort recording; retained earlier outputs; explicit reset in flight | Yes only for explicit reset | Different unsubmitted command cannot become history; retained images stay immutable. Same-command accumulator order supported; renderer rigid correspondence requires submitted predecessor. | Queued/retained probes, submission ticket and pixel evidence | not_run | not_run |

Failed submission/device loss is a separate controlled fault run in a suitable
existing host, not implied by S18's abandonment test. Record propagation/cleanup
outcome; automatic recovery is not a requirement or an implemented feature. Never
read an unsubmitted output or reuse an unretired slot to manufacture a result.

### Existing probe invocation map

After authorization, include the indicated header in an existing temporary stopped
host and call the following functions in `bg2e::draw::tests`, using its initialized
`Engine& engine` and paired compiled native `std::filesystem::path shaderRoot`.
Paths are relative to repository root; calls below are C++ host expressions, not
shell commands. Run entries separately and preserve each result/exception. No
aggregate boolean should hide which optional family is unsupported.

| Header | Exact call(s) | Scope |
|---|---|---|
| tests/draw/settings_codec/probe.hpp | `runSettingsCodecProbe()` | Reflected settings, legacy/default/invalid adaptive tuple; CPU fixture. |
| tests/draw/temporal_invalidation/probe.hpp | `runTemporalInvalidationProbe(engine, shaderRoot)` | Cached/reference signature parity and reset policy; not visual acceptance. |
| tests/draw/temporal/probe.hpp | `runTemporalProbe(engine, shaderRoot)`; `runTemporalMetadataProbe(engine, shaderRoot)` | v1 scalar/HDR/freeze/tail/owners and v2 token/diagnostic cases. |
| tests/draw/temporal/reprojection_probe.hpp | `runTemporalReprojectionProbe(engine, shaderRoot)` | v3 previous geometry, camera rejection/fallback and profile transitions. |
| tests/draw/temporal/footprint_probe.hpp | `runTemporalFootprintProbe(engine, shaderRoot)`; `(engine, shaderRoot, true)`; `(engine, shaderRoot, true, true)` | Separate v3/v4/v5 all-mask, color/count/moment checks. |
| tests/draw/temporal/queued_reprojection_probe.hpp | `runQueuedTemporalReprojectionProbe(engine, shaderRoot)`; `(engine, shaderRoot, true)`; `(engine, shaderRoot, true, true)` | Separate v3/v4/v5 two/three-slot batches, isolated keys and abandoned predecessors. |
| tests/draw/temporal/adaptive_probe.hpp | `runAdaptiveTemporalProbe(engine, shaderRoot)`; `(engine, shaderRoot, true)`; `(engine, shaderRoot, false, true)` | v4, v5, forced missing optional moment module fallback. |
| tests/draw/temporal/adaptive_probe.hpp | `runAdaptiveTemporalProbe(engine, shaderRoot, false, false, true)`; `(engine, shaderRoot, true, false, true)`; `(engine, shaderRoot, false, true, true)` | v4/v5/fallback numeric metrics with capture-disabled parity control. |
| tests/draw/temporal_surface/probe.hpp | `runTemporalSurfaceProbe(engine, shaderRoot, &report)` with `std::string report` | Actual raster tokens/cutout/transparent coverage and immutable owners; save report. |
| tests/draw/temporal_surface/correspondence_probe.hpp | `runTemporalCorrespondenceProbe()`; `runTemporalCorrespondenceChronologyProbe()` | Internal CPU fixtures; require private correspondence implementation linked, not an installed-header API. |
| tests/draw/temporal_surface/rigid_probe.hpp | `runRigidSurfaceReferenceProbe()` | CPU reference transforms and motion conventions. |
| tests/draw/upscaling/opaque_motion_probe.hpp | `runOpaqueMotionProbe(engine, shaderRoot)` | Native motion generator; does not execute/validate the FSR SDK itself. |

Use the existing [surface](../temporal_surface/README.md),
[upscaling](../upscaling/README.md) and [settings](../settings_codec/README.md)
instructions for fixture-specific prerequisites and supported formats. A boolean
false is unsupported, never pass. Record actual initialization/allocation/compiler
errors; distinguish demonstrated unsupported capability from a failure. Void CPU
probes pass only if they return without assertion/exception; that establishes no
backend pixels. Do not downgrade a thrown assertion to unsupported. Real SDK,
AO/GI/reflection, editor interaction and transparency images need their own S rows
in addition to synthetic probe results.

### Per-run evidence record

Copy one record per scenario/configuration/backend and replace every pending value:

```text
Scenario / probe / configuration: Sxx or exact function + arguments; F0/L2/R3/A4/M5/U
Status: not_run | unsupported (actual reason) | failed (assertion/artifact) | passed
Revision + dirty diff identifier; exact build/shader/probe commands: pending
Backend / GPU / driver / SDK / shader deployment: pending
Scene asset + saved camera/transforms/motion schedule/noise settings: pending
Output/internal/signal extent; quality/rays; active signals/view/layer; slots: pending
Temporal maxFrames/freeze/adaptive tuple; replay/interface/format/SDK availability: pending
Warmup/motion/tail scene indices; presentation-only and skipped-key indices: pending
Expected/observed global generation; local reasons/counts/metrics; pixel tolerance: pending
Clean images + diagnostic captures + probe logs; retained-owner/submission evidence: pending
Known artifact vs unexpected failure; unsupported combinations and reason: pending
```

Run source-only checks independently with the existing Phase 2/3/4 and integration
Python checkers. Their success never changes the native matrix. P5.3 is the next
plan step and must retain native checks as not_run while explicit build/run
authorization is absent; P5.4 measurements and P5.6 default tuning remain pending.

### Authorized native host

`native_host.cpp` is a manual stopped host for the separately authorized
`runTemporalProbe` and S10/F0 checks. It is compiled directly; it adds no CMake target.
Arguments: `metal|vulkan probe|s10 SHADER_ROOT OUTPUT_DIRECTORY`. Exit 0 means its
scoped assertions passed, 3 means unavailable/partial support, 1 means an exception
or failed assertion, and 2 means invalid argument count. Do not treat all nonzero
results as unsupported. The `probe` command calls the unchanged existing fixture.

The `s10` command uses a 128x64 identity/orthographic camera, an opaque wall and two
colored semitransparent planes. It enables AO if supported, keeps metadata replay
and adaptation off, uses maxFrames=32/freeze=true and two completed slots, warms up
32 records, translates the near pane for 16 records, then records the 96-record tail
and following sample (145 total). It checks stable global generation and exact layer
IDs, absent F0 metadata, changed motion pixels, linear output until its own stopped slot reuse, and immutable
retained accumulator outputs across later slot reuse;
when AO and temporal support are available it also checks both transparent nominal
history counts. PPM snapshots are written at scene indices 31, 47 and 144.

This is scoped S10/F0 evidence, not the whole S10 configuration matrix. It does not
assert exact glass transport or per-pixel convergence from nominal counts. F0 has no
token/motion/local metric captures. Perspective/FSR-SDK on/off, other sizes/qualities,
maxFrames and freeze variants still need separate runs; this host uses the default
resampling upscaler. Fixture-only waits/readbacks are intentional, not timings.
