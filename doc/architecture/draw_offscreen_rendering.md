# Headless draw rendering and accumulated camera output

The draw path supports a windowless Engine and a typed
[OffscreenApplication](../api/app/OffscreenApplication.md). Production render's
offscreen overload remains separate and supported. Backend selection within draw
chooses Vulkan or Metal; it does not select the high-level render framework.

## Ownership and completion

The app owns Engine, backend lease, Device and OffscreenSurface. Its default
[scene delegate](../api/draw/DefaultOffscreenApplicationDelegate.md) reuses the
windowed renderer/scene owner without constructing a window, UI or MainLoop.
`surface()` remains the window accessor and is null headlessly; generic clients
use `renderSurface()`, `offscreenSurface()` and `frameSlotCount()`.

Current offscreen surfaces use one reusable slot. Each scene submission completes
before the callback contributes its sample. The app drains submitted work before
releasing delegate resources, then destroys surface/device/instance. Custom
producers must stop across lifecycle boundaries. FrameContext references and
completed surface pointers are borrowed; copy host output before run ends.

## Signal and sample count

```text
scene update and lighting/transparency
    -> output-sized reconstructed linear RGBA16F sample
    -> GPU completion -> synchronous readback -> CPU double RGB sum
    -> repeat until exactly N successful scene contributions
    -> divide by N, alpha=1 -> one RGBA16F upload
    -> final tone/color and optional SMAA -> GPU completion
    -> last didRenderFrame: hasResult=true -> encoded image
```

Reconstruction occurs per sample. Final color and optional SMAA run once on the
mean; their final-only submission and readback helpers add no scene sample. N=1
still resolves and converts once. Each requested sample contributes, including
sample zero; no hidden warm-up is discarded. AO/GI/reflection and FSR histories
advance normally and can correlate the scene signals. Averaging them is distinct
from independent raw ray samples, an effect's temporal history or UV coverage/counts
for bake. The reference implementation is CPU/readback bound at large dimensions/N.

Zero timestep freezes time; application frame-counter animation must also stop.
A positive fixed timestep averages an animation sequence, beginning at zero delta.
Job settings/root/generation/output stay fixed. Frozen fingerprints reject detected
camera/resource/history changes; callers serialize opaque in-place GPU writes or
invalidate explicitly and start another job. No editor overlays enter this path.

## Configuration and pixels

The initial stopped boundary imports live settings. An explicit document bypasses
persisted defaults and explicit internal-size/scale overrides win. Unknown target
IDs reject unless the caller selects explicit diagnostic skipping or supplies a
catalog mapping. Render settings do not override the job's output size/backend/N.
Optional unavailable RT/FSR follows ordinary renderer fallback; required bake
capabilities are a different job contract.

The shipped CLI selects R8G8B8A8_UNORM after final rendering. GPU readback returns
tight rows in image order; the intended PNG top row is row zero. The DB writer
adds no flip, gamma, channel conversion or alpha normalization. It encodes the
final rendered bytes and verifies exact RGBA dimensions/bytes by decoding the
PNG before publication. This verifies codec preservation; independent scene/image
inspection is still needed for visual orientation and content. Generic raw
readback is not an automatic BGRA/sRGB/HDR conversion service.

See the [CLI guide](../apps/draw_offscreen_application.md) and
[tutorial](../tutorials/draw/13_offscreen_application.md). Complete scene/resource
save and standalone bake batches have separate ownership from this camera-image
product; the [integrated lightmap architecture](draw_integrated_lightmaps.md)
describes the available bake path.
