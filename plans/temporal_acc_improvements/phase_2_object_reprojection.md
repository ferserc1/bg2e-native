# Phase 2: rigid-object motion and previous-surface reprojection

Dependency: Phase 1 implemented. Entry: P2.1. Exit: P3.1.

## Contract and fixed representation

Support opaque and individually processed transparent receivers with static vertex geometry transformed by invertible affine matrices. Deformations, skinning, morphing and unknown custom vertex shaders are unsupported and fall back to Phase 0 or local rejection. Do not add deformation support implicitly.

Extend the separate metadata pass to four color outputs, subject to capability checks:

| Output | Format | Meaning |
|---|---|---|
| token | R32_UINT | Canonical stable surface token; zero if correspondence invalid |
| motion | R16G16_SFLOAT | Previous minus current unjittered framebuffer pixels, internal resolution |
| previousPosition | R32G32B32A32_SFLOAT | Expected previous world position xyz; w=1 valid, 0 invalid |
| previousNormal | R16G16B16A16_SFLOAT | Expected previous world shading normal xyz; w=1 valid |

Keep private depth from Phase 1. Four attachments do not guarantee device support: check per-format usages, attachment count and combined storage limits exposed by the current API. If the existing API cannot express a required capability, document it rather than assuming support; use feature initialization failure/status fallback without modifying GPU architecture unnecessarily. A missing feature must not restore global resets on object transforms.

## P2.1 — lock coordinates and frame chronology

1. Document conventions in the API and probe: RH view, engine zero-to-one clip depth, normalized framebuffer Y opposing clip Y, motion sign previous-current, pixel units at internal extent, unjittered motion.
2. Capture current/previous unjittered and jittered camera matrices plus previous per-instance matrices in immutable scene recording state. Previous means the valid preceding rendered scene submission, not previous presentation/update/slot.
3. A submitted predecessor may be consumed before CPU completion in the existing ordered stream. An unsubmitted/abandoned predecessor from a different recording must reject correspondence. Store completion owners with snapshot data; never use a mutable live scene node as history.
4. On scene gap, cut, first use, singular/non-affine transform, invalid numbers or incompatible geometry, mark previous-surface metadata invalid locally. Keep normal explicit/lifecycle global reset behavior. Do not introduce per-frame GPU idle.

## P2.2 — generate object motion and expected previous geometry

1. Extend only the temporal pass's dedicated uniform records with previous model and camera transforms, current-to-previous normal transform, valid flag and canonical token. Match CPU alignments, GLSL std140 and Metal struct offsets with static assertions.
2. For each local vertex p compute current world `M_now*p` and previous world `M_prev*p`. Raster position uses current jittered camera. Interpolate current/previous clip/world data and perform perspective divides per fragment, not a linear interpolation of pre-divided endpoint velocities.
3. Compute pixel motion from previous and current unjittered clips using framebuffer scale `(0.5,-0.5)*internalExtent`. Reject invalid w/nonfinite results instead of pretending invalid correspondence is valid zero motion.
4. Compute expected previous world position using the same local point and previous model. For current world position x, the reference is `M_prev * inverse(M_now) * x`; use it as the CPU probe oracle.
5. Reproduce the current mapped shading normal as in GBuffer. Transform it into the expected previous world normal with `normalMatrix_prev * inverse(normalMatrix_now)` and normalize. This approximation is for the same local shading normal under rigid/affine transforms; reject singular transforms and document any unsupported reflected/nonuniform-scale cases until verified. Do not compare a rotated object's current world normal directly against its previous world normal.
6. Stop outputting token zero merely because a supported rigid transform changed. Continue to do so for invalid previous correspondence. Preserve alpha coverage and transparent domain handling.

## P2.3 — consume metadata in the accumulator

1. Keep v1 behavior unchanged. In enhanced valid v2 reprojection, locate history using the previous surface point and previous jittered view-projection; this is equivalent to applying unjittered motion plus the correct jitter delta. Use the world-point route for the first implementation to avoid ambiguous motion/jitter scaling.
2. Compare previous history position against expected PREVIOUS position, not current position. Compare previous history normal against expected PREVIOUS normal. Require matching nonzero surface tokens, previous coverage, valid clip bounds and current metadata/receiver depth agreement.
3. Reference position tolerance initially keeps existing absolute/relative settings, with the relative distance measured in previous-camera space for the expected previous point. Make the chosen formula identical in GLSL/Metal and the CPU oracle. Phase 3 refines geometric thresholds.
4. Remove the `0.001f` camera-motion whole-history rejection only for supported enhanced camera-depth inputs. Preserve projection/profile/extent/generation/sequence resets. Leave legacy/world-explicit profiles on their documented fallback behavior unless they provide a matching explicit correspondence contract.
5. Keep automatic projection-change reset conservative. Same-camera teleports/cuts must use explicit invalidateHistories/resetUpscaler; do not invent an arbitrary angular threshold. Document this requirement for callers.
6. A history first created without metadata must not suddenly be treated as metadata-valid. Switch capability/profile through local history compatibility checks. Preserve rolling/freeze countdown from Phase 0.

## P2.4 — share opaque motion with FSR

1. Add optional trailing borrowed opaque motion input and retained ownership contract to UpscalerInput; keep existing aggregate callers valid. Validate internal extent, format, device and sampled usage before passing to the SDK bridge.
2. Built-in FsrUpscaler uses supplied object-aware motion when available. Keep the existing camera-only generator as fallback. Preserve SDK sign, units, jitter and depth conventions; inspect bridge scaling instead of guessing from shader names.
3. Provide the retained opaque metadata motion generated before transparent processing, not the final transparent receiver motion. Keep the existing fullscreen camera-motion pass and extend its optional inputs to the opaque token/motion images: select object-aware motion only at a nonzero valid token, otherwise use the existing camera/depth calculation. This produces one complete motion image for the SDK, including sky/uncovered/invalid-correspondence pixels. Do not bind raw metadata motion alone, because its uncovered pixels have no object vectors.
4. Do not reset FSR on transform-only changes. Keep camera cuts, projection changes, lifecycle, settings and sequence-gap resets. The engine accumulator and SDK histories remain separate.
5. Transparent composition/refraction motion remains approximate for FSR; do not advertise it as solved or feed a single glass surface vector as motion of the entire composited color.

## P2.5 — regression and source acceptance

Extend MotionVectorGenerator CPU references or add a separate rigid-surface reference without mislabeling the camera-only API. Check stationary zero motion despite jitter; camera-only parity; known pixel translation sign; rotating plane; uniform/nonuniform scale; expected previous depth under motion along view axis; odd sizes; reduced signal extents; simultaneous camera/object motion; cuts; newly introduced instances; singular transforms; mirror transforms; aborted updates/recordings; two and three in-flight slots.

Native scenarios when authorized: a textured rigid cube translates and rotates before a wall while object history is transported; wall disocclusion restarts locally; camera pans without wholesale accumulator resets; FSR object trails improve compared with camera-only fallback. Inspect token/motion/expected-previous-position diagnostic views. Confirm fallback without enhanced formats still satisfies Phase 0.

Record source and native results separately, exact shader interface compatibility, motion units and ownership findings. Next: P3.1.

## Cost and hardware

Adds matrix/interpolator work in the metadata raster pass and per-pixel sampled data. The four metadata colors total 32 bytes/pixel before depth: token 4 + motion 4 + previousPosition 16 + previousNormal 8. At 1080p that is about 66.36 MB per complete current metadata set, not including private depth, retained/in-flight copies, or history guides. This intentionally clear first representation is bandwidth-heavy; optimization requires Phase 5 evidence. It does not add another geometry pass beyond Phase 1. No RT hardware or optical-flow dependency.

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

## Opaque object motion for FSR (P2.4)

`UpscalerInput` appends optional `UpscalerOpaqueMotion opaqueMotion`. Its motion and
token pointers are borrowed RG16F/R32_UINT images, both present or both absent.
Optional motionOwner/tokenOwner shared pointers must match the respective borrowed
pointer; FsrUpscaler retains supplied owners per invocation in the acquired slot
until that slot is retired. Unowned borrowed images must independently survive GPU
completion. Existing six-field aggregate callers and custom/resampling upscalers
remain valid; custom consumers must retain any supplied owners if they read them.

`UpscalerOpaqueMotion::validate(device, internalExtent)` checks pair/owner agreement,
same valid Device, full internal extent, sampled usage, Image2D, one mip and one
layer, and exact RG16F motion / R32_UINT token formats. FSR validates supplied
metadata even if its optional selection shader or SDK is unavailable; malformed
input is a caller error, not an unsupported-platform error. Absent metadata is
valid and keeps the camera-only path. No float conversion of tokens is permitted.

FsrUpscaler retains the original `draw/upscale/motion` camera pass and adds an
independently initialized optional `draw/upscale/motion_opaque` shader family.
Failure/missing integer sampling/kernel support latches camera-only fallback until
a stopped resize/reinitialization; it does not disable working FSR or retry during
ordinary frames. `opaqueMotionAvailable/opaqueMotionStatus` report optional support
and are exposed read-only through the existing settings wrapper. Availability is
capability readiness, not proof that every pixel supplied a valid object vector.
FSR itself remains Vulkan-only; the paired Metal selection shader is provided for
source parity/native shader probing, not native Metal FSR support.

The new fullscreen pass binds sampled depth/token/motion at Vulkan 0/1/2 and nearest
samplers at 3/4/5, mapping to Metal textures/samplers 0/1/2. Its two-matrix push data
remains 128 bytes (currentFromJittered at 0, previousFromJittered at 64), with no new
push bytes or changed legacy shader bindings. At each pixel, a nonzero token and a
finite, half-range motion vector select that vector unchanged, including valid zero
motion. Otherwise the shader executes the unchanged camera/depth calculation,
including uncovered/sky pixels. One complete internal RG16F motion image is produced
in the existing per-slot target; raw partially covered metadata is never passed
alone to FidelityFX. Reset frames use the original camera pass.

Local SDK source was inspected rather than inferring scaling from shader names:
FidelityFXBridge supplies motionVectorScale=(1,1); SDK ffx_fsr3upscaler.cpp divides it
by renderSize (no display-resolution vector flag), callbacks multiply the pixel
vector by that reciprocal size, and reproject adds it to current UV. Thus vectors
are **previous minus current**, in unjittered framebuffer pixels at internal extent.
No second normalization, output-size scaling, sign flip or jitter delta is added.
The bridge enables HDR/auto exposure only: no jitter-cancellation, inverted-depth
or display-resolution motion flag. Depth remains engine zero-to-one, non-reversed,
and SDK jitterOffset remains the actual recorded projection's pixel jitter. The
bridge and vendored SDK are unchanged.

RendererDeferred supplies only slot.opaqueTemporalSurface.motion/token, generated
before transparent processing and independently retained. It never substitutes the
last transparent receiver's vector for motion of the composited image. Metadata
replay remains explicitly enabled/default off; no FSR option silently enables the
raster pass. Transparent blending/refraction and moving reflected content still
have approximate FSR correspondence; no reactive mask or virtual reflection motion
is introduced, and no quality improvement is claimed without native validation.

FSR histories remain separate from the engine accumulator. The recorded predecessor
retains its RecordingCompletion; a different unsubmitted/abandoned predecessor,
scene sequence gap or recorded projection change resets SDK history without waits.
Submitted incomplete work is usable in the existing ordered graphics stream; same-
recording ordered work retains its existing behavior. Admission is resolved during
record, since prepare can precede submission of the prior scene. A late rejection
sets SDK reset without erasing jitter already rasterized for the current scene.
SDK dispatch jitter is recovered from the input's captured jittered/unjittered
projection matrices, rather than the latest prepare call. This keeps interleaved
slot preparation from changing the recorded scene's jitter. Explicit cuts, lifecycle,
projection, settings rebuild and gap resets remain; transform-only edits and optional
metadata presence changes do not reset FSR. No ordinary wait, immediate submission,
GPU/CPU counter readback or extra geometry pass was added.

Source review and independent odd-size jitter/pixel-to-UV arithmetic passed. No C++
or shader compilation, native probe, actual SDK dispatch, Vulkan/Metal pixel pass,
FSR trail comparison or performance measurement has run. The stopped-host
`runOpaqueMotionProbe` is SDK-independent source coverage for 5x3 mixed valid/zero/
invalid-token, valid-zero-vector, NaN/Inf fallback and sky pixels, compared against
the legacy camera pass. It also checks eight malformed pair/owner/format/extent/
usage cases. Native FSR ownership across queued slots, abandoned recordings,
interleaved preparation, resize, metadata on/off and real transparent/rigid scenes
remain acceptance scenarios for an explicitly authorized run (P2.5).

## P2.5 source acceptance and native handoff (2026-10-22)

Phase 2 is `implemented_unverified`: P2.1–P2.5 source implementation, regression
sources and static review are complete. This is not a compilation, deployed shader,
backend capability, pixel, visual quality, real overlap or performance acceptance.
Replay remains default off. Do not enable it by default on this source-only result.

### Diagnostic completion

TemporalDiagnosticView preserves Off=0, Tokens=1 and LocalHistory=2 and appends
Motion=3, PreviousPosition=4 and PreviousNormal=5. Renderer settings expose these
views plus read-only temporalSurfaceDiagnosticsAvailable. Availability is separate
from replay opt-in and actual per-key acceptance capture. Views use the exact
frame-latched opaque/transparent domain. Direct setters do not reset histories;
queued settings retain the existing generic import/reset policy.

TemporalSurfaceMetadata appends an optional retained RG16F motion owner **after**
the existing legacyCompatible member, preserving all earlier aggregate prefixes.
It is display input, not a new v3 binding or FSR interchange format. Accumulation
histories explicitly drop this unused motion owner; a display invocation retains
its own metadata/source owner through completion. Existing v3 consumption and FSR
retain their original contracts. Release external owners before destroying Device.

A separate optional `draw/temporal_diagnostic/surface` compute family shares the
existing display ABI (16-byte uint4 buffer 1; Vulkan sampled token/data at 1/2,
nearest sampler 3, RGBA16 storage 4; Metal uint/float textures 0/1, output texture 2).
Only the selected floating input is bound: RG16F motion, RGBA32F position or RGBA16F
normal. CPU checks selected format, extent, device, sampled usage and 2D/single-level
shape. Missing input returns unavailable; it is never interpreted as valid zero
motion. Shader/pipeline failure affects only these new views; original token/
acceptance display, v1/v2/v3 accumulation and camera-only FSR remain usable.

Use Raw HDR display to inspect the mappings without tone/color changes:

- Motion: `(clamp(0.5 + motion.xy/64,0,1), 0.5)`. Zero motion is neutral gray;
  +/-32 internal pixels reaches the display limits. This is a bounded signed
  visualization, not raw numeric motion or a lossless encoding.
- Expected previous position: `0.5 + 0.5 * xyz/(1+abs(xyz))`, a bounded world-axis
  visualization. The underlying source remains RGBA32F world coordinates.
- Expected previous normal: `clamp(0.5+0.5*xyz,0,1)` in previous world space.
- Token zero or nonfinite/invalid data is black. Position/normal additionally require
  w>0.5. Current coverage and previous-correspondence validity are distinct.

These views allocate/dispatch only when selected. No permanent confidence/history-
length images, synchronous telemetry, extra geometry pass or new ordinary GPU wait
is introduced. Active receiver-sized RGBA16 display images cost 8 bytes/pixel each,
with retained/in-flight/pool high-water ownership counted separately; the view's
single image size is not total feature memory. Use existing renderer.temporalSurface,
effect.temporal.record and renderer.temporalDiagnostic timing, with views off for
performance runs. Performance and peak retained memory remain unmeasured.

### Reproducible source checks

Run from the repository root, without compiling:

```sh
python3 tests/draw/temporal_surface/phase2_source_checks.py
git diff --check
```

Both passed on 2026-10-22. The Python script uses only the standard library and
checks versioned shader profile/binding/uniform contracts, paired coordinate and
normal rules, unchanged blend/confidence/freeze guide-write math, integer identity,
opaque FSR selection/SDK convention, source includes/licenses and direct absence
of ordinary wait/submission/readback. Independent matrix/jitter, perspective
interpolation, odd extent, previous-camera tolerance and FP16-quantized scalar/HDR
arithmetic checks passed. This script does not execute the C++ oracle, static_asserts,
shaders, native fixtures or SDK; its PASS is strictly source/math evidence.

A pre-step snapshot in `/tmp/bg2e-p25-before` supported exact review comparisons:
production raster metadata, v1/v2/v3 accumulation and FSR shaders, FSR implementation,
correspondence implementation, accumulator beginFrame/count/freeze/reset/setters,
renderer recordLinear/signature/fingerprint/reset/camera policies are unchanged by
P2.5. Inside accumulator record the only addition drops unused diagnostic motion
ownership. No CMake, GPU abstraction, legacy render, bake or vendor files changed.

The review also fixed a pre-existing unrun native fixture that tried to call
abandon through shared_ptr<const RecordingCompletion>. It now destroys the
unsubmitted command wrapper, which owns the mutable abandonment transition. This
is an API/source correction; no compilation failure/success was observed here.

### Regression coverage and exact outstanding native work

| Contract/scenario | Source evidence | Vulkan | Metal |
|---|---|---|---|
| Stationary zero motion despite jitter; camera-only parity | Expanded rigid CPU oracle plus independent Python matrix arithmetic | not_run | not_run |
| Previous-current pixel translation, odd/reduced extents, previous view-axis position | Rigid oracle, v3 reduced-signal fixture; Python sign/texel-center checks | not_run | not_run |
| Rotation, uniform/nonuniform scale, simultaneous camera/object motion | Independent current-world-point oracle; normal recovery and perspective divide-after-interpolation reference | not_run | not_run |
| Singular/non-affine/nonfinite/mirror models, invalid clip w/normal/extent | Oracle validity cases and source producer guards; reflected models explicitly unsupported | not_run | not_run |
| New/incompatible geometry, prepared-but-unrecorded state, cuts, gaps, immutable snapshots | Internal correspondence fixtures, including synthetic incomplete submissions in two/three slot rings | not_run | not_run |
| Two/three actual ordered submissions before fixture wait | New runQueuedTemporalReprojectionProbe uploads inputs first, submits initial batch before waiting, retires slots before reuse | not_run | not_run |
| Scalar/GI/reflection transport and temporary rolling past maxFrames with freeze authored true | Queued v3 fixture checks analytic means/confidence and valid-black GI; independent FP16 rounding tolerance check | not_run | not_run |
| Abandoned command, update-only gap and retained outputs | Queued fixture, existing surface fixture and corrected wrapper-abandon path | not_run | not_run |
| Motion/previous-position/normal display, invalid-token black and missing/wrong input | New paired display kernels; queued fixture checks colors, missing input and wrong format | not_run | not_run |
| Token/alpha/transparent private-depth coverage, custom/disabled fallback | Existing raster surface fixture and exact source predicates; its synthetic consumer remains explicitly v2-only | not_run | not_run |
| Complete opaque FSR field, finite valid vectors/zero vectors and camera/sky fallback | SDK-independent opaque selector fixture, unchanged bridge/SDK source review | not_run | selector not_run; FSR unsupported by design |
| Actual FSR SDK history ownership, interleaved prepare/record jitter, queued slots/cuts/resize | Source recording owner/projection guard and explicit future FSR host scenarios | not_run | FSR unsupported by design |
| Real textured cube/wall with AO/GI/reflections, rotating normals and disocclusion | Manual visual acceptance scene below; synthetic fixtures are not this acceptance | not_run | not_run |
| Formats, shader compilation/deployment, peak retained memory and frame cost | Capability gates/pool ownership reviewed only | not_run | not_run |

Native source fixture entry points, all uncompiled/unexecuted:

- `runRigidSurfaceReferenceProbe()` in temporal_surface/rigid_probe.hpp, plus
  `runTemporalCorrespondenceProbe()` and `runTemporalCorrespondenceChronologyProbe()`
  in correspondence_probe.hpp (link its private implementation).
- `runTemporalReprojectionProbe()` and new `runQueuedTemporalReprojectionProbe()`
  in temporal/reprojection_probe.hpp and queued_reprojection_probe.hpp. The latter
  requires optional v3 and surface display support and returns false for unsupported
  support, never success. It uses synthetic receiver inputs, not rasterized v3
  geometry. The CPU ticket ring is not evidence of native GPU overlap; native
  batches may also complete naturally before a fixture wait.
- Existing temporal scalar/HDR/freeze, metadata malformed-pair, invalidation, actual
  raster surface and `runOpaqueMotionProbe()` sources remain mandatory baselines.

For an authorized native session, record backend/device, exact deployed shader
families, capability/status output, assertions and failures, then inspect a textured
rigid cube translating/rotating before a wall with actual AO/GI/reflections. Inspect
token, motion, expected previous position/normal and actual local acceptance views.
Verify transported receiver history, local wall disocclusion restart, camera pan
without wholesale v3 reset, two/three ordered slots, retained earlier outputs,
alpha UV0/UV1/scaled holes, equal-depth order, separate transparent full clears and
opaque-only FSR input. Compare FSR object trails against camera-only fallback.
Repeat replay disabled/custom/unsupported to verify Phase 0 rolling/freeze behavior.
Exercise editor/gizmo explicit-reset boundaries, cuts, projection/profile/extent
changes, diagnostic gaps and resize. Keep shader/build failure, unsupported hardware
and unavailable probes distinct from pass. Record visual trails/aliasing and real
indirect-light changes on stationary receivers rather than claiming motion vectors
solve those effects. Deformation, mirror support, transparent/refraction composite
motion, reactive masks and virtual reflected motion remain excluded/approximate.

Next source step: **P3.1** in phase_3_disocclusion.md. Specify and implement the
validated per-candidate bilinear footprint and independent CPU sampling oracle;
do not silently execute P3.2–P3.5. Native Phase 2 acceptance remains pending.
