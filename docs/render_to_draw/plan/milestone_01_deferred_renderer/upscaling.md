# M01-13 actual upscaling contract

[Step](step_13_upscaling-fsr.md) · [Progress](../progress.md) · [Lead checks](../../../../tests/draw/upscaling/README.md)

## Public surface and settings

`draw::Upscaler` is an open virtual family with identifier/label, requirements,
initialize/resize/prepare/record/reset/cleanup, capability/status, settingsType and
needsRebuild. Inject any shared implementation through
`RendererConfiguration::upscaler`; null selects public `ResamplingUpscaler`.
No engine enum or factory registration is required. The renderer exclusively
manages the selected object's active lifecycle; do not share one active instance
between renderers. Configurations/catalogs retain ownership.

Requirements explicitly declare opaqueDepth/cameraMotion/jitter/storageOutput.
`UpscalerConfiguration` gives actual extents, acquired slot count and shader root.
`UpscalerInput` borrows the current frame, fully composed linear HDR, preserved
opaque depth, output-sized linear HDR target, camera data and jittered projection.
The working depth after the last transparent object is deliberately not used.
Implementations retain descriptors/resources by acquired slot through completion.
False from record selects the ordinary resampler; capability/backend failures
must be reported as unavailable instead of throwing. Contract errors may throw.

SettingsType describes the implementation's real live properties. Catalog
accessors receive the complete object address (`dynamic_cast<void*>`), supporting
multiple inheritance. Preferences append `upscaler/<identifier>` automatically,
retain its owner and use the existing exact typed codec/generation rules.
ApplicationUpscaler in the manual fixture demonstrates an independent composed
implementation with a writable bypass and read-only recording counter.

`ResolutionSettings` has enabled and scale (finite [0.25,1]). It is retained by
configuration and catalog component `resolution`, type `bg2e.draw.resolution.v1`.
Disabled preserves the original explicit internal extent or zero-follow-output
startup convention. Enabled resolves floor(output*scale), clamped to at least
one pixel independently per axis. This is the authoritative quality/ratio policy
for both bilinear and FSR (e.g. .67 quality, .59 balanced, .5 performance); no
parallel SDK-quality preference model exists. Null creates a disabled live policy
so existing output remains native/fixed until the lead enables reduction.

Renderer's resolveInternalExtent and needsReconfiguration are optional virtual
hooks. The delegate consults them after queued edits and before acquisition;
changed extents or implementation rebuild requests drain GPU work, detach catalog
targets, resize, then rebind with fresh generations. Its original fixed extent
is retained across scale disable, output resize and format reconstruction.
FinalLayer live color settings survive this boundary. Replace an upscaler through
a queued renderer replacement/configuration; keep the same final layer selection
if its settings must survive renderer replacement. External custom metadata
continues to use ExtendCatalog where additional targets are needed.

## Graph, native ABI and color

RendererDeferred always reconstructs composed HDR to one acquired-slot-owned
output-sized RGBA16F image, followed by FinalLayer's single tone/color operation.
Public fallback uses an explicit four-tap bilinear fullscreen pass, nearest
sampler and clamped addresses; it needs no linear-filter format support, RT, SDK,
engine history, MSAA, readback or additional submission. Native and internal
extents may differ on both backends, including odd and one-pixel sizes.

The final graph contract is `draw.raster.final.reconstructed.v1`, obtained through
`pipeline::finalContract(output, depth, true)`. Its first input is output-domain
linear HDR; remaining seven G-buffer debug sources stay internal-domain. Injected
final passes must use that contract. The default false argument retains the
public standalone `draw.raster.final.v1` internal-color path. Final GLSL now picks
the selected source size for debug reconstruction; Metal already did so.

Paired shaders are draw/upscale/resample and draw/upscale/motion. Both use sampled
image set0/binding0, sampler binding1 (Metal texture0/sample0), fragment push
buffer0. Resample push is output extent vec4 (16 bytes); motion push is two mat4
(128 bytes). The existing recursive paired shader deployment handles them with
no CMake edits.

## FSR SDK boundary and lifetime

`FsrUpscaler`, identifier/type `bg2e.draw.fsr3.v1`, owns enabled, sharpness [0,1],
live status/available plus separate sdkAvailable/backendSupported/hardwareStatus.
SDK availability follows the existing non-Apple SDK linkage and header presence;
Metal/Apple do not acquire an SDK context. Hardware status is explicitly "Not
assessed" when no SDK/backend query occurred; SDK presence is not hardware proof.

Only `gpu::vk::FidelityFXBridge.cpp` includes SDK/Vulkan types and performs native
casts. Its public header uses GPU types only, with no draw/higher-level dependency.
The bridge queries actual shader/HDR storage support, creates an HDR/auto-exposure
FSR 3.1 context plus SDK-described dilated depth/motion/reconstructed depth, and
registers linear RGBA16F color/output, sampled D16/D32 depth and RG16F motion.
Partial context creation tracks successful backend allocation so later SDK errors
release partial resources. Scratch allocation is owned until context destruction.

Context and SDK shared resources belong to one ordered graphics submission
stream, while output/motion/descriptor resources belong to acquired slots. The
bridge rejects more than four in-flight slots (the vendored SDK descriptor ring
limit). Shared resources receive same-layout ordering barriers each invocation.
SDK unregister restores external images to registered states: sampled inputs
ShaderReadOnly, UAV output/shared General. The generic output transition then
orders SDK writes and exposes ShaderReadOnly to FinalLayer. No normal frame idle,
immediate submit, CPU image transfer or separate SDK queue is introduced.

FsrUpscaler prepares projection jitter using sceneFrameNumber, not presentation
number, and computes camera-only previous-minus-current unjittered framebuffer
motion in pixels. Both Vulkan's negative-height viewport and Metal invert NDC Y.
The motion shader maps current jittered clip into current/previous unjittered
clip; stationary camera motion is zero even as projection jitter changes.
SDK motion scale is (1,1); no SDK jitter cancellation flag is needed. The opaque
projection is shared by raster lighting/query shadows/skybox for that frame.

First dispatch, scene attach, resize/replacement, projection change, scene-frame
discontinuity and explicit resetUpscaler reset SDK history. Call resetUpscaler at
a drained/pre-acquire boundary after camera cuts/discontinuous object changes.
Entering explicit camera mode, useSceneCamera and setCameraParameters also reset;
repeated setCamera view updates preserve motion/history. The supported
minimal camera is conventional finite RH perspective with zero-to-one depth;
identity/orthographic uses unjittered bilinear output with a status reason.

Initialization/kernel/SDK failure, disabled FSR, unsupported backend/device or
unsupported camera retain default output and the selected reduced extent. A
failed dispatch disables further dispatch attempts until a safe rebuild; retry
through disabled/enabled or renderer replacement. One already-jittered failed
frame can be reconstructed by fallback; subsequent frames are unjittered.
FSR context resize/re-enable/cleanup drain users; live sharpness does not rebuild.

No object motion, reactive/transparency mask, general TemporalAccumulator or
DenoiseFilter is implemented. Moving objects and glass can ghost; M03 extends
motion/reset information. Native Metal upscaling and DLSS remain external
extension cases, not claimed implementations.

## Validation boundary

Baseline b955833f314d plus preserved earlier uncommitted work and lead fixes.
M01-12, including its ignored-result correction, accepted by project lead
continuation; platform/backend coverage unspecified. Agent checks are static
only: declarations/call sites, native SDK signatures/state restoration, shader
pairs/packing/Y convention, ownership, dependency boundary, links and whitespace.
No build configuration, C++/shader compilation, executable/probe/runtime tests or
CMake edits were performed. The linked manual checks cover supported Vulkan,
Metal/no-SDK fallback, reduced/default/sRGB/debug output, transitions, reflection,
external injection, resets and earlier deferred/production regressions.

## M01-15 constructor integration correction

FinalLayer now accepts an optional final constructor argument reconstructedColor (default false). Pass true for RendererDeferred injection so contract() already reports final.reconstructed.v1 before Renderer::validateConfiguration; initializing the contract later is too late. Standalone default usage remains source-compatible. See working-example.md.
