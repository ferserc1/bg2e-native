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
