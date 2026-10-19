# OffscreenSurface

Header: `<bg2e/gpu/OffscreenSurface.hpp>`. Namespace: `bg2e::gpu`.

`OffscreenSurface` derives virtually from `Surface`. Its constructor is
`explicit OffscreenSurface(const Size2D& size)` and `isOffscreen()` returns true.
The base records the requested size and delegates resource lifecycle to the native
surface. Construction alone does not create usable GPU targets or imply validity.

Obtain a surface from `Backend::createOffscreenSurface(instance, size)` and follow
the [GPU initialization workflow](index.md#offscreen-rendering). Device creation
creates the configured render target; current Vulkan and Metal implementations
report valid only while their color image exists and is valid. For managed scene
execution, [draw Engine](../draw/Engine.md#headless-initialization) owns that setup.

## Targets and frames

The current native implementations expose one color image and one in-flight slot;
`colorImage(0)` is borrowed and `depthImage()` may be null when depth is omitted.
Formats are inherited Surface configuration. `beginFrame()` acquires the slot
and waits for its previous tracked submissions. Record commands against the
acquired frame and call `present(command)` to associate the offscreen frame with
submission tracking. This does not present to an OS window. `endFrame(frame)`
requires a submitted acquired frame and advances bookkeeping; it does not itself
wait for the newly submitted work to finish.

The [OffscreenApplication](../app/OffscreenApplication.md) driver uses synchronous
`Device::immediateSubmit`, so its callbacks are after completion. Custom GPU code
must establish its own completion before readback, target reuse or destruction.
Offscreen color should use a useful image layout such as TransferSrc for export;
do not transition it to Present merely because the method is named present.

## Native implementations and recreation

`gpu::vk::OffscreenSurface` and `gpu::metal::OffscreenSurface` implement target
creation, validity, frame acquisition/tracking, resize and release. Their headers
are `<bg2e/gpu/vk/OffscreenSurface.hpp>` and
`<bg2e/gpu/metal/OffscreenSurface.hpp>`; ordinary clients use the abstract API.
`cleanup()` releases render targets. Resize/release coordinates device/frame
completion before replacing shared owners and changes surface generation. Keep
background producers stopped across those boundaries and refetch borrowed images
and views after recreation. The fixed-size application job does not expose resize
as a mid-accumulation operation.

See [Surface](Surface.md), [SurfaceFrame](SurfaceFrame.md),
[Image readback](Image.md#completed-image-readback) and
[submission tracking](Submission_tracking_and_waitIdle.md).
