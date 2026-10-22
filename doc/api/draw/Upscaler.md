# Upscaler extension contract

Header: `<bg2e/draw/Upscaler.hpp>`. Implement Upscaler and retain the shared owner
in RendererConfiguration::upscaler. identifier/label provide stable identity and
UI naming. requirements declares opaque depth, camera motion, jitter and storage
needs; primitive support is separate from optional SDK availability.

All calls run on the render thread. Initialize, resize and cleanup require a
drained device. Per-slot descriptors and borrowed inputs survive GPU completion.
initialize(Engine&, const UpscalerConfiguration&) and resize configure resources;
available/status report runtime support. prepare(camera, sceneFrameNumber) returns
the projection for recording, unjittered by default. UpscalerInput borrows frame,
linear HDR color, preserved opaqueDepth, output, unjittered camera and the actual
jitteredProjection. record(input) returns false when no usable result was produced;
the renderer then resamples unmodified HDR. Optional backend/SDK unavailability
must not itself throw. Complete successful recording before output is consumed.

reset invalidates reconstruction history; cleanup releases resources while Engine
is alive. needsRebuild defaults false. settingsType describes live accessors with
stable property IDs. Metadata receives the complete dynamic object address,
including multiple inheritance, and catalog targets retain the component owner.
Rebind catalog generations on replacement; never keep borrowed instance pointers
beyond that owner. Change settings at a stopped boundary and reset histories on
camera/projection/extent discontinuities.

ResamplingUpscaler is the bilinear default. FsrUpscaler is optional Vulkan
integration; Metal/missing SDK/failure falls back. Native Metal reconstruction
and DLSS are not implemented. See [settings and limits](Settings.md),
[renderer injection](Renderer.md) and [deferred tutorial](../../tutorials/draw/10_deferred_renderer.md).

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
