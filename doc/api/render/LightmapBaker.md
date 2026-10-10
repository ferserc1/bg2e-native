# LightmapBaker

**Header:** `<bg2e/render/LightmapBaker.hpp>`
**Namespace:** `bg2e::render`

Base class owning one target's UV surface attachments, AO/GI sample images,
composition images, accumulation histories and dilated result. Its constructor
is protected; obtain an `IntegratedLightmapBaker` or `StandaloneLightmapBaker`
from the corresponding shared context.

## Target requirements

The target must belong to the context root and directly contain a standard,
GPU-loaded `scene::Drawable` with nonempty indexed triangle geometry. CPU
submesh metadata must match the Drawable; ranges and vertex indices must be
valid. `geo::UvAtlasValidator::validate(*mesh, 1)` must accept UV2. Null,
detached, unloaded or malformed targets throw `std::invalid_argument` at creation.
See [settings and validation](LightmapSettings.md).

## Members

| Signature | Contract |
|---|---|
| `uint32_t completedFrames() const` | Successful updates counted: recorded updates for integrated baking, submitted and waited updates for standalone. Does not itself query GPU completion. |
| `void resetAccumulation()` | Set progress to zero and invalidate result access. Next sample ignores old history without a GPU clear. |
| `LightmapPixels readPixels() const` | Require a result and engine frame advance since its last update; then wait for device idle and synchronously copy the dilated RGBA32F image to tightly packed RGB. |
| `std::shared_ptr<vulkan::Image> image() const` | Return the current dilated RGBA32F sampled image. Require a recorded result; this call does not wait for GPU completion. |
| `const LightmapSettings& settings() const` | Immutable settings stored in this baker. |
| `std::shared_ptr<scene::Node> targetNode() const` | Retained target reference. |
| `virtual ~LightmapBaker()` | Schedule GPU resource release through engine deferred cleanup. |

`image()` and `readPixels()` throw `std::logic_error` before the first result
or after reset. `readPixels()` also throws if the engine frame has not advanced;
calling it while recording the last integrated update is invalid. Readback is
available after any submitted sample, not only after all requested frames.
Further updates after completion throw until reset. A retained image is mutable
GPU storage reused by later updates, not a frozen snapshot.

## Settings and output

See [LightmapSettings, LightmapPixels and enums](LightmapSettings.md) for every
field, default and CPU conversion rule. RTAO is a grayscale RGB multiplier;
RTGI is a colored environment-relative indirect-light multiplier. Both result
accessors use the same dilated image. Dedicated RGB material-channel support
remains pending; the current output workflow uses the existing AO property.

## Execution-specific methods

- [IntegratedBakerContext and IntegratedLightmapBaker](IntegratedBakerContext.md): `update(cmd, frameResources)` records into the caller's frame.
- [StandaloneBakerContext and StandaloneLightmapBaker](StandaloneBakerContext.md): `update()` submits and waits synchronously.

Sources: [header](../../../lib/include/bg2e/render/LightmapBaker.hpp),
[implementation](../../../lib/src/bg2e/render/LightmapBaker.cpp).
For the actual UV projection see [atlas surface projection](../../architecture/lightmaps/uv_surface_projection.md).
