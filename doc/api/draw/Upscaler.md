# Upscaling: FSR, MetalFX and the extension contract

## Choosing a reconstruction method

An upscaler reconstructs the **complete composed linear HDR image** from internal
render size to output size. This is separate from
[guided GI upsampling](GIUpsampling.md), which reconstructs one lighting signal
before composition. Temporal reconstruction additionally uses previous images,
depth, motion and subpixel camera jitter; bilinear reconstruction uses only current
color. Jitter is a small projection offset, not physical camera motion.

| Public method | Implementation | Principal requirements |
|---------------|----------------|------------------------|
| `None` (0) | Bilinear fallback, no SDK history | Built-in selection sets scale 1.0 |
| `FSR` (1) | `FsrUpscaler`, FidelityFX FSR 3.1 | Vulkan backend, compiled optional SDK, supported formats/device, 1–4 SDK frame slots |
| `MetalFX` (2) | `MetalFXUpscaler`, native temporal scaler | Metal backend, macOS 13+, supported device and accepted scale/configuration |

This path supplies reconstruction, not SDK frame generation. DLSS is not
implemented. Public method/mode controls retain a backend implementation rather
than constructing a new one on every frame. Backend-incompatible selections
reject; optional unavailability normally leaves bilinear rendering operational.

`UpscaleMode` selects these fractions of output **per dimension**: Native=1.0,
Quality=0.60, Balanced=0.55, Performance=0.50, UltraPerformance=0.35. Presets use exact
integer floor and a one-pixel minimum. None uses 1.0 while retaining the selected
mode. These are engine ratios; the names do not imply SDK-standard ratios or
support for every preset. Unsupported MetalFX scale preserves the selected
internal extent with bilinear fallback.

Configure before initialization, for example on a Vulkan engine:

```cpp
bg2e::draw::RendererConfiguration configuration;
configuration.upscaleMethod = bg2e::draw::UpscaleMethod::FSR;
configuration.upscaleMode = bg2e::draw::UpscaleMode::Quality;
// Supply configuration to DefaultRenderLoopDelegate::setRenderer.
```

On a Metal engine choose `UpscaleMethod::MetalFX`. For live edits, enqueue a
conservative renderer operation, use the renderer passed to that callback and
check `supportsUpscaleMethod` before calling `setUpscaleMethod`/`setUpscaleMode`.
Extent changes require the delegate's reconfiguration boundary. Do not resize or
replace GPU resources inside an unsynchronized UI callback. The settings facade
already queues this work; see [Settings](Settings.md).

`supportsUpscaleMethod` indicates selector compatibility; it is not equivalent to
`upscaler()->available()` or successful recording. Inspect `status()`,
`sdkAvailable()`, `backendSupported()` and optional opaque-motion diagnostics to
understand fallback. The FSR implementation also exposes `hardwareStatus()` and
sharpness in [0,1], default 0.5; MetalFX has no corresponding public sharpness control.

## Pipeline and anti-aliasing behavior

Both implementations consume output-before-tone-map RGBA16F color, preserved opaque
depth and a camera snapshot. They produce output-sized linear HDR RGBA16F. The
renderer performs tone/color conversion once afterwards. A successful record from
a jitter-requiring implementation skips the optional `FinalProcessor`, including
SMAA, for that frame. On bilinear fallback an injected processor can run; the
renderer does not insert SMAA automatically. Temporal diagnostic images bypass
SDK reconstruction.

Native mode requests temporal reconstruction at matching input/output sizes; it
can act as temporal anti-aliasing where accepted. It does not promise that MetalFX
accepts 1× on every system. Orthographic/identity and other unsupported camera
projections use an unjittered fallback when detected by `prepare`.

Opaque receiver replay is explicit (`setTemporalSurfaceEnabled(true)`) and defaults
off. With replay off or invalid metadata, camera/depth vectors cover the image.
With valid opaque tokens, rigid object vectors replace those estimates locally.
Transparent/refraction color and reflected content can have motion different from
the opaque receiver; no reactive mask or virtual reflected-object vectors correct
that mismatch. Temporal ghosting remains possible.

## Shared lifecycle and custom implementations

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

RendererDeferred retains a disabled backend implementation by default:
FsrUpscaler on Vulkan and MetalFXUpscaler on Metal. ResamplingUpscaler handles
None and unavailable/failed reconstruction. DLSS is not implemented.
See [settings and limits](Settings.md),
[renderer injection](Renderer.md) and [deferred tutorial](../../tutorials/draw/10_deferred_renderer.md).

## Opaque object motion for FSR (P2.4)

`UpscalerInput` appends optional `UpscalerOpaqueMotion opaqueMotion`. Its motion and
token pointers are borrowed RG16F/R32_UINT images, both present or both absent.
Optional motionOwner/tokenOwner shared pointers must match the respective borrowed
pointer; the built-in temporal consumers retain supplied owners per invocation in the acquired slot
until that slot is retired. Unowned borrowed images must independently survive GPU
completion. Existing six-field aggregate callers and custom/resampling upscalers
remain valid; custom consumers must retain any supplied owners if they read them.

`UpscalerOpaqueMotion::validate(device, internalExtent)` checks pair/owner agreement,
same valid Device, full internal extent, sampled usage, Image2D, one mip and one
layer, and exact RG16F motion / R32_UINT token formats. The built-in temporal consumers validate supplied
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


## Native MetalFX temporal reconstruction

`<bg2e/draw/MetalFXUpscaler.hpp>` implements `Upscaler` through the low-level
`gpu::metal::MetalFXBridge`. The bridge contains no draw/scene dependencies and
keeps Objective-C/MetalFX types private to its macOS Objective-C++ implementation.
Other platforms build an unavailable stub. The engine weak-links MetalFX and checks
macOS 13 availability and `supportsDevice` before offering the native selector.

`RendererDeferred::UpscaleMethod` retains None=0 and FSR=1 and adds MetalFX=2.
`RendererConfiguration::upscaleMethod` and `upscaleMode` carry selection through
renderer recreation; an unset method derives the initial injected implementation's
enabled state. Explicit custom upscalers retain their implementation. Graphics
preferences continue to expose only Upscale and Mode. None uses native extent and
retains the chosen mode. Native/Quality/Balanced/Performance/Ultra Performance use
100/60/55/50/35 percent of each output dimension, with exact integer floor and a
one-pixel minimum for these presets.

On macOS 14+, the bridge checks the device's reported minimum/maximum output/input
scale independently for each rounded dimension. On macOS 13, it conservatively
admits 1x through 2x, then requires scaler creation to succeed. Native AA and the
35% preset are never assumed operational: an unsupported scale or rejected scaler
configuration retains the selected mode/internal extent and uses **unjittered
bilinear reconstruction**. Availability/status capability metadata identifies this
fallback. No automatic SMAA substitute or extra AA/sharpness control remains.
An explicitly injected final processor retains its existing fallback behavior.

The input and output remain linear HDR RGBA16F before tone mapping. MetalFX uses
auto exposure and preExposure=1; it does not perform the renderer's final tone map.
A fullscreen pass converts preserved D16/D32 opaque depth to R32F. Motion is RG16F,
previous-current unjittered internal framebuffer pixels with Y down, so the native
motionVectorScale is (1,1). The existing complete camera/depth pass and optional
opaque-token/object-motion selection pass are reused. Receiver replay remains an
explicit renderer option; without valid metadata the camera/depth path is used.
Transparent/refraction motion and reflected content remain approximate.

`prepare` uses a centered Halton(2,3) sequence with at least eight phases, scaled
by the square of the reconstruction ratio. It applies +2*x/width and -2*y/height
clip translation. `record` recovers native pixel jitter from the actual captured
jittered projection, preserving correctness across interleaved slot preparation.
Only finite conventional perspective cameras with near=0/far=1 depth are admitted;
other projections use unjittered bilinear reconstruction.

The bridge queries all native texture usage requirements and allocates tracked,
private per-slot input/output textures at the synchronized initialization/resize
boundary. GPU blits adapt existing renderer-owned resources to those requirements.
No per-frame scaler recreation, CPU readback, immediate submit, or explicit wait
is introduced by MetalFX record/prepare. Borrowed opaque owners, intermediate
images, descriptors, native textures and recording completion handles remain
retained until acquired-slot retirement. Cleanup requires the existing drained
renderer lifecycle boundary.

Explicit camera cuts/history invalidation, projection changes, mode/extent changes,
scene sequence gaps and abandoned predecessor recordings reset MetalFX history.
Late predecessor rejection resets native history without changing already recorded
jitter. Scaler initialization/allocation/encoding failures preserve the bilinear
fallback. Asynchronous command errors disable the scaler for later frames until a
synchronized rebuild; the failed GPU submission itself remains an error handled by
the existing queue. Only a successful reconstruction sets temporalUpscaleApplied,
which bypasses the final processor for that slot.

Implementation references: [Apple descriptor API](https://developer.apple.com/documentation/metalfx/mtlfxtemporalscalerdescriptor)
and [Apple temporal scaler API](https://developer.apple.com/documentation/metalfx/mtlfxtemporalscalerbase).
Platform-specific runtime and visual acceptance must be checked on the target
device; the presence of the source implementation does not establish that coverage.
