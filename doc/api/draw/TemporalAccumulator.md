# TemporalAccumulator and camera motion

`<bg2e/draw/pipeline/TemporalAccumulator.hpp>` and `<bg2e/draw/pipeline/MotionVectorGenerator.hpp>` are included by draw/all.hpp. They provide public draw histories independently of FSR's SDK context and private RendererDeferred targets.

RendererConfiguration::temporal accepts a compatible shared component. A null value selects the default TemporalAccumulator, enabled with maxFrames=32. RendererDeferred::temporal() exposes controls. Initialize an injected component with TemporalConfiguration containing shaderRoot/prefix, draw.temporal.v1 and a maxHistories budget (default 256). The lifecycle, beginFrame, record, invalidate and availability methods are virtual. Unsupported formats or unavailable kernels produce a raw signal pass-through; no history images are allocated in that path. Histories do not require ray-tracing support.

```cpp
renderer.temporal()->setMaxFrames(64);
renderer.temporal()->setFreezeAtLimit(false);
renderer.invalidateHistories(); // At a synchronized application mutation boundary.
```

The renderer accumulates active AO, GI and reflection signals before lighting. Histories are keyed by signal kind, view and layer, independently of frame slots. RendererDeferred uses opaque layer zero and one shared transparent layer (RendererDeferred::TransparentTemporalLayer, value 1), regardless of transparent submesh count. Standalone accumulator clients may still use their own view/layer keys. The frame sequence is actual sceneFrameNumber; UI-only presentations, picks, overlays and paused retained scene images do not submit temporal samples. Presentation serial identifies an acquired, completed resource slot only.

Standalone clients call initialize(device,backend,slots), beginFrame(slot,serial,sceneFrameNumber,sceneMotion=false) once for the retired slot, then record(TemporalInput) once per effect/view/layer. Submit that recording in sequence on the same Device queue before recording a dependent successor. Sequence numbers must be nondecreasing within a lifecycle; gaps for a key restart its convergence. Discarded/failed submission requires invalidate before further use, because recording publishes provisional chronology. Input is borrowed: sampled R16 visibility or RGBA16 radiance, SurfaceInputs, TemporalView with current unjittered/jittered matrices, external scene/reset generation and optional reflection roughness/threshold. CameraDepth and explicit world profiles reuse the public effect surface contract. Explicit stationary surfaces can set view.reproject=false; UV bake accumulation is still a distinct M04 contract.

TemporalResult::image is the sampled result. owner retains an immutable accumulated image; on disabled/unavailable pass-through it is empty and image borrows the original signal. Keep borrowed input images alive through completion. Histories own separate world position/coverage and signed normal/per-pixel count images at signal resolution. Surface and signal extents may differ; changes reset correspondence. Each submitted invocation retains both history versions, descriptors and uniforms in the acquired slot; the next frame never overwrites a live image. Returned owners may survive invalidate/cleanup but must be released before destroying the Device. Cleanup drains; logical invalidate retains submitted owners without waiting per frame.

Camera reprojection maps current reconstructed world positions into the previous jittered clip space. Prior position and world-normal tests reject disocclusion. Thresholds are absolute world distance plus relative camera distance, and a normal cosine. The previous/current unjittered matrices remain available through TemporalView and the separate MotionVectorGenerator camera reference. Its motion convention is previous-minus-current framebuffer pixels with negative clip Y; jitter alone produces no unjittered camera motion. It does not generate moving-object vectors or replace the existing FSR motion texture generator.

Visibility averages scalar [0,1] and returns one for uncovered pixels. GI averages valid RGB with alpha one; invalid GI clears the pixel history and preserves IBL fallback. Reflection RGB is accumulated weighted by fractional confidence, while confidence is averaged separately; output remains unpremultiplied. Eligible zero-confidence misses participate, but roughness-rejected/uncovered pixels cannot revive older reflections. Valid black GI/hits remain distinct from invalid signals.

maxFrames is 1..1024. Before the bound, accepted per-pixel samples use an incremental arithmetic mean. At the bound, freezeAtLimit=false uses rolling weight 1/maxFrames; true retains a converged accepted pixel until rejection/reset, except during the temporary motion window. A motion record permits rolling immediately and opens exactly 3*maxFrames subsequent consecutive static records for that signal/view/layer. Repeated motion renews the window. The last tail record still rolls; the following record can freeze. The countdown belongs to recorded history, so presentation-only work and beginFrame without record cannot decrement it. Invalid, skipped, reset or abandoned predecessors contribute no countdown. Authored freezeAtLimit and maxFrames remain unchanged. frameCount(key) and TemporalResult::frameCount report a bounded nominal consecutive-view count, not proof that every pixel converged: reprojection rejection restarts individual pixel counts. Later headless jobs must select actual rendered samples explicitly. setEnabled, frame bound/freeze and rejection controls invalidate histories.

Renderer::invalidateHistories() is the generic application hook; RendererDeferred resets effect history generation and the active upscaler. resetUpscaler() remains compatible and now invokes that full hook. Scene attachment, resize, explicit camera cuts/switches, general inspector edits and environment replacement reset it; typed transform-only edits follow the transform policy. An exact extraction/settings signature also detects mesh/material/resource identity, transforms, lights, camera identity/projection, effect quality/mode/noise and temporal/shadow settings. Node/submesh identity must be unique when temporal correspondence is active. Transform-only movement preserves global histories and FSR and supplies sceneMotion to the accumulator; full fingerprints remain transform-sensitive. Optional rigid metadata generation and v3 history transport are described below; native validation remains pending. Moving surfaces and changing indirect lighting/reflections on static surfaces can trail, as can FSR's camera-only reconstruction. The finite update window is approximate and does not guarantee convergence. Notify invalidateHistories for camera teleports on the same scene camera, in-place image uploads and custom shader/resource edits that retain identity. Applications remain responsible for safe mutation boundaries and requested redraws, including partial editing failures. Generic material utilities acquire no renderer ownership.

See [ABI and implementation handoff](../../../docs/render_to_draw/plan/milestone_03_ray_tracing/temporal.md) and [manual fixture](../../../tests/draw/temporal/README.md). Compilation/runtime validation remains with the project lead. [Spatial filtering](DenoiseFilter.md) is available after temporal accumulation.

RendererDeferred applies the optional public [DenoiseFilter](DenoiseFilter.md) after temporal resolution and before lighting; filtered output is separate from temporal history.

## Live controls and current use

Both existing examples expose this component through the live [settings catalog](Settings.md).
Compatible replacement metadata must return its matching settingsInstance address.
Requested enabled/settings remain serializable independently of availability/status;
check graph selection as well. [SurfaceInputs](SurfaceInputs.md) and
[TracingSceneResources](TracingSceneResources.md) describe reusable inputs/owners.
Device must outlive retained results/snapshots. No UI, exact bake-average or
headless application scheduler is implicitly created by a standalone effect.

## Retired temporary storage

History signal/position/normal images and invocation uniform buffers/sets use
[TemporaryResourcePool](TemporaryResourcePool.md). Current histories and previous
histories still read by any slot retain separate owners; externally retained result
owners cannot become scratch storage. Frame-slot count is not reduced to a fixed
two-image ping-pong. Pool reuse does not relax any camera, sequence, settings,
classification or history-generation invalidation rule.

TemporalConfiguration::poolLimits controls retained cache bytes, image/set counts
and idle age. `poolStatistics()` reports retention; `setPoolingEnabled(false)`
restores fresh-storage diagnostics. Logical invalidate still keeps submitted
invocation owners alive. Failed/abandoned recordings cannot become input history
for a different recording; retrying their scene sample starts from a fresh history.
Previously submitted history and earlier samples within the same command retain
the normal ordered graphics-queue behavior. No new per-frame wait is introduced.

RendererDeferred now reuses [revision-keyed scene signatures](TemporalInvalidation.md).
The reset policy excludes only instance transforms; exact cached snapshot equality
and scene fingerprints retain them. Camera thresholds, previous surfaces, transparent
correspondence, disocclusion and explicit generation checks remain conservative.
Object reprojection requires the explicit optional v3/receiver metadata path below; effect-local material/light reset is not enabled.
Custom beginFrame overrides must adopt the trailing bool motion parameter and
forward it when using base record; existing three-argument callers default to false.
Rebuild binary consumers against this changed C++ interface. Application-level
explicit resets remain authoritative, including general editor callbacks.


## Optional receiver metadata (source implementation; native validation pending)

`TemporalInput::metadata` is an optional trailing retained token/depth pair. Both
images must be same-device sampled 2D images at receiver extent; token is R32_UINT
and depth is D16_UNORM or D32_SFLOAT (matching CameraDepth format). Supplied malformed
pairs throw even when the optional consumer is unavailable. Signal extent may differ.

The primary `draw.temporal.v1` layout and 208-byte uniforms remain unchanged.
`TemporalConfiguration::metadataShaderInterface="draw.temporal.v2"` provisions a
separate optional pipeline family (`metadataShaderPrefix`, default
`draw/temporal_accumulation_v2`). Its initialization failure preserves v1 and exposes
`surfaceMetadataAvailable()/surfaceMetadataStatus()`; ordinary frames never retry.
Custom/injected configurations do not request this interface by default. Renderer
owned accumulators provision it, while receiver replay still defaults off.

V2 appends current VP at byte 208 and a depth tolerance vec4 at byte 272 (288 bytes).
Vulkan sampled bindings 13/14/15 map to Metal textures 11/12/13: current uint token,
current sampled depth, previous uint token. Flags y/z indicate current/previous
metadata validity. Enhanced dispatch requires a supplied pair; absent pairs use v1.
Missing previous tokens bind the correctly typed current token as a safe placeholder
and flag z rejects acceptance. V2 kernels also bypass metadata rejection when flag y
is zero; callers of that ABI must still bind correctly typed valid images.

Current samples use signal texel-center normalized UV mapped to receiver pixels.
Previous UV is checked strictly before nearest metadata lookup, with no edge clamp.
Acceptance requires matching nonzero tokens and current receiver depth agreement
(2/65535 for D16, 2e-6 for D32). World-profile receivers project their current world
position through the jittered VP. Existing position, normal, camera, coverage and
signal checks still apply. Metadata rejection restarts the local raw sample; it does
not change coverage or globally reset histories. Previous metadata owners live with
History and retained invocations, including submitted work across slots.

Renderer selection uses opaque metadata only for layer zero and the single shared
transparent metadata set for layer one. It never substitutes opaque tokens or
previous-surface metadata for a missing transparent receiver. During transparent
effect recording, the renderer supplies the batch's actual scratch receiver depth
alongside its tokens; the public metadata replay depth remains the preserved opaque
visibility attachment. Transparent diagnostics consume the batch guide before
per-item color composition reuses scratch. See [transparent depth ownership and
recording order](RendererDeferred.md#transparent-layer).
P0 motion thawing, reflection confidence, valid-black GI and nominal per-key counts
are preserved. No ordinary submission, CPU wait or readback was added.

`runTemporalMetadataProbe` adds a stopped-host native fixture for odd 3x3 receivers
and 2x2 signals, unchanged/local/zero tokens, depth mismatch and previous UV outside
the screen. The fixture is source only: C++/shader compilation and Vulkan/Metal
execution remain not_run. Broader scene/diagnostic acceptance is P1.5.

## Phase 1 diagnostics (P1.5 source implementation)

Renderer diagnostics expose token colors and the actual per-pixel history acceptance
through the existing reflection/UI wrappers. `temporalDiagnosticView` defaults Off;
`temporalSurfaceEnabled` remains a separate default-off replay opt-in. Signal and exact
receiver layer are selectable. Settings latch at scene update. Missing optional
support or a missing/skipped layer preserves ordinary output; no opaque substitution.
Raw HDR final display preserves diagnostic colors. Token colors hash canonical uint
tokens (zero black); local acceptance is red restart/first sample, green accepted,
black uncovered and amber when the selected current key has no acceptance capture.

`TemporalInput::diagnoseAcceptance` is a trailing optional bool, default false.
Built-in diagnostic profiles use the same v2 uniform block (288 bytes), add storage
binding 16 / Metal texture 14 (RGBA16), and write the actual accepted flag separately
from the accumulated signal, position and normal. V1/v2 production layouts and math
are unchanged. This works even when maxFrames=1 and does not infer acceptance from
TemporalResult::frameCount. Custom v2 shader families do not silently run built-in
diagnostic accumulation. Diagnostic initialization failure leaves base v2/v1 usable.

`recordDiagnostic` converts a retained token and optional current acceptance image
to receiver-extent RGBA16, using normalized texel centers for lower-resolution signal
masks. Its 16-byte uint4 uniform maps to buffer 1. Vulkan bindings 1/2/3/4 map to Metal
token texture 0 / float acceptance texture 1 / sampler 0 / output texture 2. Integer
tokens never bind through float samplers. Missing acceptance binds a safe float zero
image and sets a false flag. Invocation owners retain token/depth, current history,
output image and resource leases across completion. Renderer snapshots can be obtained
with temporalSurfaceMetadata at a synchronized frame boundary.

Only an explicit LocalHistory request allocates the selected key's signal-extent
acceptance image; active views allocate a receiver-extent display image. Off adds no
per-frame diagnostic image/dispatch. Diagnostic images use the spatial reconstruction
fallback to keep display colors out of temporal-upscaler input. No per-frame CPU wait,
submission or readback was added. Direct setters do not invalidate histories; queued
settings retain the generic settings-import reset contract.

New tests/draw/temporal_surface fixtures cover correspondence chronology/identity and
native raster-to-scalar acceptance, alpha coverage versus actual GBuffer depth,
transparent domains/depth, two slots, abandoned scenes and retained output owners.
The existing metadata fixture adds diagnostic/malformed-pair/maxFrames=1 assertions.
See that directory's README for exact coverage, fallback reporting and unrun manual
scenarios. These sources and all new shaders are uncompiled/unexecuted. Vulkan/Metal
formats, pixels, visual acceptance and performance remain not_run. Phase 1 is
implemented_unverified; native acceptance remains pending. Tokens do not validate
indirect-light changes on stationary receivers. Phase 1 source handoff originally advanced to P2.1; current Phase 2 state is recorded below.

The reflected `temporalDiagnosticLayer` setting uses unsigned decimal text because
the reflection scalar catalog does not support uint64. The C++ getter/setter retain
the uint64 API. Parsing rejects empty, signed, overflowing or trailing text before
mutating renderer state; the complete 64-bit transparent key is preserved.

## Expected previous-surface consumption (P2.3)

`TemporalSurfaceMetadata` appends retained `previousPosition` (RGBA32F) and
`previousNormal` (RGBA16F), plus `legacyCompatible=true`. The two new images must be
supplied together with token/depth, as same-device sampled 2D images at receiver
extent. Malformed/partial/wrong-format/wrong-extent inputs throw even when optional
v3 is unavailable. Both metadata w components encode validity. Existing token/depth
aggregate initializers remain valid and explicitly retain the static v2 contract.
A metadata producer that emits moving canonical tokens must set legacyCompatible
false for such a domain; stripping that flag/pair is not an object-motion contract.

Standalone opt-in requires `metadataShaderInterface="draw.temporal.v2"` and
`reprojectionShaderInterface="draw.temporal.v3"`. The latter has its own optional
`reprojectionShaderPrefix` (default `draw/temporal_accumulation_v3`), availability
and status (`surfaceReprojectionAvailable/Status`). Initialization failure latches
fallback without retries or disabling v1/v2. Only camera_scalar/camera_hdr profiles
are defined for v3. Custom families use their requested prefix and never silently
receive canonical acceptance kernels. Built-in acceptance diagnostics use
`draw/temporal_accumulation_v3_diagnostic` independently of core v3 initialization;
missing capture displays the existing unavailable/amber result.

V3 uses a 352-byte uniform: the exact v2 288-byte prefix plus previousView at byte
288. C++ and Metal assert sizes/offsets; GLSL fixes previousView's offset explicitly.
Bindings 0–15 preserve v2; sampled bindings 17/18 map to Metal textures 15/16 for
expected previous world position/normal. Diagnostic storage remains binding 16 /
Metal texture 14. V1 uniforms (208), v2 uniforms (288), kernels and binding sets
remain unchanged. No new textures are bound to v1/v2/custom legacy kernels.

Selection requires v3 availability, the full previous-surface pair, CameraDepth,
view.reproject=true and an affine view matrix. World-explicit and stationary-explicit
profiles retain their v2/static or v1 fallback; this step adds no implicit explicit-
world correspondence contract. An incompatible moving domain uses Phase 0/v1.
Each history records its actual metadata mode (v1, v2 or v3). Mode changes restart
that key's history locally; a history recorded without metadata cannot become a
valid v3 predecessor. Existing sequence, recording owner, generation, reset,
projection, surface profile/extent, signal extent and roughness checks remain.
The legacy 0.001 camera-matrix rejection is bypassed only for supported v3 input.
Projection changes still restart; same-camera cuts/teleports require explicit
invalidateHistories/resetUpscaler (or standalone accumulator invalidation).

The current signal texel center selects the current receiver pixel as before.
V3 reads expected previous world position/normal there and projects the expected
point through the matching history's previous **jittered** view-projection. This
implements the world-point route, including jitter delta, without consuming or
rescaling the unjittered motion attachment. Finite/valid expected geometry,
positive clip w, zero-to-one previous clip depth and strict in-bounds UV are
required before integer previous-pixel conversion. Current metadata depth must
agree with the actual current receiver depth (existing D16/D32 tolerances).
V3 uses q=previousUV*historyExtent-0.5, floor(q) and the four standard bilinear
weights. Each positive-weight in-bounds tap maps its signal texel-center UV into
the actual retained token image extent and requires that integer token to match
the current nonzero token. Out-of-bounds taps are skipped without edge clamping.

Previous history position is compared to expected previous position, and previous
history normal to normalized expected previous normal. The exact position tolerance
in both shaders and the CPU oracle is:
`absolute + relative * length((previousView * vec4(expectedPreviousPosition,1)).xyz)`.
Expected and previous geometry, sample/count and tolerance must be finite. Previous
position coverage and normal/count validity remain required. Both expected and
candidate previous-view depths (-cameraPosition.z) must be positive and differ
by no more than that same tolerance; full world distance and normal cosine tests
also apply. No motion/footprint-dependent tolerance widening is enabled.

Each tap validates finite signal/guides/count before contributing. GI requires
coverage alpha > .5 and retains valid black. Reflection accumulates spatially
confidence-premultiplied RGB and confidence, then recovers unpremultiplied RGB;
zero-confidence misses contribute support/count but no radiance. Valid weights
are renormalized only when their sum W > 1e-6. Effective previous count is the
minimum positive-weight valid count, bounded by maxFrames, and the current sample
increments it once. Internal geometricConfidence=clamp(W,0,1) is footprint support,
independent of reflection confidence, reserved for future adaptive weighting;
there is no permanent confidence image or new dispatch/binding/ABI.

Current coverage is
independent of correspondence validity: rejected correspondence restarts the raw
sample and stores current world guides for the next scene. Temporal averaging
keeps the existing per-signal confidence rules. Freeze applies only to accepted
correspondence with saturated minimum count; rejection restarts even when freeze
is authored. Phase 0 rolling/freeze countdown is unchanged. V1/v2/custom families
keep their existing sampling behavior. Source/math checks passed; C++/shader
compilation, native pixels, visual acceptance and performance remain not_run. No reflected/deformed surface support is added.

RendererDeferred provisions v3 for its own accumulator, but replay still defaults
off. It passes the exact opaque or exact transparent domain's full retained pair
through temporalSurfaceMetadata, never borrowing opaque metadata for a missing
layer. The P2.2 blanket moving-domain gate is replaced by supported v3 selection;
injected v1/v2 components retain their capability/static admission fallback.
recordLinear uses the captured slot camera consistently for reconstructed surfaces,
TemporalView, denoise/effect camera inputs and transparent projection/order, so a
later live camera update cannot disagree with the recorded metadata matrices.
Histories and invocations retain the consumed token/depth/previous-geometry owners
with their matching submitted predecessor; no ordinary wait, immediate submission,
CPU image/counter readback or motion-driven global reset is introduced. The motion
attachment remains unconsumed here; FSR integration is P2.4.

Source review and independent arithmetic checks passed. C++/shader compilation,
Vulkan/Metal execution, actual format support, pixels, visual acceptance and
performance remain not_run. The new stopped-host runTemporalReprojectionProbe
covers reduced 2x2 signal / 3x3 receiver transport with simultaneous camera/position/
normal changes, local token/geometry/normal/out-of-bounds/depth rejection, retained
owners, profile transitions, cuts and projection changes. The existing metadata
fixture adds six malformed previous-pair cases. The surface fixture explicitly
isolates its old v2 token-only synthetic consumer; it does not claim v3 transport.
The rigid CPU oracle now includes the previous-camera-space tolerance formula.
All C++ fixtures remain uncompiled/unexecuted; queued multi-slot real-raster/HDR/
normal-map/visual scenarios remain P2.5/native acceptance work.

## Phase 2 inspection and handoff (P2.5)

TemporalDiagnosticView appends Motion=3, PreviousPosition=4 and PreviousNormal=5,
preserving existing values. A separate optional temporal_diagnostic/surface shader
uses the unchanged 16-byte display ABI and uint-token/selected-float texture pair;
surfaceDiagnosticsAvailable reports its readiness independently. Missing per-view
source returns no display; malformed selected format/extent/device/usage throws.
Original token/actual-acceptance kernels and accumulation v1/v2/v3 are unchanged.
The renderer exposes matching read-only temporalSurfaceDiagnosticsAvailable.

TemporalSurfaceMetadata appends an optional retained motion image after
legacyCompatible. Accumulation does not consume/retain this unused motion field;
display invocations retain it only when read. Motion colors map xy as 0.5+xy/64,
clamped to [0,1], with B=0.5. Previous position maps signed world axes using
0.5+0.5*xyz/(1+abs(xyz)); normals map 0.5+0.5*xyz. Invalid/zero-token/nonfinite
values are black, and position/normal require valid w. Use Raw HDR to inspect.
These colors are bounded diagnostic encodings, not lossless numeric exports.

All views are opt-in, exact-domain, receiver-sized RGBA16 outputs using existing
GPU timing; they add no ordinary readback/wait or permanent confidence images.
Replay stays default off. Phase 2 source implementation and regression sources
are complete (implemented_unverified); C++/shader compilation, all native/pixel/
visual/performance acceptance remain not_run. See the Phase 2 plan's P2.5 native
matrix and tests/draw/temporal_surface/phase2_source_checks.py for reproducible
source/math checks; Phase 3 progression and native acceptance are tracked separately in the plan.

## P3.4 built-in v3 footprint reasons

LocalHistory keeps the existing opt-in acceptance image/display allocation. Built-in
v3 diagnostics now encode the following reason colors in RGB (alpha remains one).
V1/v2/custom kernel behavior and the display ABI are unchanged. Amber (1,.5,0)
continues to mean no capture for the selected key, not a pixel rejection reason.

| Order | Meaning | RGB |
|---|---|---|
| 0 | No admitted predecessor (first frame, reset or chronology rejection) | (.25,.25,.25) gray |
| 1 | Unsupported/invalid expected correspondence or missing metadata flags | (.5,0,1) violet |
| 2 | Off-screen/behind-camera/invalid projected domain | (0,.25,1) blue |
| 3 | Uncovered current receiver or previous position/count coverage | (0,0,0) black |
| 4 | Zero/different surface token | (1,0,1) magenta |
| 5 | Position/positive previous-view depth/current metadata-depth mismatch | (1,0,0) red |
| 6 | Invalid/mismatched previous normal | (1,1,0) yellow |
| 7 | Invalid signal/absent GI coverage/insufficient finite support | (1,1,1) white |
| 8 | Accepted partial footprint | (0,1,1) cyan |
| 9 | Accepted full footprint | (0,1,0) green |

A surviving footprint takes precedence over failures in rejected neighbors. Full
means W >= 1-1e-6; partial means 1e-6 < W < 1-1e-6, not a count of four taps.
An exact texel center can have full support from one tap. When no footprint
survives, the earliest evaluated failure in the ordered rejection list wins,
independent of tap order. Stages whose prerequisites fail are not sampled merely
to discover hypothetical downstream errors. At the candidate identity gate,
coverage is inspected in diagnostic kernels so uncovered beats token mismatch
when both fail. Invalid count is uncovered; nonfinite position/depth is geometry,
nonfinite normal is normal, and nonfinite radiance is signal. A first frame remains
gray even if other inputs also fail. Zero-confidence reflection misses remain
valid, so they can display cyan/green while radiance is black.

All new bookkeeping and any extra coverage reads at token-rejected taps are under
TEMPORAL_DIAGNOSTIC. With it disabled the production shader tokens are unchanged
from P3.3. No counters, new bindings/formats, permanent images, synchronization,
CPU telemetry or ordinary-frame readback are added. The capture remains an
optional RGBA16F image at signal extent plus the existing optional RGBA16F display
at receiver extent; retained/in-flight owners and pool high-water allocations
still matter when measuring total cost.

## Phase 3 profiling and cost status

The existing effect.temporal.record CPU/renderer-GPU labels remain. Admitted
opt-in captures add effect.temporal.diagnosticCapture CPU/GPU scopes; admitted
displays add effect.temporal.diagnosticDisplay alongside renderer.temporalDiagnostic.
The capture label measures the decorated temporal pass, not isolated coloring
cost. Missing/Off/unavailable diagnostics add no such scope. Existing GPU pass/
detailed timing switches and timestamp availability still apply; no synchronous
telemetry is added, and nested scopes must not be summed as separate frame costs.

Phase 3 allocates no mandatory image beyond Phase 2. One scalar history set costs
26 bytes per signal pixel, one HDR history 32; one metadata domain costs 32+D bytes
per receiver pixel (D=2/4 for D16/D32). Optional capture/display cost 8 bytes per
signal/receiver pixel respectively. Deduplicate shared metadata across effects;
include previous/current/in-flight generations, external output owners and idle
pool high-water allocations. These content-byte estimates exclude driver overhead
and are not total resident memory. poolStatistics covers its own temporal pool,
not every metadata/renderer/external allocation. Full accounting and the pending
nearest/four-tap timing experiment are recorded in the Phase 3 plan's P3.5 review.

Source/math/cost checks pass via tests/draw/temporal/phase3_source_checks.py.
C++/shader compilation, native pixels/visuals, timestamp availability, performance
and measured peak allocation remain not_run. Source completion does not enable
replay or adaptive settings by default. Next source step is P4.1.

## P4.1 optional bounded adaptive weighting (source implementation)

TemporalSettings appends adaptiveEnabled=false, minCurrentWeight=.03125,
reactiveCurrentWeight=.25 and historyClipSigma=3. The atomic
setAdaptiveSettings(enabled, minimum, reactive, sigma) rejects nonfinite values,
requires 0 <= minimum <= reactive <= 1 and 0 <= sigma <= 10, and invalidates once
when the tuple changes (no invalidation for an identical tuple). Sigma is reserved
for later clipping; it does not clip history in P4.1.

All four fields are reflected and appended, in that order, after normalThreshold
in cached equality/payload and full-reference signatures. Old preference payloads
remain readable: omitted fields retain live values, which are the defaults on a
fresh accumulator. The standard temporal catalog validates the complete proposed
tuple before applying any property. Individual reflected weight accessors stage
the companion bound to permit either order of a valid multi-property import;
these accessors are intended for the validated catalog. Direct API callers should
use the atomic tuple setter, which strictly rejects inconsistent pairs. Existing
catalog property setters and the post-commit renderer invalidation still operate;
this does not introduce per-frame setters or a new transaction/reset protocol.

The new trailing TemporalConfiguration fields are adaptiveShaderInterface (empty
by default) and adaptiveShaderPrefix (draw/temporal_accumulation_v4). Explicit
interface draw.temporal.v4 requires available v3 camera-depth reprojection. It
initializes independently; unavailable native kernels leave v3 nonadaptive usable
and report adaptiveAvailable()/adaptiveStatus(). Failed optional initialization
is not retried during ordinary recording. The owned RendererDeferred accumulator
provisions v4, but both replay and adaptiveEnabled still default off. Injected
v1/v2/v3/custom families receive their original uniforms and bindings. A custom
family must explicitly declare v4 to receive the new layout.

V4 appends a vec4 at byte 352 to the exact 352-byte v3 prefix (368 bytes total):
minimum current weight, reactive current weight, reserved sigma, enabled flag.
The existing previousView offset stays 288. Resource bindings and signal/guide
formats are unchanged; separate v4 scalar/HDR production and diagnostic pipelines
keep old uniform sizes 208/288/352 intact. Diagnostics use the existing optional
acceptance binding/colors and fail independently from core v4. Pool binding leases
are keyed by selected layout and uniform size. No new image or persistent moment
storage is allocated. Per-key mode 3 denotes v4; switching modes restarts that key.

For accepted history N=min(minimum contributing count,maxFrames),
alpha0=1/min(N+1,maxFrames). Permitted adaptive updates use
alpha=clamp(max(alpha0,minCurrentWeight,(1-geometricConfidence)*reactiveCurrentWeight,
reactivity*reactiveCurrentWeight),0,1), with reactivity=0 until meaningful current
statistics are implemented. Scalar/GI and confidence-premultiplied reflection
mixing retain their existing arithmetic, using oldWeight=1-alpha. Full support
retains the floor; partial support can increase the current contribution. Rejected
correspondence restarts at current weight one/count one. Counts still increment
once and are bounded; trust-count adjustment is deferred to P4.3.

When the authored freeze is effective and minimum valid count is saturated, v4
skips the adaptive override and preserves the nonadaptive frozen result. The
existing motion/3*maxFrames rolling window remains the only temporary freeze
exception. Disabling adaptation selects the exact v3 baseline; missing v4 support
also selects nonadaptive fallback. No neighborhood gathering, clipping, moments,
new ray counts or detection of changing indirect illumination is claimed yet.

Source/reference checks pass via tests/draw/temporal/phase4_source_checks.py, with
Phase 2/3 checks retained. Settings codec, cached/reference native fixtures and
runTemporalFootprintProbe(engine,shaderRoot,true) have been expanded as source;
C++/shader compilation and all native pixels/visual/performance checks are not_run.
Next step: P4.2 current-neighborhood statistics only.

## P4.2 current-neighborhood statistics (source implementation)

Opted-in v4 accepted/nonfrozen updates now have an internal CurrentStatistics
helper for a bounded 3x3 raw-current-signal neighborhood. Each signal texel center
maps independently to current receiver resolution. Only same nonzero token,
finite coverage/signal, matching receiver/metadata depth, reconstructed current
world distance within the existing absolute/relative tolerance, and compatible
normalized current normal can contribute. No temporally resolved/output neighbor
is read. The metric distance test conservatively restricts lateral separation as
well as depth; strict thresholds may leave insufficient support.

Uniform valid weights produce statistic mean/secondMoment/variance, statistic and
componentwise RGB min/max and sampleCount. Scalar samples clamp to [0,1]; radiance
uses nonnegative luminance with coefficients (.2126,.7152,.0722), retaining raw RGB
for bounds. GI black with valid alpha is included. Reflection bounds include only
confidence>1e-6 and roughness-eligible samples; zero-confidence misses retain their
separate valid temporal meaning. Alpha/confidence and the output signal are not
modified by this helper. Both backends use epsilon=1e-6 and float32 arithmetic.

varianceValid is false below two valid samples, or for nonfinite statistics.
No samples produce explicit zero fields/count zero, not usable clipping bounds.
P4.3 must gate variance-driven behavior on varianceValid. In P4.2 the local result
is reserved: reactivity remains zero and no clipping/count/output change consumes
these statistics. Compilers may remove unused gathering until that consumer is
implemented. No new ABI, image, moment storage or runtime cost is claimed.

neighborhood_reference.py and Phase 2/3/4 source/reference checks pass. No shader
compilation, native statistics/projection/transparent/HDR acceptance, visual test
or benchmark was run. Next step is P4.3 clipping/reactivity/trust-count integration;
moments remain P4.4.

## P4.3 clipping, reactivity and effective trust (source implementation)

Accepted, enabled, nonfrozen v4 updates now consume CurrentStatistics. With at least
two valid samples and finite statistics, intersect statistic min/max with
mean +/- historyClipSigma*sqrt(variance+1e-6). Scalar history clips to this interval.
HDR uses historical nonnegative luminance and scales RGB by clipped/original
luminance when the original exceeds 1e-6; near-zero history uses the componentwise
current-neighborhood bounds. Empty/nonfinite intervals retain unclipped history
and zero reactivity. Confidence/alpha is preserved independently.

HDR scaled channels are bounded to [-65504,65504], the finite FP16 history storage
range. Otherwise a target luminance valid in FP16 could push a saturated RGB
channel beyond that format. This safety bound can prevent exact target luminance;
luminance-only scaling can retain hue/chromatic artifacts, including equal-luminance
color changes. It does not provide perfect specular reconstruction.

Reactivity uses the original, unclipped historical scalar/luminance:
clamp((abs(Lh-mean)-sigmaStd)/(abs(Lh-mean)+sigmaStd+1e-6),0,1).
It feeds the P4.1 maximum-current-weight rule together with baseline alpha0,
minimum weight and geometric support. Insufficient statistics leaves clipping off
and reactivity zero, while support/minimum-weight adaptation remains available.
Temporal reflection mixing remains confidence-premultiplied, with separate
confidence accumulation and unpremultiplication only above its existing epsilon.
Zero-confidence reflection misses do not contribute arbitrary RGB through alpha.

When chosen alpha exceeds alpha0, cap previous effective count to
min(N,max(1,1/max(alpha,1e-6)-1)) before incrementing once and bounding by maxFrames.
The per-pixel guide count now measures trust and can be fractional, quantized by
existing RGBA16F storage. It remains >=1 for valid prior history and <=maxFrames;
geometry rejection restarts at one, uncovered pixels at zero. TemporalResult's CPU
frameCount remains nominal consecutive scene frames and is not this trust value.
An accepted alpha=1 can store count two (the specified cap has minimum one before
the increment); geometry rejection still stores one.

Already saturated valid pixels outside the motion window skip statistics/response
entirely and keep their frozen result. Repeated reactivity, low support or a
minimum weight above 1/maxFrames can keep effective trust below saturation, so
nominal elapsed frameCount alone does not guarantee freezing. No motion-window,
maxFrames, authored freeze preference or reset policy is changed. Disabling
adaptation still selects the exact v3 baseline. No new uniform/binding, image,
moment state, settings or synchronization is introduced.

history_response_reference.py covers clipping/ranges, noise and insufficient
support, unclipped reactivity, near-zero/HDR storage/chromatic limits, reflection
miss/hit confidence, fractional trust and rejected/frozen/disabled paths. The
optional adaptive footprint fixture now expects the constant-current clipping
result and includes eight stable frames followed by a reactive step (GPU trust
two versus CPU frameCount nine), saturated freeze and motion thaw. These C++
assertions are source only. All source/reference checks pass; C++/shader compilation,
native HDR/pixels/queued/freeze/visual quality and cost remain not_run. Next: P4.4
optional temporal moments, with neighborhood-only v4 retained as the fallback.

## P4.4 optional moments (source implementation)

TemporalConfiguration appends momentShaderInterface (empty by default) and
momentShaderPrefix (draw/temporal_accumulation_v5). Explicit draw.temporal.v5
requires available v4 and RG16F sampled/storage/transfer support. It initializes
its pipelines and one typed 1x1 zero-moment image independently; format/layout/
shader/dummy initialization failure reports momentsAvailable()/momentsStatus()
and retains neighborhood-only v4. There is no ordinary-frame retry. Built-in
RendererDeferred provisions this optional interface while replay and adaptation
remain default off. An external configuration with empty moment interface selects
the v4 baseline; v1–v4/custom families retain their declared resource maps.

V5 retains the exact 368-byte v4 uniform and adds sampled binding 19 (Metal texture
17) for previous RG16F moments and storage binding 20 (Metal texture 18) for current
RG16F moments. Its diagnostic family independently retains acceptance binding
16/Metal14. Mode 4 denotes per-key v5; mode switches or missing predecessor moments
restart that key. A missing predecessor binds the typed zero image, guarded by
existing history-valid flags. Selected layout/uniform identity remains part of
pool binding leases. No new settings, preference fields or full-signature payload
are needed for the immutable optional interface.

Each active v5 signal/view/layer owns a separate moment image at signal extent.
History and per-slot previous/next invocations retain it until retirement, with
the existing submitted/abandoned admission, pruning and cleanup drain. Moments
are never shared between signals, views or transparent layers. Extra content cost
is 4 bytes per signal pixel: 8.2944 decimal MB at 1080p or 33.1776 MB at 4K for one
image, before current/previous/in-flight/external/pool retention and driver overhead.
Scalar/HDR history sets become 30/36 bytes per signal pixel with moments. The
existing pool byte accounting already handles RG16F; maxHistories/pruning remain,
but idle-retention limits are not a cap on live images. The optional dummy adds
one tiny allocation per v5 accumulator lifecycle, not per key/frame.

Visibility moments use clamped visibility in [0,1]. HDR uses
clamp(log2(1+max(luminance,0)),0,16); first/second moments are bounded by 16/256,
so squaring cannot overflow half storage. Finite input validation precedes the
transform. For reflection confidence-zero/epsilon misses the temporal statistic
is zero, independent of arbitrary miss RGB; eligible confident hits use radiance.
Moments track the raw incoming statistic, not the clipped resolved RGB or its
alpha-weighted radiance. Current spatial moment statistics transform each valid
neighbor before averaging (mean(log), not log(mean)). Alpha/confidence behavior
in the color path stays separate.

Each color candidate additionally requires a finite, in-range previous moment
pair. Invalid pairs reject the joint tap locally (invalid-signal diagnostic color),
so color/moments use exactly the same accepted taps, raw support and normalized
weights. Moment variance tolerates half quantization by max(m2-m1*m1,0); it does
not require m2>=m1*m1 before validation. Rejected color correspondence initializes
current moments; uncovered/invalid signal clears them. Accepted moments mix with
the same oldWeight/effective alpha as color. Frozen history transports the previous
pair without blending current statistics; retained image owners are not mutated.

V5 uses moment-domain previous mean versus current moment-domain spatial mean to
estimate reactivity. Below four previous effective samples it uses only current
spatial moment variance. From four samples onward it conservatively uses the
maximum of current spatial and temporal variance, then applies sigma/epsilon in
that same domain. Insufficient current support still gives zero reactivity.
Linear neighborhood variance continues to control P4.3 luminance clipping;
log variance is never fed into its linear bounds. Geometric/minimum floors, trust
cap and literal saturated freeze remain unchanged.

moments_reference.py and the Phase 2/3/4 source/reference checks pass. The native
footprint source accepts (engine,shaderRoot,true,true), inspects retained RG16F
pairs for every mask, same-alpha transport, old owners and frozen moments, but
is uncompiled/unrun. Actual format/deployment, HDR precision, moment unavailability,
queued per-key retention, pixels/visuals and costs remain not_run. P4.5 regression
and visual acceptance sources are next; native execution still requires permission.

## P4.5 regression/acceptance status

Phase 4 regression sources now include a matched adaptive/nonadaptive scalar/HDR
fixture and optional v4/v5 queued variants with moment/key retention. The combined
half-quantized sequence reference covers changing and noisy signals, insufficient
support, freeze/settling, drops/gaps, finite HDR, confidence and fallback. Source
checks pass. Its convergence/noise numbers are reference-model evidence only.
Production sources/defaults are unchanged in P4.5; C++/shader compilation and all
native/visual acceptance remain not_run. The Phase 4 plan's P4.5 matrix records
exact synthetic host entries, unavailable-module versus unavailable-format cases,
controlled AO/reflected-only/GI/pan/edge scenes and common ROI/noise criteria.
Global generation/frameCount alone cannot establish visual correctness.
Next: P4.6 source cost/diagnostic review and handoff; no implicit native execution.

## P4.6 opt-in adaptive diagnostic views and final source status

TemporalDiagnosticView appends CurrentWeight=6, Reactivity=7, Variance=8 and
EffectiveCount=9; prior values 0–5 are preserved. Renderer settings reflect these
views plus temporalAdaptiveDiagnosticsAvailable. The accumulator exposes
adaptiveDiagnosticsAvailable()/adaptiveDiagnosticsStatus(); availability is generic
across supported canonical capture families, not a promise that a particular
signal/layer currently has a capture. Renderer capability also requires metadata
pass availability. Replay/adaptation are never enabled by selecting a view.

TemporalInput appends diagnoseAdaptive=false. It is mutually exclusive with
diagnoseAcceptance; requesting both throws before recording/mutating history.
Only an admitted adaptive camera-depth v4/v5 domain and its available canonical
metric pipeline allocate a signal-sized RGBA16F capture. Separate internal
v4_metrics/v5_metrics families add storage binding 21/Metal texture19 to their own
layouts, with the existing 368-byte uniform. Core/custom/v1–v5 and LocalHistory
families never receive that binding. Canonical prefix guards and independent
initialization failures preserve existing kernels; no ordinary retries occur.
The matching optional metrics display uses its own pipeline/layout and the existing
16-byte display resource ABI. Capture failure can leave core accumulation usable.

The renderer requests only the latched diagnostic signal and exact opaque/
transparent layer. recordDiagnostic requires the matching current sequence,
recording completion owner and metadata token owner to use that capture. Stale,
skipped or disabled captures are never reused. Missing capture displays amber
(1,.5,0); unavailable display/metadata returns no diagnostic image. The display
samples the capture at receiver extent and produces grayscale for the chosen
channel, with alpha one. Zero tokens/nonfinite selected values are black.

| View | Captured channel / interpretation |
|---|---|
| CurrentWeight | R: actual current contribution 1-oldWeight; one for covered restart, zero for accepted saturated freeze |
| Reactivity | G: actual signal-change reactivity used by the selected v4/v5 path |
| Variance | B: variance/(1+variance), a bounded visual encoding that can saturate at half precision |
| EffectiveCount | A: post-response per-pixel trust count/maxFrames, not CPU nominal frameCount |

Variance is the estimate used for response: linear scalar/luminance spatial
variance in v4; bounded scalar/log-HDR spatial or max(spatial,temporal) variance
in v5. These domains are not interchangeable physical units. Zero also means no
statistics evaluated (rejected/frozen/insufficient support); it does not establish
noise-free lighting. Counts are normalized and half-quantized visual values, not
lossless numeric exports. Uncovered captures are all zero. The capture never
modifies color, moments, counts or the acceptance decision, and toggling it keeps
the same per-key history mode. LocalHistory and metric captures use separate
families/images; only one kind is requested per invocation.

Off/unselected keys add no per-frame capture or display work. Previous/in-flight
captures and idle pool entries can remain retained. One selected metric capture
adds 8*S bytes and a receiver display 8*R bytes per unique image, where S/R are
signal/receiver pixel counts: 33.1776 decimal MB together when both are 1080p,
before retained generations/slots/external owners/pool/driver overhead. No permanent
adaptive telemetry image or CPU/GPU readback is added. Existing diagnosticCapture/
diagnosticDisplay timing scopes include these optional paths; capture measures
the decorated temporal pass, not isolated coloring cost. Disable diagnostics and
signature-reference checks for production timing, and do not sum nested timings.

The native adaptive source fixture's optional fifth metrics bool requests both
metric and ordinary adaptive streams for FP16-tolerant color/guide/moment parity,
four-channel displays, missing-capture amber, normalized trust, cold/frozen/thaw
values, mutual-exclusion rejection and toggled-capture chronology. It deliberately
requests multiple views/controls, unlike the one-view renderer; it is not a cost
benchmark. All source/reference checks pass; compilation and native/visual/timing
acceptance remain not_run. Phase 4 source work is complete as
implemented_unverified. Replay/adaptive defaults remain off; P5.1 integration audit
is next, with no release tuning or native quality/performance claim.

## Phase 5 final source handoff

Status: `implemented_unverified`. Source integration and validation protocols are
prepared; C++/shader compilation, Vulkan/Metal pixels, SDK/editor/visual behavior,
measured memory and performance remain not_run. No optimization was selected without
a measured bottleneck. Historical phase handoffs above do not supersede this status.

Defaults remain temporal enabled, maxFrames=32, freezeAtLimit=false;
adaptiveEnabled=false, minCurrentWeight=.03125, reactiveCurrentWeight=.25 and
historyClipSigma=3. Renderer metadata replay remains temporalSurfaceEnabled=false.
Opting in still requires actual interface/format availability; unavailable optional
families fall back to compatible earlier paths. No native evidence supports enabling
replay or adaptation by default or claiming these parameters are tuned.

The temporary freeze override reacts to detected scene instance-transform changes,
including animation and queued programmatic edits, rather than mouse/key activity.
Unrelated UI gestures must not open the rolling window. The model editor's mouse
input and typed numeric transform edit no longer explicitly reset/reattach scenes;
generic scene/material/component edits remain conservative. Light-transform edits
still change lighting signatures. Authored freeze/maxFrames are never rewritten by
motion; each valid per-key history owns its exact 3*maxFrames settling countdown.

Rigid history cannot represent skinning, morphs or custom vertex deformation.
Transparent/refraction composited motion and virtual reflection motion are not
tracked exactly. Static receiver geometry does not describe indirect illumination
or a reflected moving object; finite rolling/adaptation cannot guarantee convergence.
Lighting changes after a saturated freeze without detected motion or a reported
revision/reset may remain stale. In-place resource/custom shader changes absent from
existing revisions require explicit invalidation. Unsupported formats/interfaces,
world-profile v1/v2 and custom v1 kernels retain their documented fallback limits.
Renderer rigid correspondence requires a submitted predecessor; automatic device-loss
recovery is not provided. These limits are independent of passing source checks.

First pending native action, after explicit build/run authorization: compile using
the existing platform setup, record actual backend/device/driver and deployed shaders,
then run `bg2e::draw::tests::runTemporalProbe(engine, shaderRoot)` in an initialized
stopped host and S01/F0 static convergence. Follow the full
[validation protocol](../../../tests/draw/temporal/README.md#p52-reproducible-validation-protocol)
after that baseline; unsupported is not pass. Actual P5.3 acceptance/P5.4 measurements
and any evidence-based optimization remain pending. The plan resume pointer is
[progress.md](../../../plans/temporal_acc_improvements/progress.md).

Native subset update after the Phase 5 source handoff: runTemporalProbe passed on
Metal and Vulkan/MoltenVK; scoped S10/F0 actual AO-temporal assertions passed on
Metal. Vulkan S10 raster assertions passed with its AO capability path unavailable.
Library/paired shaders compiled; broader matrix, SDK and benchmarks remain pending.
See [native evidence](../../../plans/temporal_acc_improvements/validation_2026_10_23/README.md)
for exact scope and commands. Defaults and implemented_unverified status are unchanged.
