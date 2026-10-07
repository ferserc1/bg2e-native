# Integrated Lightmap Baker

**Headers:** `<bg2e/render/LightmapSettings.hpp>`,
`<bg2e/render/IntegratedBakerContext.hpp>`, and
`<bg2e/render/LightmapBaker.hpp>`  
**Namespace:** `bg2e::render`

The integrated baker runs inside an existing windowed or offscreen engine
frame loop. A shared `IntegratedBakerContext` owns the bake ray-tracing scene
for each in-flight frame slot; each `IntegratedLightmapBaker` owns the target's
UV-space buffers, accumulation, and result. The scene root and engine must
outlive the context and its bakers. A target must be an attached standard
Drawable with a usable UV2 atlas.

```cpp
auto context = std::make_shared<render::IntegratedBakerContext>(engine, sceneRoot);
context->setEnvironmentResources(renderer->environmentResources());

render::LightmapSettings settings;
settings.mode = render::LightmapMode::RTGI;
settings.resolution = 512;
settings.accumulationFrames = 16;
auto baker = context->createBaker(targetNode, settings);
```

In the application's render delegate, after scene/component updates and while
recording the engine's current frame, prepare the context once and update each
target baker:

```cpp
context->prepareFrame(cmd, frameResources); // once per context per frame
baker->update(cmd, frameResources);         // once per sample/frame
```

The command buffer must be the one belonging to the current `FrameResources`.
The integrated context does not run scene lifecycle callbacks. The application
continues to own them. The baker's updates must be recorded in the same ordered
command stream and submitted by the running loop.

## Settings and output

`LightmapMode` is `RTAO` or `RTGI`. `LightmapPixelFormat` is `RGB8` or
`RGB32F`. `LightmapSettings` defaults are:

| Field | Default | Meaning |
|-------|---------|---------|
| `resolution` | `512` | Square output and intermediate resolution. |
| `mode` | `RTGI` | Ray-traced ambient occlusion or global illumination. |
| `accumulationFrames` | `16` | Number of update calls that form the accumulation. |
| `samplesPerPixel` | `8` | Ray samples per UV texel per update. |
| `giBounces` | `2` | RTGI bounce count. |
| `maxRayDistance` | `50.0f` | RTGI ray range in metres; RTAO uses a fixed 0.1 m radius. |
| `giRayBias` | `0.0005f` | RTGI ray-origin offset in metres. |
| `exposureEV` | `0.0f` | RGB8 RTGI export exposure (`2^EV`) before clamp; does not change RGB32F or the GPU image. |
| `cpuFormat` | `RGB8` | Format returned by `readPixels()`. |

Each `update()` records one full-resolution accumulation iteration. Use
`completedFrames()` to observe progress. Once the configured frame count is
reached, further updates require `resetAccumulation()` first. `readPixels()`
waits for the submitted result and returns tightly packed, row-major RGB
pixels; RGB8 contains bytes, RGB32F contains floats. `image()` returns the
sampled Vulkan result image. The result is an indirect-light multiplier:
RTAO is grayscale visibility replicated across RGB, while RTGI is colored
indirect light normalized against the unoccluded environment. Direct lighting
and material emission are not multiplied by the result.

## Public result access

| Member | Description |
|--------|-------------|
| `IntegratedLightmapBaker::update(VkCommandBuffer, vulkan::FrameResources&)` | Records one sample for the prepared current frame. |
| `LightmapBaker::completedFrames() const` | Number of recorded accumulation updates. |
| `LightmapBaker::resetAccumulation()` | Clears accumulation history and progress. |
| `LightmapBaker::readPixels() const` | Waits for completion and returns `LightmapPixels`. |
| `LightmapBaker::image() const` | Shared handle to the renderable/sampled GPU result. |
| `LightmapBaker::settings() const` | Immutable settings used by this baker. |
| `LightmapBaker::targetNode() const` | Shared target node reference. |
| `IntegratedBakerContext::setEnvironmentResources(EnvironmentResources*)` | Supplies the active renderer's environment resources; the renderer must outlive the context. |

The same context can create bakers for multiple targets. Call
`prepareFrame()` only once per active bake frame, then update each baker with
that prepared command buffer and frame resources. Read CPU pixels outside
command recording, after the frame has been submitted and advanced.

## See also

- [UV atlas generation and editor previews](../../lightmap_baking.md)
- [`geo::GenerateUv2AtlasModifier`](../geo/GenerateUv2AtlasModifier.md)
- [`render::UvMapPreviewRenderer`](UvMapPreviewRenderer.md)
- [`ui::UvMapPreview`](../ui/UvMapPreview.md)
- [Standalone baking](../../../docs/lightmap_baker/03_standalone_baker/overview.md) — planned headless CLI phase.
