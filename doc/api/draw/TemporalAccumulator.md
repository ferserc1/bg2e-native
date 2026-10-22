# TemporalAccumulator and camera motion

`<bg2e/draw/pipeline/TemporalAccumulator.hpp>` and `<bg2e/draw/pipeline/MotionVectorGenerator.hpp>` are included by draw/all.hpp. They provide public draw histories independently of FSR's SDK context and private RendererDeferred targets.

RendererConfiguration::temporal accepts a compatible shared component. A null value selects the default TemporalAccumulator, enabled with maxFrames=32. RendererDeferred::temporal() exposes controls. Initialize an injected component with TemporalConfiguration containing shaderRoot/prefix, draw.temporal.v1 and a maxHistories budget (default 256). The lifecycle, beginFrame, record, invalidate and availability methods are virtual. Unsupported formats or unavailable kernels produce a raw signal pass-through; no history images are allocated in that path. Histories do not require ray-tracing support.

```cpp
renderer.temporal()->setMaxFrames(64);
renderer.temporal()->setFreezeAtLimit(false);
renderer.invalidateHistories(); // At a synchronized application mutation boundary.
```

The renderer accumulates active AO, GI and reflection signals before lighting. Histories are keyed by signal kind, view and layer, independently of frame slots. Opaque layer zero and each transparent submesh have separate identities. The frame sequence is actual sceneFrameNumber; UI-only presentations, picks, overlays and paused retained scene images do not submit temporal samples. Presentation serial identifies an acquired, completed resource slot only.

Standalone clients call initialize(device,backend,slots), beginFrame(slot,serial,sceneFrameNumber,sceneMotion=false) once for the retired slot, then record(TemporalInput) once per effect/view/layer. Submit that recording in sequence on the same Device queue before recording a dependent successor. Sequence numbers must be nondecreasing within a lifecycle; gaps for a key restart its convergence. Discarded/failed submission requires invalidate before further use, because recording publishes provisional chronology. Input is borrowed: sampled R16 visibility or RGBA16 radiance, SurfaceInputs, TemporalView with current unjittered/jittered matrices, external scene/reset generation and optional reflection roughness/threshold. CameraDepth and explicit world profiles reuse the public effect surface contract. Explicit stationary surfaces can set view.reproject=false; UV bake accumulation is still a distinct M04 contract.

TemporalResult::image is the sampled result. owner retains an immutable accumulated image; on disabled/unavailable pass-through it is empty and image borrows the original signal. Keep borrowed input images alive through completion. Histories own separate world position/coverage and signed normal/per-pixel count images at signal resolution. Surface and signal extents may differ; changes reset correspondence. Each submitted invocation retains both history versions, descriptors and uniforms in the acquired slot; the next frame never overwrites a live image. Returned owners may survive invalidate/cleanup but must be released before destroying the Device. Cleanup drains; logical invalidate retains submitted owners without waiting per frame.

Camera reprojection maps current reconstructed world positions into the previous jittered clip space. Prior position and world-normal tests reject disocclusion. Thresholds are absolute world distance plus relative camera distance, and a normal cosine. The previous/current unjittered matrices remain available through TemporalView and the separate MotionVectorGenerator camera reference. Its motion convention is previous-minus-current framebuffer pixels with negative clip Y; jitter alone produces no unjittered camera motion. It does not generate moving-object vectors or replace the existing FSR motion texture generator.

Visibility averages scalar [0,1] and returns one for uncovered pixels. GI averages valid RGB with alpha one; invalid GI clears the pixel history and preserves IBL fallback. Reflection RGB is accumulated weighted by fractional confidence, while confidence is averaged separately; output remains unpremultiplied. Eligible zero-confidence misses participate, but roughness-rejected/uncovered pixels cannot revive older reflections. Valid black GI/hits remain distinct from invalid signals.

maxFrames is 1..1024. Before the bound, accepted per-pixel samples use an incremental arithmetic mean. At the bound, freezeAtLimit=false uses rolling weight 1/maxFrames; true retains a converged accepted pixel until rejection/reset, except during the temporary motion window. A motion record permits rolling immediately and opens exactly 3*maxFrames subsequent consecutive static records for that signal/view/layer. Repeated motion renews the window. The last tail record still rolls; the following record can freeze. The countdown belongs to recorded history, so presentation-only work and beginFrame without record cannot decrement it. Invalid, skipped, reset or abandoned predecessors contribute no countdown. Authored freezeAtLimit and maxFrames remain unchanged. frameCount(key) and TemporalResult::frameCount report a bounded nominal consecutive-view count, not proof that every pixel converged: reprojection rejection restarts individual pixel counts. Later headless jobs must select actual rendered samples explicitly. setEnabled, frame bound/freeze and rejection controls invalidate histories.

Renderer::invalidateHistories() is the generic application hook; RendererDeferred resets effect history generation and the active upscaler. resetUpscaler() remains compatible and now invokes that full hook. Scene attachment, resize, explicit camera changes, inspector changes and environment replacement reset it. An exact extraction/settings signature also detects mesh/material/resource identity, transforms, lights, camera identity/projection, effect quality/mode/noise and temporal/shadow settings. Node/submesh identity must be unique when temporal correspondence is active. Transform-only movement preserves global histories and FSR and supplies sceneMotion to the accumulator; full fingerprints remain transform-sensitive. Optional rigid metadata generation and v3 history transport are described below; native validation remains pending. Moving surfaces and changing indirect lighting/reflections on static surfaces can trail, as can FSR's camera-only reconstruction. The finite update window is approximate and does not guarantee convergence. Notify invalidateHistories for camera teleports on the same scene camera, in-place image uploads and custom shader/resource edits that retain identity. Applications remain responsible for safe mutation boundaries and requested redraws, including partial editing failures. Generic material utilities acquire no renderer ownership.

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

Renderer selection uses opaque metadata only for layer zero and an exact map lookup
for each transparent layer, never substituting opaque metadata for a missing layer.
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
required before integer previous-pixel conversion. Nearest previous token must
match the current nonzero token, and current metadata depth must agree with the
actual current receiver depth (existing D16/D32 tolerances).

Previous history position is compared to expected previous position, and previous
history normal to normalized expected previous normal. The exact position tolerance
in both shaders and the CPU oracle is:
`absolute + relative * length((previousView * vec4(expectedPreviousPosition,1)).xyz)`.
Expected and previous geometry, sample/count and tolerance must be finite. Previous
position coverage and normal/count validity remain required. Current coverage is
independent of correspondence validity: rejected correspondence restarts the raw
sample and stores current world guides for the next scene. Scalar/HDR averaging,
GI validity, reflection confidence, nominal bounded counts and Phase 0 rolling/
freeze countdown are unchanged. No reflected/deformed surface support is added.

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
source/math checks; next implementation step is P3.1, not native acceptance.
