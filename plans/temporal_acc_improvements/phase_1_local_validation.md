# Phase 1: stable surface identity and local rejection

Dependency: Phase 0 implemented. Entry: P1.1. Exit: P2.1. Preserve the Phase 0 fallback and thaw behavior.

## Contract

On the supported built-in raster path, identify each visible receiver with a stable integer token. During this phase, moving receivers deliberately reject their own history; stationary matching receivers can retain theirs. Discovered background rejects the foreground's old token. Do not yet transport samples with object motion, remove the camera threshold, or relax material/light/membership resets.

Use a new draw-level `pipeline::TemporalSurfacePass` and renderer-owned correspondence state. Do not add attachments to the G-buffer, change ObjectData's public/shader layout, or introduce GPU abstraction dependencies on draw/scene.

## P1.1 — establish persistent instance records

1. Create renderer-private correspondence state keyed by `(nodeId, submeshIndex)` with retained geometry/material snapshot references, nonzero surface token, previous recorded world matrix, and current change flags.
2. Use an incrementing uint32 token allocator. Reserve 0, never reuse removed tokens, diagnose exhaustion without wrapping. Reset the allocator only with a renderer lifecycle/history reset that discards all old correspondence. Record exact reset location.
3. Reuse TemporalSignatureState's captured effective material/texture and geometry components rather than comparing only authoring wrappers. A rigid transform changes the motion flag but not the token. Effective material/mesh/classification change replaces the token. Global resets for these edits remain in force in this phase.
4. Build immutable per-frame records. Full fingerprint/cache inspection cannot publish or consume these records. Separate a prepared update snapshot from a snapshot attached to recorded scene work. Consult recordingCompletion and its wasSubmitted state before treating prior records as a valid predecessor. If chronology is unknown, mark that instance's correspondence invalid locally.
5. Do not use the current frame slot's previous occupant as the previous scene. UI-only frames and repeated preparation are not motion steps. Missing or duplicate stable identities keep the existing explicit diagnostic when correspondence is required.

### P1.1 implemented source boundary

Renderer-private `TemporalCorrespondence.{hpp,cpp}` owns the token registry and
immutable prepared/recorded frame values. RendererDeferred prepares from captured
TemporalSignatureState components in update, resolves previous matrices at the
start of recordLinear, and publishes only after successful linear recording. The
recorded value retains that command's recordingCompletion. Only a submitted,
consecutive, same-generation predecessor is eligible; it need not be CPU-complete.
Unknown chronology gives previousValid=false/correspondenceValid=false locally.

Tokens ignore extraction objectId and world transform. Geometry identity compares
retained mesh/BLAS/RT inclusion; material identity compares captured source identity,
effective packed values and texture/sampler bytes. Removed or ambiguous keys are
dropped without recycling tokens. Existing signature diagnostics still reject
empty/duplicate keys when temporal correspondence is required; otherwise these keys
receive no record/token and identitiesValid is false, preserving Phase 0 admission.

The allocator uses a uint64 CPU cursor checked before conversion to uint32. Zero is
reserved; UINT32_MAX+1 is an exhausted sentinel and allocation throws without
wrapping. The cursor is reset only when RendererDeferred::cleanup destroys
_temporalCorrespondence, after its existing device drain and _targets.clear have
discarded all retained per-slot correspondence values. resetHistories only clears
the recorded predecessor; it does not reset the token cursor or persistent registry.
Recovery from exhaustion requires that drained cleanup and a fresh renderer lifecycle.
No ordinary-frame drain or automatic rollover was introduced.

newInstance/geometryChanged/materialChanged preserve identity observations from
preparation even if a global policy reset removed the predecessor. Recording also
checks the previous captured resources and token. previousWorld is meaningful only
when previousValid is true; otherwise it equals current world as an invalid
placeholder. A moving matching instance keeps its canonical token and valid previous
matrix, but correspondenceValid is false for Phase 1's static-only rule. None of
these CPU flags participates in temporal rejection yet. P1.3 maps recorded values
to the P1.2 token/static-valid uniform when experimental replay is requested;
temporal consumption (P1.4) is now implemented in source. Native/source probe work and
visual acceptance are tracked separately in progress.md.

## P1.2 — implement a token metadata raster pass

1. Add `lib/include/bg2e/draw/pipeline/TemporalSurfacePass.hpp`, its .cpp under lib/src, and paired `shaders/src/draw/temporal_surface/surface.vert.{glsl,metal}` and `surface.frag.{glsl,metal}`. Copy full GPL headers. Follow existing shader discovery/name conventions; no CMake changes.
2. Start with one `R32_UINT` color image (sampled + color attachment), cleared to integer zero, and a private sampled depth image matching receiver dimensions/depth convention. Check format/usage support before initialization. No blending on the integer output.
3. Reproduce the built-in GBuffer vertex positions, culling, depth comparison/order, material UV selection, albedo alpha multiplication, cutoff and discard rules. Reuse material bindings and helpers. Add dedicated per-instance temporal uniforms/bindings for token and correspondence-valid flag rather than altering shared ObjectData. Rendering coverage, including alpha-tested holes, must match GBuffer.
4. Use private depth and repeat the same receiver draw order. Do not rely on depth-equal replay of an existing depth image: equal-depth overlapping surfaces can disagree about which instance owns a pixel. Clear and write private depth using the same rules as the original receiver pass. The temporal consumer must also reject metadata whose sampled depth disagrees with its actual surface depth.
5. Current output token is zero for an instance with no valid static correspondence in this phase (new, moved, singular/unsupported). Stable instances output their nonzero token. Recovered stationary instances can begin new history on their first valid frame. Store canonical token in the CPU record even when output is zero.
6. Enable only when the renderer uses the exact supported built-in GBuffer type and supported surface conventions. Custom/deformed receiver shaders get no metadata and retain Phase 0 behavior, with an explicit status. Do not silently claim static correspondence for unknown deformation.

### P1.2 implemented source boundary

`pipeline::TemporalSurfacePass` is a separately initialized optional pass, exported
by draw/all.hpp. P1.3 now initializes it in RendererDeferred and records it only
with explicit experimental opt-in (default off). Initialization
accepts the current receiver RenderLayer, admits only exact type GBuffer with its
canonical seven-output contract, and requires the original paired built-in shader
conventions. Subclasses/custom contracts return an explicit unavailable status.
A caller must not present a modified/deformed shader deployment as the built-in
contract. No capability-dependent renderer default is enabled by this step.

Initialization checks one color attachment, nonzero 2D image limits, the token
format's sampled/color usages, the selected receiver depth format's sampled/depth/
transfer-destination usages, and the aggregate color-byte limit where present.
Missing kernels/layout/pipeline support is latched as unavailable until cleanup and
explicit reinitialization. No actual Vulkan/Metal capability query was executed by
the agent; all native status/format results remain not_run.

Call beginFrame(slot,serial) only for a retired acquired slot. record takes its
FrameContext, RasterFrame, internal receiver extent and current extraction-ID/token/
static-correspondence tuples. IDs select current object bindings only; canonical
tokens still come from persistent correspondence. Missing/invalid static matches
emit zero but still rasterize/depth-write, preserving occlusion. Malformed supplied
IDs/valid zero tokens reject. Vertex sources copy the built-in GBuffer exactly
apart from the Metal include path. Fragment UV selection, implicit texture sampling,
sRGB decode helper, authored albedo-alpha multiplication and cutoff/transparent
zero-alpha discard match GBuffer. Back culling and LessOrEqual depth write/test
match the original draw order; token blending is disabled.

Frame/object sets 0/1 retain drawBindingLayout's exact compatible prefix; dedicated
set 2/binding 0 (Metal fragment buffer 8) carries one 16-byte uint4: token, valid flag,
and two reserved zeros. ObjectData and the shared material/binding helpers are
unchanged. GLSL outputs uint at color location 0; Metal outputs uint at color(0).
There is no float/half token encoding.

Each invocation obtains a new retained/pool-leased R32_UINT token image (sampled +
color attachment) and matching-format private depth (sampled + depth attachment +
transfer destination). Opaque replay clears token to unsigned zero and private
depth to one. A single transparent replay accepts preserved sampled/transfer-source
opaque depth, copies it to the private depth, clears token over the full extent and
draws that item with LOAD depth. P1.3 now uses this API for independently retained
receiver domains. Regional scissors are deliberately ignored.
No depth-equal replay of another receiver target is used.

Per-slot invocation owners retain drawn resource snapshots, temporal uniform/sets
and both output images until slot retirement. Returned shared image owners may
remain in histories and prevent pool overwrite/reuse; frame/object binding rings
remain the caller's responsibility through GPU completion. Every view/layer is
recorded at most once per acquired serial. cleanup uses a lifecycle drain; ordinary
recording adds no CPU wait, immediate submission or readback. P1.4 consumers
validate metadata depth against their actual receiver depth. Native
coverage/pixel/lifetime probes are pending P1.5 and explicit build/run authorization.

## P1.3 — integrate opaque and transparent receiver domains

1. Generate opaque metadata after opaque GBuffer recording and before opaque temporal consumers. Match internal receiver resolution, not output resolution or effect quality resolution.
2. The renderer rewrites GBuffer/depth for each transparent submesh. Generate metadata matching that same current receiver domain before its temporal consumers. Repeat the original semantics: private depth begins from the preserved opaque depth for the single transparent receiver, and only that transparent item is drawn. Clear metadata color outside its covered region to invalid.
3. Do not let opaque metadata describe a transparent receiver. Generate full receiver extent first; do not optimize to the transparent scissor until coverage/clearing semantics have native evidence. A layer skipped by existing region logic produces no temporal record and must not advance its history.
4. Preserve opaque metadata separately for FSR's future use; never pass the last transparent layer's data as opaque data.
5. Retain immutable token/depth image owners for every history that needs them. Do not overwrite a slot image still referenced as previous history. Use the existing TemporaryResourcePool lifetime pattern or equivalent retained images, scoped by view/layer and recording ownership. Current metadata may be shared by signals of the same receiver invocation; retain it before recording commands.

### P1.3 implemented source boundary

RendererDeferred initializes one TemporalSurfacePass after its actual replacement
layers and frame/object bindings are initialized. The pass's existing exact-type,
format, shader and combined-limit gates determine availability. Unsupported/custom
receivers keep Phase 0; an explicit request that was set before initialization also
records the unavailable reason in initializationWarnings. No ordinary retry occurs.

`setTemporalSurfaceEnabled(true)` is an experimental opt-in at a stopped update
boundary. It defaults to false because native validation has not occurred and the
v2 consumer has no native acceptance yet. The existing settings catalog exposes the
writable Diagnostics property temporalSurfaceEnabled and read-only Capabilities
properties temporalSurfaceAvailable/temporalSurfaceStatus. The setter itself never
resets histories. Queued settings edits still obey the existing generic settings
reset contract. Release/default tuning is deferred to Phase 5.

Each acquired scene update releases that slot's previous metadata owners, retires
pass invocations even when replay was disabled, and latches requested/available
state for the whole scene frame. Missing/duplicate persistent identities, when
admitted by the inactive Phase 0 correspondence policy, disable metadata for that
scene with an explicit fallback status; active correspondence keeps its original
signature diagnostics. UI-only frames do not call metadata beginFrame or record.

recordLinear resolves its P1.1 submitted predecessor first, then maps captured
node/submesh records to current objectId/token/static-valid tuples. objectId selects
current raster bindings only. Opaque metadata is generated immediately after opaque
GBuffer, at internal receiver extent, before composition/temporal effects. The
original GBuffer depth is separately preserved as before.

For each transparent/solid-transparent item in the existing far-to-near order,
regional/inspection skip decisions run first. After that item's GBuffer rewrite,
metadata replays the single item at full extent, with private depth seeded from the
preserved original opaque depth and token cleared outside coverage. No transparent
history/metadata record is made for a skipped layer. Existing processing regions
still apply to the original passes; metadata never inherits their scissor.

The slot owns opaqueTemporalSurface independently and stores each processed
transparent pair in transparentTemporalSurfaces keyed by the same layer value used
by composite/TemporalKey (view 0). These strong image owners and the pass's retained
invocations protect immutable outputs; no transparent recording replaces opaque
metadata. P1.4 copies the matching pair into any history that consumes it and never
selects the opaque pair for a transparent fallback. Opaque metadata remains retained
for future FSR integration, but current UpscalerInput/motion generation is unchanged.

Metadata GPU scopes use the existing renderer.temporalSurface timing facility,
scoped by layer, without synchronous readback. Resize uses the renderer's existing
drain and target replacement; the pass allocates pooled images matching each new
receiver extent without reinitialization. cleanup drains/releases the metadata pass
before frame/object binding teardown. No ordinary-frame wait/submission/readback,
G-buffer attachment/layout change or temporal v1 input change was introduced.
The token pass and P1.4 consumer are integrated in source; native pixel acceptance
remains unverified. Native formats/coverage,
transparency, retained-owner and fallback scenarios remain not_run on both backends.

## P1.4 — extend temporal inputs and rejection

1. Introduce optional trailing metadata inputs/owners and a versioned enhanced shader interface, e.g. `draw.temporal.v2`. Keep `draw.temporal.v1` available for custom/legacy kernels. Metadata absent means the Phase 0 path, not an exception or global reset. Built-in v2 kernels support both metadata-valid and fallback invocations through explicit flags and safe dummy bindings.
2. Validate device, extent, type, integer format and sampled usage; reject supplied malformed inputs. An unsupported optional feature selects fallback at initialization rather than retrying every frame.
3. For enhanced histories retain previous token metadata. At the reprojected nearest sample require nonzero equal tokens and current metadata depth matching the current receiver. Keep existing world position/normal/camera tests; transform motion remains locally rejected in Phase 1.
4. Signal pixels and metadata pixels can have different extents. Convert each sample through normalized framebuffer UV, using texel centers, clamping only for current sampling. Out-of-bounds previous UV must reject, not clamp onto the screen edge. Add odd-size and half-resolution examples to the probe.
5. Preserve existing reflection confidence weighting, GI valid-black handling, coverage behavior and per-key counts. Do not mistake TemporalResult::frameCount for per-pixel convergence.


### P1.4 source implementation (implemented_unverified)

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

## P1.5 — probes, diagnostics and acceptance

Add token and local-rejection diagnostic views through existing draw/UI wrappers, with no imgui types in public headers. Do not add per-frame CPU readbacks. Source-review CPU/GLSL/Metal layouts and integer binding types explicitly.

Extend native probe sources with: translated foreground exposing a wall; nearby parallel surfaces with equal normals; alpha-cutout holes; unchanged neighbors; object insertion changing extraction IDs; two submeshes on one node; two frames in flight; abandoned scene recording; retained earlier metadata; transparent receiver over opaque depth; custom GBuffer fallback. Keep global material/light reset assertions unchanged.

Native acceptance, when authorized: static wall token and history survive moving foreground; newly revealed pixels restart; moving object's own pixels are allowed to restart every frame. Phase 0 no-global-motion-reset acceptance must pass with metadata intentionally disabled/unavailable too.

Record all formats supported/unsupported, paired backend results, source checks and unrun scenarios in progress.md. Do not claim identity tests solve indirect-light changes on a stationary receiver. Next: P2.1.

## Cost and fallback

One extra geometry raster pass per processed receiver domain, integer token storage and private depth, plus token lookups. Many transparent layers may dominate this cost. CPU storage is proportional to instances and retained frame snapshots. Estimate actual high-water images including histories; one R32_UINT image at 1080p is 8.29 MB. No new RT rays or specialized hardware. Unavailable metadata uses Phase 0 unchanged.

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
indirect-light changes on stationary receivers. Next source step: P2.1.
