# Temporal receiver diagnostics and probes

These fixtures are source only and have not been compiled or run on Vulkan or
Metal. Run only after explicit build/runtime authorization, in a stopped initialized
Engine with the paired native shader tree. No CMake changes or test auto-registration
are required by this change.

## Diagnostic controls

The existing renderer settings catalog exposes:

- `temporalSurfaceEnabled`: explicit experimental replay opt-in (still defaults off).
- `temporalDiagnosticView`: Off, Surface tokens, Local history, Motion, Expected previous position, Expected previous normal.
- `temporalDiagnosticSignal`: Visibility, Diffuse radiance, Reflected radiance.
- `temporalDiagnosticLayer`: decimal text in the settings UI (reflection has no
  uint64 scalar); zero for opaque, otherwise the exact transparent
  `(uint64_t(objectId)<<32)|submeshIndex` layer from the current queue.
- `temporalDiagnosticsAvailable`: read-only token/acceptance kernel availability.
- `temporalSurfaceDiagnosticsAvailable`: read-only motion/previous-geometry display availability.

Choose Final or Raw HDR in the existing final-pass display settings. Raw HDR gives
unprocessed diagnostic colors. Token colors are a display hash, not a lossless token
encoding; zero tokens are black. Local history is **red for rejection/first sample,
green for accepted history, black for uncovered pixels, amber for no captured
acceptance in the selected current key** (for example disabled AO in an IBL graph).
The decision comes from the accumulation kernel, including maxFrames=1; it is not
inferred from the nominal per-key or per-pixel sample count. It does not distinguish
the individual rejection reasons. At lower signal resolution, each decision covers
the receiver pixels mapping to that signal texel.

Controls latch at scene update. Selecting a view does not implicitly enable replay
or effects. A missing/skipped layer or unsupported optional kernel leaves ordinary
scene output in place. Diagnostics bypass the temporal upscaler and use spatial
reconstruction; subsequent ordinary frames retain the upscaler's existing chronology
checks. Core accumulation continues unchanged. No ordinary-frame CPU readback,
submission or device wait is added. Queued settings imports preserve their existing
generic invalidation behavior; direct diagnostic setters do not reset histories.

## Fixtures

1. `runTemporalCorrespondenceProbe()` in `correspondence_probe.hpp` is an internal
   source fixture linked with the engine's private TemporalCorrespondence
   implementation (not an installed/public-header test). It fabricates immutable
   signatures and incomplete submission tickets. It checks insertion/extraction-ID
   shifts, two submeshes, transform-only token preservation, previous recorded world
   rather than prepared world, a submitted incomplete predecessor without waiting,
   abandoned recording, immutable earlier snapshots, reset without token recycling,
   material replacement and removal/reinsertion. Its completion ticket is a CPU
   model, not evidence of native GPU overlap.
2. `runTemporalSurfaceProbe(engine, shaderRoot, &report)` in `probe.hpp` uses actual
   RendererDeferred raster replay plus a separate scalar metadata accumulator.
   `false` means unsupported, never pass; `report` preserves initialization reasons.
   The fixture queries native format/kernel support through initialization. The
   first two commands use distinct renderer/consumer slots and are submitted in order
   before any fixture wait/readback (the GPU may finish naturally; no overlap claim).
   Later slot reuse follows explicit completion. All waits/readbacks are fixture-only.
3. Also run `runTemporalProbe`, `runTemporalMetadataProbe` and
   `runTemporalInvalidationProbe` from the existing test directories. Keep existing
   material/light/projection/membership reset expectations intact. The metadata
   fixture additionally checks exact diagnostic colors, maxFrames=1 acceptance,
   malformed pairs, odd extents, zero/local token rejection and out-of-bounds history.

The native surface fixture checks a stationary wall behind translated foreground,
nearby parallel surfaces with equal normals and depth separation below the old
position threshold, unchanged neighbors and newly revealed wall acceptance decisions,
two rasterized submeshes on one node, retained earlier pairs/diagnostic images after
slot reuse, and an abandoned scene. Its scalar signal and encoded normals are
synthetic; receiver depth and tokens are from the actual raster pass.

A nearest alpha texture creates holes. The fixture compares token coverage against
the actual GBuffer depth displayed through FinalLayer, pixel by pixel. Transparent
submeshes get independent pairs, full clears outside coverage, preserved opaque
metadata and private-depth occlusion behind the opaque wall. A derived/custom GBuffer
is explicitly unavailable. Disabling metadata leaves the Phase 0 renderer path.
The native fixture also inserts a culled object before existing extraction items,
checks changed object IDs and stable wall identity after warmup, and requires the
existing global membership reset. The CPU fixture checks identity without conflating
that reset policy with token allocation.

## Native acceptance record

For each authorized run record backend, device, shader deployment, support/fallback
status, exact assertions and visual observations in progress.md. Required formats:
R32_UINT Sampled|ColorAttachment, D32_SFLOAT private Sampled|DepthStencil|TransferDst,
R16_SFLOAT scalar history and RGBA16/RGBA32 sampled storage history, plus RGBA16
storage diagnostics. The fixture does not establish D16 support or arbitrary devices.
Currently all native formats and all runtime/visual/performance scenarios are not_run.

Separately inspect a converged wall/cube scene with real AO/GI/reflections, replay
both enabled and disabled, freeze enabled/disabled, actual application gizmos,
alpha UV0/UV1 and scaled UVs, equal-depth overlap order, skipped transparent regions,
resize, and custom fallback. Require stable wall history, local restart of newly
revealed pixels and moving receiver restart without a global transform reset.
Stable identity does not validate indirect-light changes on a stationary receiver.
No object-motion reprojection or indirect-light correctness is claimed by Phase 1.

## Recorded camera coordinates (P2.1)

Views are right handed and engine clip depth is zero to one. Normalized framebuffer
coordinates are `(0.5 * clip.x / clip.w + 0.5, 0.5 - 0.5 * clip.y / clip.w)`;
framebuffer Y opposes clip Y (negative-height Vulkan viewport, matching Metal).
Motion is **previous minus current**, in pixels at the internal receiver extent,
using unjittered projections: `(previousNdc.xy - currentNdc.xy) * (0.5, -0.5) * extent`.
Raster coverage uses the current jittered projection. Previous history lookup will
use the previous jittered projection; jitter must not become object motion.

The private immutable correspondence snapshot stores view, unjittered projection,
jittered projection and internal extent for current and previous scene recordings,
as well as captured current/previous instance world matrices. Previous state must
belong to the consecutive scene sequence in the same history generation and an
already submitted recording in the renderer's ordered graphics stream. It need not
be CPU-complete. The snapshot retains both recording completion owners without
retaining a recursive chain of frames. Updates, presentation-only work, reused
slots and fingerprint queries do not supply previous camera state.

First use, gaps, history invalidation/cuts, changed projection/extent, invalid camera
matrices and unsubmitted/abandoned predecessors invalidate previous camera state.
Nonfinite, singular or non-affine instance transforms invalidate only that receiver;
geometry/material/token compatibility remains required. Reflected and nonuniform
scale matrices are captured when invertible, but previous-normal raster support and
native acceptance remain pending. Explicit camera cuts/teleports must call
`invalidateHistories()` (and reset upscaler history as required by its API).

P2.1 only captures and validates state. Moving receivers still reject under the
Phase 1 policy; motion attachments, enhanced reprojection and FSR integration are
P2.2–P2.4 work. Existing camera rejection/reset behavior remains unchanged.

P2.1 source fixture: `runTemporalCorrespondenceProbe()` now also checks distinct
jittered cameras at odd 101x73 internal extent, immutable retained camera state,
incomplete submitted predecessor ownership without waits, unsubmitted/abandoned
recordings, gaps, nonfinite cameras and local singular-instance rejection.
Compile/run this internal fixture with TemporalCorrespondence.cpp only when a
native build is authorized. Vulkan/Metal execution and pixel motion checks remain
not_run; these assertions are source fixtures, not executed results.

## Rigid metadata generation (P2.2)

`TemporalSurfacePass` now uses the explicitly versioned `surface_v2` shader family
and `draw.temporal_surface.v2` layout. The original `surface` shaders remain
unchanged; a custom shader root must supply the v2 family or initialization reports
unavailable. The GBuffer and its six colors/depth index are unchanged.

The dedicated 288-byte std140/Metal uniform contains uint4 identity at byte 0,
previous model at 16, current unjittered view-projection at 80, previous unjittered
view-projection at 144, current-to-previous normal transform at 208 and float4
internal extent at 272. C++ and Metal assert size/offsets; GLSL specifies offsets.
The same allocation is bound at Vulkan set 2 bindings 0 (vertex) and 1 (fragment),
Metal vertex buffer 9 and fragment buffer 8. Distinct Metal indices are required
because ResourceSet identifies uniform entries by buffer index, across stages.
Frame/object bindings are unchanged. Normal texture/sampler reuse the GBuffer
set 1 bindings 5/11, Metal texture/sampler 3, with the exact material UV/decode path.

Outputs are token R32_UINT, motion RG16F, previousPosition RGBA32F and previousNormal
RGBA16F plus private depth. All colors require sampled/color-attachment support,
four simultaneous attachments and the backend-reported aggregate attachment bytes.
A zero aggregate limit means no additional limit in the existing GPU contract.
No new GPU capability API or implicit hardware-support claim is introduced.
All outputs clear over the full receiver extent and retain independent pooled owners
for opaque and each transparent invocation. Transparent private depth seeding and
coverage remain unchanged. Retain all consumed image owners through completion.

`TemporalSurfaceInstance` adds optional trailing previousWorld/hasPreviousWorld;
legacy aggregate callers without them mean a static current model. `record` adds
an optional trailing `TemporalSurfaceCamera` containing the two unjittered
view-projections (identity defaults require an identity camera). RendererDeferred
supplies the immutable P2.1 submitted camera/model snapshots explicitly.
Raster position still uses the current jittered frame binding. Perspective-correct
varyings carry both unjittered clips and previous world position; fragment code
performs the divides and scales previous-current NDC by `(0.5,-0.5)*internalExtent`.
Normals reproduce GBuffer mapping before applying
`transpose(inverse(previousModel)) * transpose(currentModel)` to the current world
normal, equivalent in xyz to `normalMatrix_prev * inverse(normalMatrix_now)`.
This preserves the same-local-shading-normal approximation under affine scale;
nonuniform-scale mapped-normal visual acceptance remains unverified.

Missing chronology, incompatible resources and invalid current/previous matrices
reject locally. Singular/non-affine/nonfinite models and reflected (negative
determinant) models are conservatively unsupported. Invalid clip w, nonfinite
fragment values, degenerate normals and motion exceeding RG16F finite range
emit token zero and position/normal w=0 while retaining receiver depth coverage.
Valid moving receivers keep their canonical nonzero token.

P2.2 generates metadata only. It does not transport accumulator/FSR history.
Until P2.3, `TemporalSurfaceResult::legacyCompatible` is false for a domain containing
valid moving receivers; RendererDeferred supplies that domain to the unchanged v1
accumulator path instead of letting the P1 token-only consumer misinterpret motion.
Static domains retain P1 metadata consumption. No global motion reset is added;
continuous fallback uses Phase 0 rolling history. Returning to enhanced P1 input
requires its existing missing-predecessor-metadata rejection. Token diagnostics
remain available; actual acceptance diagnostics show unavailable when v1 is used.
This temporary compatibility gate must be removed/replaced by P2.3 correspondence
validation. Public token/depth metadata access remains a P1 pair; callers must not
interpret it as a complete object-reprojection contract.

Static verification passed; compilation, shader compilation, Vulkan/Metal execution,
format support, visual quality and performance remain not_run. Native follow-up:
run the surface fixture and `runRigidSurfaceReferenceProbe`, compare the new images
against `rigidSurfaceReference` for translations/rotations/scales and perspective
triangles, inspect motion sign/jitter independence and normal maps, then exercise
invalid transforms and transparent/opaque retained owners across queued slots.

The native raster fixture now expects moving foreground to keep nonzero tokens.
Its explicit P1 test consumer still tests only local token/position rejection;
it is not an object-history transport acceptance test. The new `rigid_probe.hpp`
provides an independent current-world-to-previous-world oracle, with odd-extent
translation sign/depth, rotation/nonuniform normal recovery and stationary motion
assertions. C++ assertions are source fixtures and have not been executed.

## P2.3 update

The public synchronized metadata snapshot now retains previousPosition/previousNormal
and the legacyCompatible flag as well as token/depth. Renderer-owned accumulators
consume the pair using optional v3. The earlier P2.2 blanket moving-domain fallback
is replaced for v3-capable camera-depth consumers; injected legacy consumers still
fall back safely. The existing raster fixture constructs an explicit token-only pair
for its synthetic stationary v2 consumer, preserving that probe's original scope.
Use `../temporal/reprojection_probe.hpp` for v3 transport source assertions and
`rigidPositionTolerance` in rigid_probe.hpp for the exact previous-camera-space
threshold oracle. Neither fixture has been compiled or executed in this session.
Real-raster v3 acceptance, perspective/jitter, queued slots and normal-map/HDR visual
checks remain pending for an authorized native validation session.


## P2.5 source acceptance

Phase 2 is implemented_unverified. Run the repository-only source check without a
native build: `python3 tests/draw/temporal_surface/phase2_source_checks.py`.
It passed along with git diff --check; it executes source assertions and independent
standard-library arithmetic only. It does not run C++ static_asserts, the CPU C++
oracles, native shaders, formats, SDK dispatch or visual/performance checks.

The expanded runRigidSurfaceReferenceProbe covers RH zero-to-one perspective,
stationary jitter, production camera-only parity, pixel translation, rotating planes,
uniform/nonuniform scales, previous view-axis geometry, simultaneous camera/object
motion, post-interpolation divides, odd/reduced pixel mapping and invalid/singular/
non-affine/reflected models. runTemporalCorrespondenceChronologyProbe models two/
three slot rings with incomplete submitted tickets, immutable retained state,
prepared-only ghost updates, new/revised geometry, cuts, gaps and abandoned state.
This CPU model is not native overlap evidence. Both functions remain uncompiled/unrun.

The new optional surface display shader shares the old display resource ABI but is
initialized independently. Motion xy maps 0.5+xy/64 (B=.5); expected previous position
uses 0.5+0.5*xyz/(1+abs(xyz)), normal uses 0.5+0.5*xyz. Invalid/zero-token/nonfinite
samples are black, and geometry requires valid w. Source data is retained by each
active display invocation; inactive accumulation histories drop the unused motion
owner. Colors are bounded visual encodings. Missing selected data is unavailable,
never valid zero motion. Original token/actual-acceptance display is unchanged.

Use runQueuedTemporalReprojectionProbe from ../temporal/queued_reprojection_probe.hpp
for actual native two/three ordered-slot assertions when authorized. It is a source
fixture with synthetic receiver guides, not a real-raster transport pass. The
existing raster fixture remains explicitly a v2 synthetic signal/normal consumer;
its invalid abandonment call through a const completion owner is corrected to
release the unsubmitted command wrapper. Full raster v3, deployed normal maps and
actual AO/GI/reflection/FSR visual comparisons remain pending. See the Phase 2 plan
P2.5 table for exact remaining native cases. Replay stays default off. Next: P3.1.
